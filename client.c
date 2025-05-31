/*
 * client.c
 *
 * OctaFlip 서버와 TCP/JSON 통신을 수행하며, “your_turn” 메시지를 받으면:
 *   1) 8×8 보드 파싱 → LED 매트릭스에 그리기 (draw_board_daemon 호출)
 *   2) Greedy AI 로직 실행 → 다음 수 계산
 *   3) 서버로 move JSON 전송
 *
 * 내부적으로는 “board”라는 실행 파일을 fork+exec 하여 LED 매트릭스 갱신 데몬을 생성하고,
 * 파이프로 8×8 보드 데이터를 전달합니다.
 *
 * 컴파일 예:
 *   gcc -O2 -o client client.c -lcjson
 *
 * 실행 예:
 *   sudo ./client -ip <서버_IP> -port <포트> -username <이름>
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <signal.h>
 #include <time.h>
 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <sys/types.h>
 #include <sys/wait.h>
 #include <fcntl.h>
 #include "cJSON.h"
 
 #define SIZE 8
 #define PIPE_READ  0
 #define PIPE_WRITE 1
 
 // 전역 변수: LED 데몬(forked process)의 PID 및 파이프 FD
 static pid_t board_daemon_pid = -1;
 static int board_pipe_fd[2] = { -1, -1 };
 
 // SIGPIPE 무시 (파이프 깨져도 프로세스가 죽지 않도록)
 static void sigpipe_handler(int signum) {
     (void)signum;
 }
 
 // 프로토타입 선언 (이 파일 내부의 static 함수와 AI 함수)
 static void init_board_daemon(void);
 static void draw_board_daemon(char board[SIZE][SIZE]);
 static void greedy_move_generate(char board[SIZE][SIZE], char my_color,
                                   int *r1, int *c1, int *r2, int *c2);
 
 // --------------------------------------------------------------------------------------
 // main: OctaFlip 클라이언트
 // --------------------------------------------------------------------------------------
 int main(int argc, char *argv[]) {
     if (argc != 7) {
         fprintf(stderr, "Usage: %s -ip <server_ip> -port <port> -username <name>\n", argv[0]);
         return 1;
     }
 
     char ip[64]       = {0};
     int port          = 0;
     char username[64] = {0};
 
     // 인자 파싱
     for (int i = 1; i < argc; i += 2) {
         if      (strcmp(argv[i], "-ip") == 0)        strncpy(ip,       argv[i+1], sizeof(ip)-1);
         else if (strcmp(argv[i], "-port") == 0)      port = atoi(argv[i+1]);
         else if (strcmp(argv[i], "-username") == 0)  strncpy(username, argv[i+1], sizeof(username)-1);
         else {
             fprintf(stderr, "Usage: %s -ip <server_ip> -port <port> -username <name>\n", argv[0]);
             return 1;
         }
     }
     if (ip[0] == '\0' || port <= 0 || username[0] == '\0') {
         fprintf(stderr, "Usage: %s -ip <server_ip> -port <port> -username <name>\n", argv[0]);
         return 1;
     }
 
     // SIGPIPE 무시
     signal(SIGPIPE, sigpipe_handler);
 
     // 1) 서버 연결
     int sockfd;
     struct sockaddr_in serv_addr;
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) {
         perror("socket 생성 실패");
         return 1;
     }
     memset(&serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_port   = htons(port);
     if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
         fprintf(stderr, "IP 주소 변환 실패: %s\n", ip);
         close(sockfd);
         return 1;
     }
     if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
         perror("서버 연결 실패");
         close(sockfd);
         return 1;
     }
 
     // 2) 서버에 register 메시지 전송
     cJSON *req = cJSON_CreateObject();
     cJSON_AddStringToObject(req, "type", "register");
     cJSON_AddStringToObject(req, "username", username);
     char *req_str = cJSON_PrintUnformatted(req);
     write(sockfd, req_str, strlen(req_str));
     write(sockfd, "\n", 1);
     free(req_str);
     cJSON_Delete(req);
 
     // 3) LED 데몬 초기화 (board 실행 파일 fork+exec)
     init_board_daemon();
 
     // 4) 메인 루프: 서버 메시지 처리
     while (1) {
         // (4-1) recv_json: 한 줄(\n) 단위로 읽어 cJSON 객체로 파싱
         static char buffer[8192];
         int idx = 0;
         while (1) {
             char ch;
             int n = read(sockfd, &ch, 1);
             if (n <= 0) {
                 fprintf(stderr, "서버 연결이 끊어졌습니다.\n");
                 close(sockfd);
                 return 0;
             }
             if (ch == '\n') {
                 buffer[idx] = '\0';
                 break;
             }
             if (idx < (int)sizeof(buffer)-1) buffer[idx++] = ch;
         }
         cJSON *msg = cJSON_Parse(buffer);
         if (!msg) {
             // JSON 파싱 실패
             continue;
         }
 
         cJSON *type = cJSON_GetObjectItemCaseSensitive(msg, "type");
         if (!cJSON_IsString(type)) {
             cJSON_Delete(msg);
             continue;
         }
 
         // (4-2) your_turn 메시지 처리
         if (strcmp(type->valuestring, "your_turn") == 0) {
             // (a) board 배열 파싱
             cJSON *jboard = cJSON_GetObjectItemCaseSensitive(msg, "board");
             char board8x8[SIZE][SIZE];
             for (int i = 0; i < SIZE; i++) {
                 cJSON *row = cJSON_GetArrayItem(jboard, i);
                 const char *str = row->valuestring;
                 for (int j = 0; j < SIZE; j++) {
                     board8x8[i][j] = str[j];
                 }
             }
 
             // (b) LED 매트릭스 갱신 (board 데몬으로 파이프 전송)
             draw_board_daemon(board8x8);
 
             // (c) AI 로직: Greedy (백혈구는 'W')
             int r1, c1, r2, c2;
             greedy_move_generate(board8x8, 'W', &r1, &c1, &r2, &c2);
 
             // (d) 서버로 move 전송 (1-based 인덱스)
             cJSON_Delete(msg);
             cJSON *mv = cJSON_CreateObject();
             cJSON_AddStringToObject(mv, "type", "move");
             cJSON_AddStringToObject(mv, "username", username);
             if (r1 < 0) {
                 // PASS
                 cJSON_AddNumberToObject(mv, "sx", 0);
                 cJSON_AddNumberToObject(mv, "sy", 0);
                 cJSON_AddNumberToObject(mv, "tx", 0);
                 cJSON_AddNumberToObject(mv, "ty", 0);
             } else {
                 cJSON_AddNumberToObject(mv, "sx", r1+1);
                 cJSON_AddNumberToObject(mv, "sy", c1+1);
                 cJSON_AddNumberToObject(mv, "tx", r2+1);
                 cJSON_AddNumberToObject(mv, "ty", c2+1);
             }
             char *mv_str = cJSON_PrintUnformatted(mv);
             write(sockfd, mv_str, strlen(mv_str));
             write(sockfd, "\n", 1);
             free(mv_str);
             cJSON_Delete(mv);
         }
         // (4-3) 기타 메시지 처리 (move_ok, invalid_move, pass, game_over)
         else if (strcmp(type->valuestring, "game_over") == 0) {
             cJSON_Delete(msg);
             break;
         } else {
             // move_ok, invalid_move, pass 등은 필요 시 로그만 찍고 무시
             cJSON_Delete(msg);
             continue;
         }
     }
 
     close(sockfd);
     return 0;
 }
 
 // --------------------------------------------------------------------------------------
 // init_board_daemon: LED 데몬(board)를 fork+exec 하는 함수
 //   - board 데몬은 board.c로 컴파일된 실행 파일 "board"를 의미합니다.
 //   - 파이프(board_pipe_fd)를 통해 부모 → 자식 (board 데몬)으로 8×8 보드 데이터를 보냅니다.
 // --------------------------------------------------------------------------------------
 static void init_board_daemon(void)
 {
     if (board_daemon_pid > 0) return;  // 이미 init 되었다면 무시
 
     // SIGPIPE 무시
     signal(SIGPIPE, sigpipe_handler);
 
     // 파이프 생성
     if (pipe(board_pipe_fd) < 0) {
         perror("init_board_daemon: pipe 생성 실패");
         exit(1);
     }
 
     pid_t pid = fork();
     if (pid < 0) {
         perror("init_board_daemon: fork 실패");
         exit(1);
     }
     else if (pid == 0) {
         // 자식 프로세스: board 실행 파일을 stdin으로 pipe_fd[0]을 연결 후 execlp
         close(board_pipe_fd[1]);  // 쓰기용은 부모만 사용
         dup2(board_pipe_fd[0], STDIN_FILENO);
         close(board_pipe_fd[0]);
         execlp("board", "board", NULL);
         // 실행 실패 시
         perror("init_board_daemon: execlp 실패 (board 실행)");
         exit(1);
     }
     else {
         // 부모 프로세스: 자식 PID 저장, 읽기용 FD 닫기
         board_daemon_pid = pid;
         close(board_pipe_fd[0]);
         // board_pipe_fd[1]는 draw_board_daemon() 호출 시 사용
     }
 }
 
 // --------------------------------------------------------------------------------------
 // draw_board_daemon: 8×8 보드 데이터를 파이프로 board 데몬에 전달하여 
 //                   LED 매트릭스를 갱신하도록 하는 함수
 // --------------------------------------------------------------------------------------
 static void draw_board_daemon(char board[SIZE][SIZE])
 {
     if (board_daemon_pid < 0 || board_pipe_fd[1] < 0) {
         // init_board_daemon이 안 되어 있으면 먼저 초기화
         init_board_daemon();
     }
     // 부모 프로세스: pipe에 8줄(각각 8문자 + '\n')을 출력
     FILE *fp = fdopen(board_pipe_fd[1], "w");
     if (!fp) {
         perror("draw_board_daemon: fdopen 실패");
         return;
     }
     for (int i = 0; i < SIZE; i++) {
         fwrite(board[i], 1, SIZE, fp);
         fputc('\n', fp);
     }
     fflush(fp);
     // 파이프를 닫지 않고 재사용 (데몬은 무한 루프 중이므로)
 }
 
 // --------------------------------------------------------------------------------------
 // Greedy AI 로직: 가능한 모든 이동을 평가하여 가장 점수가 높은 수를 반환
 // --------------------------------------------------------------------------------------
 static void greedy_move_generate(char board[SIZE][SIZE], char my_color,
                                  int *r1, int *c1, int *r2, int *c2)
 {
     char opp = (my_color == 'W' ? 'B' : 'W');
     int best_score = -999999;
     int best_sr=-1, best_sc=-1, best_tr=-1, best_tc=-1;
     int center = (SIZE - 1) / 2;  // = 3
 
     int d8[8][2] = {
         {-1,-1}, {-1,0}, {-1,1},
         { 0,-1},         { 0,1},
         { 1,-1}, { 1,0}, { 1,1}
     };
 
     for (int sr = 0; sr < SIZE; sr++) {
         for (int sc = 0; sc < SIZE; sc++) {
             if (board[sr][sc] != my_color) continue;
             for (int dr = -2; dr <= 2; dr++) {
                 for (int dc = -2; dc <= 2; dc++) {
                     if (dr == 0 && dc == 0) continue;
                     int tr = sr + dr, tc = sc + dc;
                     if (tr < 0 || tr >= SIZE || tc < 0 || tc >= SIZE) continue;
                     if (board[tr][tc] != '.') continue;
                     int dist = abs(dr) > abs(dc) ? abs(dr) : abs(dc);
                     if (dist > 2) continue;
 
                     // 전환 개수 계산
                     int convert = 0;
                     for (int k = 0; k < 8; k++) {
                         int nr = tr + d8[k][0], nc = tc + d8[k][1];
                         if (nr < 0 || nr >= SIZE || nc < 0 || nc >= SIZE) continue;
                         if (board[nr][nc] == opp) convert++;
                     }
 
                     int score = convert * 10;
                     if (dist == 1) score += 5; else score -= 2;
                     int manh = abs(tr - center) + abs(tc - center);
                     if (manh <= 2) score += 3;
 
                     if (score > best_score) {
                         best_score = score;
                         best_sr = sr; best_sc = sc;
                         best_tr = tr; best_tc = tc;
                     }
                 }
             }
         }
     }
 
     if (best_score < 0) {
         *r1 = -1;  // PASS
     } else {
         *r1 = best_sr; *c1 = best_sc;
         *r2 = best_tr; *c2 = best_tc;
     }
 }
 
