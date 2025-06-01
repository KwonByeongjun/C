kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ g++ -Iinclude -Ilibs/rpi-rgb-led-matrix/include main.c src/server.c src/client.c src/board.c src/game.c src/json.c libs/cJSON.c -Llibs/rpi-rgb-led-matrix/lib -lrgbmatrix -lpthread -lrt -o hw3
main.c: In function ‘int main(int, char**)’:
main.c:35:30: error: invalid conversion from ‘int’ to ‘const char*’ [-fpermissive]
   35 |         int ret = server_run(port);
      |                              ^~~~
      |                              |
      |                              int
In file included from main.c:4:
include/server.h:26:28: note:   initializing argument 1 of ‘int server_run(const char*)’
   26 | int server_run(const char *port);
      |                ~~~~~~~~~~~~^~~~
main.c:59:31: error: invalid conversion from ‘int’ to ‘const char*’ [-fpermissive]
   59 |         return client_run(ip, port, username);
      |                               ^~~~
      |                               |
      |                               int
In file included from main.c:5:
include/client.h:8:44: note:   initializing argument 2 of ‘int client_run(const char*, const char*, const char*)’
    8 | int client_run(const char *ip, const char *port, const char *username);
