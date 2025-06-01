kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ sudo ./hw3 server -p 8080 --led-rows=64 --led-cols=64 --led-gpio-mapping=adafruit-hat --led-brightness=75 --led-chain=1 --led-no-hardware-pulse
Suggestion: to slightly improve display update, add
	isolcpus=3
at the end of /boot/cmdline.txt and reboot (see README.md)
LED Matrix Initialized.
LED Matrix Initialized.
Usage:
  ./hw3 server [-p port]
  ./hw3 client -i ip -p port -u username
LED Matrix closed.
