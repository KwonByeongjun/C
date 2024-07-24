#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define PageSize 4096  // 페이지 크기 (4KB)
#define BlockSize (4L * 1024 * 1024)  // 블록 크기 (4MB)
#define DeviceSize (8L * 1024 * 1024 * 1024)  // 디바이스 크기 (8GiB)
#define PPB (BlockSize / PageSize)  // 블록당 페이지 수
#define TotalBlocks (DeviceSize / BlockSize) // 전체 블록 수 (8GiB / 4MB)
#define TotalPages (TotalBlocks * PPB)  // 전체 페이지 수
#define LogicalSize (8L * 1000 * 1000 * 1000)  // 논리 크기 (8GB)
#define GCBoundary (8L * 1000 * 1000 * 1000)  // 8GB마다 통계 출력
#define FreeBlockThreshold 3
#define LAB_NUM (LogicalSize / PageSize)  // LBA 수

typedef struct {
    bool valid;  // 페이지 유효성
} Page;

typedef struct {
    Page *pages;  // 동적 할당된 페이지 배열
    int freePageOffset;  // 블록당 free page offset 유지
    int validPageCount;  // 블록당 valid 페이지 수 유지
} Block;

typedef struct {
    int *free_block_queue;
    int free_block_front;
    int free_block_rear;
    int free_block_count;
} SSD;

typedef struct {
    double timestamp;
    int io_type;  // 0: READ, 1: WRITE, 2: X, 3: TRIM
    unsigned long lba;
    unsigned int size;
    unsigned int stream_number;
} IORequest;

// 글로벌 변수들
Block *blocks;
SSD ssd;
int *mappingTable;  // 논리 -> 물리 매핑 테이블
int *OoBa;  // Out of Band area
int current_active_block;
int current_active_page;
unsigned long user_written_data = 0;  // 사용자 데이터 쓰기량
unsigned long gc_written_data = 0;  // 가비지 컬렉션 쓰기량
unsigned int progress_boundary = 8;
int remainFreeBlocks = 0;
unsigned long utl = 0;  // Utilization을 위한 페이지 수
unsigned long erase_count = 0; // ERASE 횟수 추적

// 누적 데이터 추적 변수
unsigned long cumulative_written_data = 0;
unsigned long cumulative_gc_written_data = 0;
unsigned long last_checkpoint_data = 0;
unsigned long last_checkpoint_gc_data = 0;

// 큐 함수들
void init_queue(SSD *ssd) {
    ssd->free_block_queue = (int *)malloc(sizeof(int) * TotalBlocks);
    ssd->free_block_front = 0;
    ssd->free_block_rear = -1;
    ssd->free_block_count = 0;
}

void enqueue(SSD *ssd, int block_index) {
    ssd->free_block_rear = (ssd->free_block_rear + 1) % TotalBlocks;
    ssd->free_block_queue[ssd->free_block_rear] = block_index;
    ssd->free_block_count++;
    remainFreeBlocks++;
}

int dequeue(SSD *ssd) {
    if (ssd->free_block_count == 0) {
        return -1; // 큐가 비어있을 때
    }
    int block_index = ssd->free_block_queue[ssd->free_block_front];
    ssd->free_block_front = (ssd->free_block_front + 1) % TotalBlocks;
    ssd->free_block_count--;
    remainFreeBlocks--;
    return block_index;
}

// SSD 초기화
void initial() {
    // 블록 배열을 할당합니다.
    blocks = (Block*)malloc(TotalBlocks * sizeof(Block));

    // 블록의 각 페이지 배열을 초기화합니다.
    for (int i = 0; i < TotalBlocks; i++) {
        blocks[i].pages = (Page*)malloc(PPB * sizeof(Page));
        blocks[i].freePageOffset = 0;
        blocks[i].validPageCount = 0;
        // 페이지 배열을 초기화합니다.
        memset(blocks[i].pages, 0, PPB * sizeof(Page));  // 페이지 유효성을 false로 초기화
    }
    
    // 자유 블록 큐를 초기화합니다.
    init_queue(&ssd);
    for (int i = 0; i < TotalBlocks; i++) {
        enqueue(&ssd, i);
    }

    // 매핑 테이블을 초기화합니다.
    mappingTable = (int*)malloc(LAB_NUM * sizeof(int));
    memset(mappingTable, -1, LAB_NUM * sizeof(int));  // 모든 LBA에 대해 -1로 초기화

    // OoBa 배열을 초기화합니다.
    OoBa = (int*)malloc(TotalPages * sizeof(int));
    memset(OoBa, -1, TotalPages * sizeof(int));  // 모든 페이지에 대해 -1로 초기화

    // 현재 활성 블록 및 페이지 설정
    current_active_block = dequeue(&ssd);
    if (current_active_block == -1) {
        fprintf(stderr, "No available blocks at initialization.\n");
        exit(EXIT_FAILURE);
    }
    current_active_page = 0;

    // 데이터 통계 초기화
    user_written_data = 0;
    gc_written_data = 0;
    progress_boundary = 8;
    cumulative_written_data = 0;
    cumulative_gc_written_data = 0;
    last_checkpoint_data = 0;
    last_checkpoint_gc_data = 0;
}

// 페이지 쓰기
void writePage(int LBA, bool GCWrite) {
    if (blocks[current_active_block].freePageOffset >= PPB) {
        current_active_block = dequeue(&ssd);
        if (current_active_block == -1) {
            fprintf(stderr, "No available blocks during write operation.\n");
            exit(EXIT_FAILURE);
        }
        blocks[current_active_block].freePageOffset = 0;
    }

    int old_physical_address = mappingTable[LBA];
    if (old_physical_address != -1) {
        int old_block_id = old_physical_address / PPB;
        int old_page_id = old_physical_address % PPB;
        if (blocks[old_block_id].pages[old_page_id].valid) {
            blocks[old_block_id].pages[old_page_id].valid = false;
            blocks[old_block_id].validPageCount--;
            utl--;  // 페이지가 유효하지 않게 되었으므로 감소
        }
    }

    blocks[current_active_block].pages[blocks[current_active_block].freePageOffset].valid = true;
    mappingTable[LBA] = current_active_block * PPB + blocks[current_active_block].freePageOffset;
    OoBa[current_active_block * PPB + blocks[current_active_block].freePageOffset] = LBA;
    blocks[current_active_block].freePageOffset++;
    blocks[current_active_block].validPageCount++;

    if (GCWrite) {
        gc_written_data++;
        cumulative_gc_written_data += PageSize;  // 누적 GC 데이터 양 업데이트
    } else {
        user_written_data++;
        cumulative_written_data += PageSize;  // 누적 데이터 양 업데이트
    }
    utl++;

    // 체크포인트에 도달했는지 확인
    if (cumulative_written_data >= GCBoundary) {
        last_checkpoint_data = cumulative_written_data;  // 마지막 체크포인트 데이터 양 업데이트
        last_checkpoint_gc_data = cumulative_gc_written_data;  // 마지막 체크포인트 GC 데이터 양 업데이트
        cumulative_written_data = 0;  // 누적 데이터 양 초기화
        cumulative_gc_written_data = 0;  // 누적 GC 데이터 양 초기화
    }
}

// 블록 제거
void removeBlock(int blockId) {
    for (int i = 0; i < PPB; i++) {
        if (blocks[blockId].pages[i].valid) {
            utl--;  // 페이지가 유효하지 않게 되므로 감소
        }
        blocks[blockId].pages[i].valid = false;
    }
    blocks[blockId].freePageOffset = 0;
    blocks[blockId].validPageCount = 0;
    enqueue(&ssd, blockId);
    erase_count++; // 블록 제거 시 ERASE 횟수 증가
}

// GC 알고리즘
int countValidPages(int blockId) {
    return blocks[blockId].validPageCount;
}

// GC 실행
void GC() {
    int victim_block = -1;
    int min_valid_pages = PPB + 1;  // 초기화: 최악의 경우로 설정

    // 활성 블록과 남아있는 블록을 제외한 블록 중 유효 페이지 수가 가장 적은 블록을 찾습니다.
    for (int i = 0; i < TotalBlocks; i++) {
        // 활성 블록과 남아있는 블록을 제외합니다.
        if (i == current_active_block || blocks[i].freePageOffset == 0) {
            continue;
        }

        int valid_pages = countValidPages(i);
        if (valid_pages < min_valid_pages) {
            min_valid_pages = valid_pages;
            victim_block = i;
        }
    }

    // 유효 페이지가 있는 블록을 찾은 경우 가비지 컬렉션을 수행합니다.
    if (victim_block != -1) {
        for (int i = 0; i < PPB; i++) {
            if (blocks[victim_block].pages[i].valid) {
                unsigned long lba = OoBa[victim_block * PPB + i];
                if (lba != (unsigned long)-1) {  // 페이지가 유효할 때만
                    writePage(lba, 1);  // 가비지 컬렉션 쓰기
                }
            }
        }
        removeBlock(victim_block);  // 블록 제거
    }
}

double calculateValidDataRatio() {
    unsigned long total_valid_pages = 0;
    unsigned int used_blocks = 0;

    for (int i = 0; i < TotalBlocks; i++) {
        if (blocks[i].validPageCount > 0) {
            total_valid_pages += blocks[i].validPageCount;
            used_blocks++;
        }
    }

    if (total_valid_pages == 0 || used_blocks == 0) return 0.0;
    return (double)total_valid_pages / (used_blocks * PPB);
}

void Statistics() {
    double waf = (double)(user_written_data + gc_written_data) / (double)user_written_data;
    double tmp_waf = (double)(last_checkpoint_data + last_checkpoint_gc_data) / (double)last_checkpoint_data;
    double utilization = (double)(utl) / (double)(LAB_NUM);
    double valid_data_ratio = calculateValidDataRatio();  // 유효 데이터 비율 계산

    printf("[Progress: %d GiB] WAF: %.3f, TMP_WAF: %.3f, Utilization: %.3f\n", progress_boundary, waf, tmp_waf, utilization);
    printf("GROUP 0[%d]: %.6f (ERASE: %lu)\n", TotalBlocks - remainFreeBlocks, valid_data_ratio, erase_count);
    
}

void processRequests(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }
    IORequest request;
    unsigned long processed_data = 0;
    while (fscanf(file, "%lf %d %lu %u %u", &request.timestamp, &request.io_type, &request.lba, &request.size, &request.stream_number) != EOF) {
        if (request.io_type == 1) {  // 실제 사용자 데이터 쓰기
            unsigned int num_pages = (request.size + PageSize - 1) / PageSize; // 페이지 수 계산
            for (unsigned int i = 0; i < num_pages; i++) {
                writePage(request.lba + i, 0);
                processed_data += PageSize;
            }
        }
        if (remainFreeBlocks < FreeBlockThreshold) {
            while (remainFreeBlocks < FreeBlockThreshold) {
                GC();
            }
        }
        if (processed_data >= GCBoundary) {
            Statistics();
            progress_boundary += 8;
            processed_data = 0;
        }
    }
    fclose(file);
}

int main() {
    initial();
    processRequests("test-fio-small");

    Statistics();

    // 메모리 해제
    for (int i = 0; i < TotalBlocks; i++) {
        free(blocks[i].pages);
    }
    free(blocks);
    free(mappingTable);
    free(OoBa);
    free(ssd.free_block_queue);

    return 0;
}
