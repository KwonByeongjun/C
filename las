kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ g++ iinclude -Ilibs/rpi-rgb-led-matrix/include src/test_local_led.c src/board.c -Llibs/rpi.-rgb-led-matrix/lib -lrgbmatrix -lpthread -lrt -o test_local_led
/usr/bin/ld: cannot find iinclude: No such file or directory
/usr/bin/ld: cannot find -lrgbmatrix: No such file or directory
collect2: error: ld returned 1 exit status
