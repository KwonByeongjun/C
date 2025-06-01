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




kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ ./hw3 client -i 10.8.128.233 -p 8080 -u SS
Suggestion: to slightly improve display update, add
	isolcpus=3
at the end of /boot/cmdline.txt and reboot (see README.md)
Need root. You are configured to use the hardware pulse generator for
	smooth color rendering, however the necessary hardware
	registers can't be accessed because you probably don't run
	with root permissions or privileges have been dropped.
	So you either have to run as root (e.g. using sudo) or
	supply the --led-no-hardware-pulse command-line flag.

	Exiting; run as root or with --led-no-hardware-pulse
