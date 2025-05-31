g++ -Iinclude -Ilibs/rpi-rgb-led-matrix/include main.c src/board.c libs/cJSON.c -Llibs/rpi-rgb-led-matrix/lib -lrgbmatrix -lpthread -lrt -o hw3
/usr/bin/ld: cannot find -lrgbmatrix: No such file or directory
collect2: error: ld returned 1 exit status
