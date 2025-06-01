kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ ./hw3 client -i 127.0.0.1 -p 8080 -u sun
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

