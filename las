kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ g++ -Iinclude -Ilibs/rpi-rgb-led-matrix/include main.c src/server.c src/client.c src/board.c src/game.c src/json.c libs/cJSON.c -Llibs/rpi-rgb-led-matrix/lib -lrgbmatrix -lpthread -lrt -o hw3
src/json.c: In function ‘cJSON* recv_json(int)’:
src/json.c:29:31: error: invalid conversion from ‘void*’ to ‘char*’ [-fpermissive]
   29 |         char *newline = memchr(buf, '\n', buf_len);
      |                         ~~~~~~^~~~~~~~~~~~~~~~~~~~
      |                               |
      |                               void*
