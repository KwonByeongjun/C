/*
 * board.c
 *
 * 8×8 보드 문자열을 받아 64×64 LED 매트릭스를 갱신하는 데몬 프로그램.
 * 이 파일을 컴파일하면 실행 파일명은 "board"가 됩니다.
 *
 * 실제로는 C 소스이지만 내부적으로 C++ API(rpi-rgb-led-matrix)를 호출하므로
 * 컴파일 시 g++ 링커 옵션(-lstdc++ -lrgbmatrix 등)이 필요합니다.
 *
 * 컴파일 예:
 *   g++ -O2 -o board board.c -lstdc++ -lrgbmatrix -lpthread -lm
 *
 * 설치(옵션):
 *   sudo cp board /usr/local/bin/
 *   sudo chmod +x /usr/local/bin/board
 *
 * 실행 예:
 *   sudo board   (표준입력으로 8줄보드 수신 → 매트릭스 갱신)
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <signal.h>
 
 // rpi-rgb-led-matrix의 C++ API 헤더
 #include "rgbmatrix.h"
 #include "graphics.h"
 
 // 보드 크기: 8×8
 #define BOARD_SIZE 8
 
 // LED 매트릭스 픽셀 크기
 #define MATRIX_ROWS 64
 #define MATRIX_COLS 64
 
 // 셀 하나의 크기 (8)
 #define CELL_COUNT BOARD_SIZE
 #define CELL_SIZE (MATRIX_ROWS / CELL_COUNT)   // = 8
 #define INNER_SIZE (CELL_SIZE - 2)             // = 6 (테두리 1픽셀 마진)
 
 /*
  * SIGINT/SIGTERM 시 안전하게 종료할 함수
  */
 static void cleanup_and_exit(int signum) {
     // 단순히 프로세스 종료
     (void)signum;
     exit(0);
 }
 
 int main(int argc, char *argv[]) {
     // 이 프로그램은 따로 인자를 사용하지 않습니다.
     (void)argc; (void)argv;
 
     // SIGINT, SIGTERM을 받으면 cleanup_and_exit 호출
     signal(SIGINT, cleanup_and_exit);
     signal(SIGTERM, cleanup_and_exit);
 
     // --------------------------------------------
     // (1) rpi-rgb-led-matrix 초기화
     // --------------------------------------------
     rgb_matrix::RGBMatrix::Options options;
     options.rows = MATRIX_ROWS;
     options.cols = MATRIX_COLS;
     options.chain_length = 1;
     options.parallel = 1;
     options.hardware_mapping = "adafruit-hat";  // 필요 시 "regular" 등으로 변경
     options.pwm_bits = 11;
     options.brightness = 75;
     options.gpio_slowdown = 1;  // Pi3 이상이면 1~2
 
     rgb_matrix::RGBMatrix *matrix = new rgb_matrix::RGBMatrix(&options);
     if (!matrix) {
         fprintf(stderr, "board: RGBMatrix 초기화 실패\n");
         return 1;
     }
     rgb_matrix::FrameCanvas *canvas = matrix->CreateFrameCanvas();
 
     // 색상 정의
     rgb_matrix::Color BLACK = rgb_matrix::Color(0,   0,   0);
     rgb_matrix::Color WHITE = rgb_matrix::Color(255, 255, 255);
     rgb_matrix::Color RED   = rgb_matrix::Color(255,   0,   0);
     rgb_matrix::Color BLUE  = rgb_matrix::Color(0,     0, 255);
     rgb_matrix::Color GRAY  = rgb_matrix::Color(128, 128, 128);
 
     // 8×8 문자열 보드를 읽어서 그릴 버퍼
     char board[BOARD_SIZE][BOARD_SIZE + 1];  // 8문자 + NULL
 
     // --------------------------------------------
     // (2) 무한 루프: stdin으로 8줄씩 읽을 때마다 화면 갱신
     // --------------------------------------------
     while (1) {
         // (2-1) 표준입력으로 8줄 읽기
         for (int i = 0; i < BOARD_SIZE; i++) {
             if (fgets(board[i], sizeof(board[i]), stdin) == NULL) {
                 // EOF 또는 에러가 발생한 경우, 잠시 대기 후 재시도
                 usleep(100000);
                 i = -1;  // 다시 0부터 읽기
                 continue;
             }
             // 개행(\n) 또는 \r 제거
             size_t len = strlen(board[i]);
             if (len > 0 && (board[i][len-1] == '\n' || board[i][len-1] == '\r')) {
                 board[i][len-1] = '\0';
                 len--;
             }
             // 만약 길이가 8이 아니면, 해당 줄을 다시 읽기
             if (len != BOARD_SIZE) {
                 i = -1;
                 continue;
             }
         }
 
         // (2-2) 화면을 검은색으로 초기화
         canvas->Fill(BLACK.r, BLACK.g, BLACK.b);
 
         // (2-3) 8×8 셀 경계(격자) 그리기 (1픽셀 흰선)
         for (int k = 0; k <= CELL_COUNT; k++) {
             int pos = k * CELL_SIZE;  // 0, 8, 16, ..., 64
             // 수평선 그리기 (y = pos)
             for (int x = 0; x < MATRIX_COLS; x++) {
                 canvas->SetPixel(x, pos, WHITE.r, WHITE.g, WHITE.b);
             }
             // 수직선 그리기 (x = pos)
             for (int y = 0; y < MATRIX_ROWS; y++) {
                 canvas->SetPixel(pos, y, WHITE.r, WHITE.g, WHITE.b);
             }
         }
 
         // (2-4) 각 셀 내부에 기물 또는 장애물 그리기
         for (int row = 0; row < BOARD_SIZE; row++) {
             for (int col = 0; col < BOARD_SIZE; col++) {
                 char c = board[row][col];
                 rgb_matrix::Color colr = BLACK;
                 if      (c == 'R') colr = RED;   // 백혈구(빨강)
                 else if (c == 'B') colr = BLUE;  // 세균(파랑)
                 else if (c == '#') colr = GRAY;  // 장애물(회색)
                 else continue;  // '.' 이면 빈칸 → 아무것도 그리지 않음
 
                 // 셀 내부 시작 좌표: (col*8+1, row*8+1)
                 int base_x = col * CELL_SIZE + 1;
                 int base_y = row * CELL_SIZE + 1;
                 // 6×6 영역을 colr 색으로 채우기
                 for (int dx = 0; dx < INNER_SIZE; dx++) {
                     for (int dy = 0; dy < INNER_SIZE; dy++) {
                         canvas->SetPixel(base_x + dx, base_y + dy,
                                          colr.r, colr.g, colr.b);
                     }
                 }
             }
         }
 
         // (2-5) 화면 스왑 (Double Buffering)
         canvas = matrix->SwapOnVSync(canvas);
     }
 
     // 이 지점에는 절대 도달하지 않습니다.
     return 0;
 }
 
