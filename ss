compile


g++ -Iinclude -Ilibs/rpi-rgb-led-matrix/include main.c src/server.c src/client.c src/board.c src/game.c src/json.c libs/cJSON.c -Llibs/rpi-rgb-led-matrix/lib -lrgbmatrix -lpthread -lrt -o hw3

sudo ./hw3 --led-rows=64 --led-cols=64 --led-gpio-mapping=adafruit-hat --led-brightness=75 --led-chain=1 --led-no-hardware-pulse server -p 8080

sudo ./hw3 --led-rows=64 --led-cols=64 --led-gpio-mapping=adafruit-hat --led-brightness=75 --led-chain=1 --led-no-hardware-pulse client -i 127.0.0.1 -p 8080 -u SS
sudo ./hw3 --led-rows=64 --led-cols=64 --led-gpio-mapping=adafruit-hat --led-brightness=75 --led-chain=1 --led-no-hardware-pulse client -i 127.0.0.1 -p 8080 -u SA

