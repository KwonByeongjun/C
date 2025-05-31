kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ g++ -Iinclude -Ilibs/rpi-rgb-led-matrix/include main.c src/server.c src/client.c src/board.c libs/cJSON.c -Llibs/rpi-rgb-led-matrix/lib -lrgbmatrix -lpthread -lrt -o hw3
/usr/bin/ld: /tmp/ccmXO6bs.o: in function `broadcast_json(cJSON const*)':
server.c:(.text+0x18c): undefined reference to `send_json(int, cJSON const*)'
/usr/bin/ld: /tmp/ccmXO6bs.o: in function `send_to_client(int, cJSON const*)':
server.c:(.text+0x1d0): undefined reference to `send_json(int, cJSON const*)'
/usr/bin/ld: /tmp/ccmXO6bs.o: in function `accept_and_register(int)':
server.c:(.text+0x4bc): undefined reference to `recv_json(int)'
/usr/bin/ld: /tmp/ccmXO6bs.o: in function `game_loop()':
server.c:(.text+0xb58): undefined reference to `isGameOver(char (*) [8])'
/usr/bin/ld: server.c:(.text+0xb88): undefined reference to `recv_json(int)'
/usr/bin/ld: server.c:(.text+0xcc0): undefined reference to `hasValidMove(char (*) [8], char)'
/usr/bin/ld: server.c:(.text+0xdb4): undefined reference to `isGameOver(char (*) [8])'
/usr/bin/ld: server.c:(.text+0xdf8): undefined reference to `isValidInput(char (*) [8], int, int, int, int)'
/usr/bin/ld: server.c:(.text+0xe4c): undefined reference to `isValidMove(char (*) [8], char, int, int, int, int)'
/usr/bin/ld: server.c:(.text+0xe88): undefined reference to `Move(char (*) [8], int, int, int, int, int)'
/usr/bin/ld: server.c:(.text+0xf74): undefined reference to `isGameOver(char (*) [8])'
/usr/bin/ld: server.c:(.text+0x1008): undefined reference to `countR(char (*) [8])'
/usr/bin/ld: server.c:(.text+0x1028): undefined reference to `countB(char (*) [8])'
/usr/bin/ld: /tmp/ccuslX7z.o: in function `client_run(char const*, char const*, char const*)':
client.c:(.text+0x628): undefined reference to `send_json(int, cJSON const*)'
/usr/bin/ld: client.c:(.text+0x688): undefined reference to `recv_json(int)'
/usr/bin/ld: client.c:(.text+0xbc8): undefined reference to `send_json(int, cJSON const*)'
collect2: error: ld returned 1 exit status
