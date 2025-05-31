kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ g++ -Iinclude -Ilibs/rpi-rgb-led-matrix/include main.c src/server.c src/client.c src/board.c libs/cJSON.c -Llibs/rpi-rgb-led-matrix/lib -lrgbmatrix -lpthread -lrt -o hw3
src/server.c: In function ‘void game_loop()’:
src/server.c:192:5: error: ‘update_led_matrix’ was not declared in this scope
  192 |     update_led_matrix(game.board);
      |     ^~~~~~~~~~~~~~~~~
