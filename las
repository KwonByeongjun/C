kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ g++ -Iinclude -Ilibs/rpi-rgb-led-matrix/include main.c src/server.c src/client.c src/board.c src/game.c src/json.c libs/cJSON.c -Llibs/rpi-rgb-led-matrix/lib -lrgbmatrix -lpthread -lrt -o hw3
src/board.c: In function ‘int init_led_matrix(int, char**)’:
src/board.c:59:55: error: invalid conversion from ‘int’ to ‘int*’ [-fpermissive]
   59 |     matrix = led_matrix_create_from_options(&options, argc, argv);
      |                                                       ^~~~
      |                                                       |
      |                                                       int
src/board.c:59:61: error: cannot convert ‘char**’ to ‘char***’
   59 |     matrix = led_matrix_create_from_options(&options, argc, argv);
      |                                                             ^~~~
      |                                                             |
      |                                                             char**
In file included from src/board.c:1:
src/../libs/rpi-rgb-led-matrix/include/led-matrix-c.h:246:70: note:   initializing argument 3 of ‘RGBLedMatrix* led_matrix_create_from_options(RGBLedMatrixOptions*, int*, char***)’
  246 |          struct RGBLedMatrixOptions *options, int *argc, char ***argv);
      |                                                          ~~~~~~~~^~~~

