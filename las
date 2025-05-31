kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ g++ -Iinclude -Ilibs/rpi-rgb-led-matrix/include main.c src/board.c libs/cJSON.c -Llibs/rpi-rgb-led-matrix/lib -lrgbmatrix -lpthread -lrt -o hw3
/usr/bin/ld: /tmp/cc7POEXR.o: in function `main':
main.c:(.text+0x124): undefined reference to `server_run(char const*)'
/usr/bin/ld: main.c:(.text+0x274): undefined reference to `client_run(char const*, char const*, char const*)'
collect2: error: ld returned 1 exit status
