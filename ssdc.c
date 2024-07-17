'''
Simulation setup

Device size: 8GiB
Logical size: 8GB (8GiB-8GB: OP)
Page size: 4KB
Block size: 4MB
GC threshold: less than 2 free blocks

Output: 출력 해야하는 정보

8GB 마다:
[Progress: 8GiB] WAF: 1.012, TMP_WAF: 1.024, Utilization: 1.000
GROUP 0[2046]: 0.02 (ERASE: 1030)

TMP_WAF: 0~50GiB, 50GiB~100GiB 동안의 WAF
WAF: 지금까지의 WAF
Utilization: LBA 기준으로 지금 얼마만큼 쓰였는가
GROUP 0[Free block을 제외한 사용된 블록의 개수]: Valid data ratio on GC (50GiB) (50GiB 동안 발생한 ERASE 횟수)
'''


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define PageSize 4096 // 페이지 크기 (4KB)
#define PPB 1024 // 블록당 페이지 수
#define TotalBlocks 2048 // 전체 블록 수 (8GiB / 4MB)
#define TotalPages (TotalBlocks * PPB) // 전체 페이지 수
#define BlockSize (PageSize * PPB) // 블록 크기 (4MB)
#define DeviceSize (TotalBlocks * BlockSize) // 디바이스 크기 (8GiB)
#define LogicalSize (8L * 1024 * 1024 * 1024) // 논리 크기 (8GB)
#define GCBoundary (8L * 1024 * 1024 * 1024) // 8GB마다 통계 출력
#define FreeBlockThreshold 2 // GC 임계값 (2개 미만의 블록)

typedef struct {
    bool valid;
    char data[PageSize]; // 데이터 배열
} Page;

typedef struct {
    Page pages[PPB];
} Block;

typedef struct Node {
    int blockId;
    struct Node *next;
} Node;

typedef struct {
    Node *front;
    Node *rear;
} Queue;

typedef struct {
    unsigned int timestamp;
    int io_type;
    unsigned long lba;
    unsigned int size;
    unsigned int stream_number;
} IORequest;

// 글로벌 변수들
Block blocks[TotalBlocks];
Queue freeBlocks;
int mappingTable[TotalPages]; // 논리 -> 물리 매핑 테이블
int OoBa[TotalPages]; // Out of Band area

int current_active_block;
int current_active_page;
unsigned long total_written_data = 0;
unsigned long gc_written_data = 0;
unsigned int tmp_erase_count = 0;
unsigned long tmp_written_data = 0;
unsigned int progress_boundary = 8;

// Queue 함수들
void initQueue(Queue *q) {
    q->front = q->rear = NULL;
}

void enqueue(Queue *q, int blockId) {
    Node *temp = (Node*)malloc(sizeof(Node));
    temp->blockId = blockId;
    temp->next = NULL;
    if (q->rear == NULL) {
        q->front = temp;
        q->rear = temp;
        return;
    }
    q->rear->next = temp;
    q->rear = temp;
}

int dequeue(Queue *q) {
    if (q->front == NULL) {
        return -1;
    }
    Node *temp = q->front;
    int blockId = temp->blockId;
    q->front = q->front->next;
    if (q->front == NULL) {
        q->rear = NULL;
    }
    free(temp);
    return blockId;
}

// SSD 초기화
void initial() {
    for (int i = 0; i < TotalBlocks; i++) {
        for (int j = 0; j < PPB; j++) {
            blocks[i].pages[j].valid = false;
        }
    }
    initQueue(&freeBlocks);
    for (int i = 0; i < TotalBlocks; i++) {
        enqueue(&freeBlocks, i);
    }
    for (int i = 0; i < TotalPages; i++) {
        mappingTable[i] = -1;
        OoBa[i] = -1;
    }
    current_active_block = dequeue(&freeBlocks);
    current_active_page = 0;
    total_written_data = 0;
    gc_written_data = 0;
    tmp_erase_count = 0;
    tmp_written_data = 0;
    progress_boundary = 8;
}

// 유효한 페이지 찾기
int findFreePage(int blockId) {
    for (int i = 0; i < PPB; i++) {
        if (!blocks[blockId].pages[i].valid) {
            return i;
        }
    }
    return -1;
}

// 페이지 쓰기
void writePage(int blockId, int pageId, char *data, int LBA) {
    int new_page = findFreePage(current_active_block);
    if (new_page == -1) {
        current_active_block = dequeue(&freeBlocks);
        current_active_page = 0;
        new_page = findFreePage(current_active_block);
    }
    int old_physical_address = mappingTable[LBA];
    if (old_physical_address != -1) {
        int old_block_id = old_physical_address / PPB;
        int old_page_id = old_physical_address % PPB;
        if (blocks[old_block_id].pages[old_page_id].valid) {
            blocks[old_block_id].pages[old_page_id].valid = false;
        }
    }
    blocks[blockId].pages[pageId].valid = true;
    mappingTable[LBA] = current_active_block * PPB + new_page;
    OoBa[current_active_block * PPB + new_page] = LBA;
    current_active_page++;
    total_written_data += PageSize;
    tmp_written_data += PageSize;
}

// 블록 제거
void removeBlock(int blockId) {
    for (int i = 0; i < PPB; i++) {
        blocks[blockId].pages[i].valid = false;
    }
    enqueue(&freeBlocks, blockId);
    tmp_erase_count++;
}

// GC 알고리즘
int countValidPages(int blockId) {
    int valid_page_count = 0;
    for (int i = 0; i < PPB; i++) {
        if (blocks[blockId].pages[i].valid) {
            valid_page_count++;
        }
    }
    return valid_page_count;
}

// GC 실행
void GC() {
    int victim_block = -1;
    int min_valid_pages = PPB;
    for (int i = 0; i < TotalBlocks; i++) {
        int valid_pages = countValidPages(i);
        if (valid_pages < min_valid_pages) {
            min_valid_pages = valid_pages;
            victim_block = i;
        }
    }
    if (victim_block != -1) {
        for (int i = 0; i < PPB; i++) {
            if (blocks[victim_block].pages[i].valid) {
                int new_page = findFreePage(current_active_block);
                if (new_page == -1) {
                    current_active_block = dequeue(&freeBlocks);
                    current_active_page = 0;
                    new_page = findFreePage(current_active_block);
                }
                char copy_data[PageSize];
                memcpy(copy_data, blocks[victim_block].pages[i].data, PageSize);
                writePage(current_active_block, new_page, copy_data, OoBa[victim_block * PPB + i]);
                gc_written_data += PageSize;
            }
        }
        removeBlock(victim_block);
    }
}


void Statistics() {
    double waf = (double)total_written_data / (double)LogicalSize;
    double tmp_waf = (double)tmp_written_data / (double)(progress_boundary * 1024 * 1024 * 1024);
    double utilization = (double)total_written_data / (double)DeviceSize;
    int used_blocks = TotalBlocks - freeBlocks.rear->blockId - 1;
    double valid_data_ratio = (double)gc_written_data / (double)(tmp_erase_count * BlockSize);

    printf("[Progress: %dGiB] WAF: %.3f, TMP_WAF: %.3f, Utilization: %.3f\n", progress_boundary, waf, tmp_waf, utilization);
    printf("GROUP 0[%d]: %.2f (ERASE: %d)\n", used_blocks, valid_data_ratio, tmp_erase_count);

    tmp_written_data = 0;
    tmp_erase_count = 0;
    gc_written_data = 0;
}

void processRequests(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    IORequest request;
    unsigned long processed_data = 0;

    while (fscanf(file, "%lf %d %lu %u %u", &request.timestamp, 
                  &request.io_type, &request.lba, 
                  &request.size, &request.stream_number) != EOF) {
        if (request.io_type == 0) { 
            // READ 생략
        }
        else if (request.io_type == 1) { 
            char data[PageSize] = {0}; 
            writePage(current_active_block, current_active_page, data, request.lba); // WRITE
        } 
        else if (request.io_type == 3) { 
            removeBlock(request.lba / PPB); // TRIM
        }

        processed_data += request.size;

        if (freeBlocks.rear == NULL || freeBlocks.rear->blockId < FreeBlockThreshold) {
            GC();
        }

        if (processed_data >= GCBoundary) {
            Statistics();
            progress_boundary += 8;
            processed_data = 0;
        }
    }
    fclose(file);
}

// 메인 함수
int main() {
    initial();
    processRequests("test-fio-small");
    Statistics(); 
    return 0;
}
