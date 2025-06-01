kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ g++ -Iinclude -Ilibs/rpi-rgb-led-matrix/include main.c src/server.c src/client.c src/board.c src/game.c src/json.c libs/cJSON.c -Llibs/rpi-rgb-led-matrix/lib -lrgbmatrix -lpthread -lrt -o hw3

kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ sudo ./hw3 --led-rows=64 --led-cols=64 --led-gpio-mapping=adafruit-hat --led-brightness=75 --led-chain=1 --led-no-hardware-pulse server -p 8080
Suggestion: to slightly improve display update, add
	isolcpus=3
at the end of /boot/cmdline.txt and reboot (see README.md)
LED Matrix Initialized.
[main] init_led_matrix() 성공
[main] init_led_matrix 이후 남은 인자 (argc=4):
  argv[0] = './hw3'
  argv[1] = 'server'
  argv[2] = '-p'
  argv[3] = '8080'
[main] 서버 모드로 실행 (port=8080)
Server started on port 8080


kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ sudo ./hw3 --led-rows=64 --led-cols=64 --led-gpio-mapping=adafruit-hat --led-brightness=75 --led-chain=1 --led-no-hardware-pulse client -i 10.8.128.233 -p 8080 -u SS
Suggestion: to slightly improve display update, add
	isolcpus=3
at the end of /boot/cmdline.txt and reboot (see README.md)
LED Matrix Initialized.
[main] init_led_matrix() 성공
[main] init_led_matrix 이후 남은 인자 (argc=8):
  argv[0] = './hw3'
  argv[1] = 'client'
  argv[2] = '-i'
  argv[3] = '10.8.128.233'
  argv[4] = '-p'
  argv[5] = '8080'
  argv[6] = '-u'
  argv[7] = 'SS'
[main] 클라이언트 모드로 실행 (ip=10.8.128.233, port=8080, username=SS)
Failed to connect to 10.8.128.233:8080
Failed to connect to 10.8.128.233:8080
LED Matrix closed.

kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ sudo ./hw3 --led-rows=64 --led-cols=64 --led-gpio-mapping=adafruit-hat --led-brightness=75 --led-chain=1 --led-no-hardware-pulse client -i 10.8.128.233 -p 8080 -u SA
Suggestion: to slightly improve display update, add
	isolcpus=3
at the end of /boot/cmdline.txt and reboot (see README.md)
LED Matrix Initialized.
[main] init_led_matrix() 성공
[main] init_led_matrix 이후 남은 인자 (argc=8):
  argv[0] = './hw3'
  argv[1] = 'client'
  argv[2] = '-i'
  argv[3] = '10.8.128.233'
  argv[4] = '-p'
  argv[5] = '8080'
  argv[6] = '-u'
  argv[7] = 'SA'
[main] 클라이언트 모드로 실행 (ip=10.8.128.233, port=8080, username=SA)


