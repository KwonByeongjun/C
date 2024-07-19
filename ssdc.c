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
#define LogicalSize (8L * 1000 * 1000 * 1000) // 논리 크기 (8GB)
#define GCBoundary (8L * 1000 * 1000 * 1000) // 8GB마다 통계 출력
#define FreeBlockThreshold 2 // GC 임계값 (2개 미만의 블록)
#define LAB_NUM (LogicalSize / PageSize) // LBA 수

typedef struct {
    bool valid;
    char data[PageSize]; // 데이터 배열
} Page;

typedef struct {
    Page pages[PPB];
    int freePageOffset; // 블록당 free page offset 유지
    int validPageCount; // 블록당 valid page 수 유지
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
int mappingTable[LAB_NUM]; // 논리 -> 물리 매핑 테이블
int OoBa[LAB_NUM]; // Out of Band area

int current_active_block;
int current_active_page;
unsigned long total_written_data = 0;
unsigned long gc_written_data = 0;
unsigned int tmp_erase_count = 0;
unsigned long tmp_written_data = 0;
unsigned int progress_boundary = 8;
int remainFreeBlocks = 0;

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
    }
    else {
        q->rear->next = temp;
        q->rear = temp;

    }
    remainFreeBlocks++;
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
    remainFreeBlocks--;
    return blockId;
}

// SSD 초기화
void initial() {
    for (int i = 0; i < TotalBlocks; i++) {
        for (int j = 0; j < PPB; j++) {
            blocks[i].pages[j].valid = false;
        }
        blocks[i].freePageOffset = 0;
        blocks[i].validPageCount = 0;
    }
    initQueue(&freeBlocks);
    for (int i = 0; i < TotalBlocks; i++) {
        enqueue(&freeBlocks, i);
    }
    for (int i = 0; i < LAB_NUM; i++) {
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

// 페이지 쓰기
void writePage(char *data, int LBA) {
    int new_page = blocks[current_active_block].freePageOffset;
    if (new_page >= PPB) {
        current_active_block = dequeue(&freeBlocks);
        blocks[current_active_block].freePageOffset = 0;
        current_active_page = 0;
        new_page = blocks[current_active_block].freePageOffset;
    }
    int old_physical_address = mappingTable[LBA];
    if (old_physical_address != -1) {
        int old_block_id = old_physical_address / PPB;
        int old_page_id = old_physical_address % PPB;
        if (blocks[old_block_id].pages[old_page_id].valid) {
            blocks[old_block_id].pages[old_page_id].valid = false;
            blocks[old_block_id].validPageCount--;
        }
    }
    blocks[current_active_block].pages[new_page].valid = true;
    mappingTable[LBA] = current_active_block * PPB + new_page;
    OoBa[current_active_block * PPB + new_page] = LBA;
    blocks[current_active_block].freePageOffset++;
    blocks[current_active_block].validPageCount++;
    current_active_page++;
    total_written_data += PageSize;
    tmp_written_data += PageSize;
}

// 블록 제거
void removeBlock(int blockId) {
    for (int i = 0; i < PPB; i++) {
        blocks[blockId].pages[i].valid = false;
    }
    blocks[blockId].freePageOffset = 0;
    blocks[blockId].validPageCount = 0;
    enqueue(&freeBlocks, blockId);
    tmp_erase_count++;
}

// GC 알고리즘
int countValidPages(int blockId) {
    return blocks[blockId].validPageCount;
}

// GC 실행
void GC() {
    int victim_block = -1;
    int min_valid_pages = PPB;
    for (int i = 0; i < TotalBlocks; i++) {
        if (blocks[i].validPageCount < min_valid_pages && blocks[i].validPageCount > 0) {
            min_valid_pages = blocks[i].validPageCount;
            victim_block = i;
        }
    }
    if (victim_block != -1) {
        for (int i = 0; i < PPB; i++) {
            if (blocks[victim_block].pages[i].valid) {
                int new_page = blocks[current_active_block].freePageOffset;
                if (new_page >= PPB) {
                    current_active_block = dequeue(&freeBlocks);
                    blocks[current_active_block].freePageOffset = 0;
                    current_active_page = 0;
                    new_page = blocks[current_active_block].freePageOffset;
                }
                writePage(blocks[victim_block].pages[i].data, OoBa[victim_block * PPB + i]);
                gc_written_data += PageSize;
            }
        }
        removeBlock(victim_block);
    }
}

void Statistics() {
    double waf = (double)(total_written_data + gc_written_data) / (double)total_written_data;
    double tmp_waf = (double)(tmp_written_data + gc_written_data) / (double)(progress_boundary * 1000 * 1000 * 1000);
    double utilization = (double)(LAB_NUM - freeBlocks.rear->blockId) / (double)LAB_NUM;
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
            writePage(data, request.lba); // WRITE
        } 
        else if (request.io_type == 3) { 
            removeBlock(request.lba / PPB); // TRIM
        }

        processed_data += request.size;

        if (remainFreeBlocks < FreeBlockThreshold) {
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


