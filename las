kwon@raspberrypi:~/real_real/System-Programming-OctaFlip/hw3_202311160 $ sudo ./hw3 server -p 8080 -- --led-rows=64 --led-cols=64 --led-gpio-mapping=adafruit-hat --led-brightness=75 --led-chain=1 --led-no-hardware-pulse
Suggestion: to slightly improve display update, add
	isolcpus=3
at the end of /boot/cmdline.txt and reboot (see README.md)

=== snd_bcm2835: found that the Pi sound module is loaded. ===
Don't use the built-in sound of the Pi together with this lib; it is known to be
incompatible and cause trouble and hangs (you can still use external USB sound adapters).

See Troubleshooting section in README how to disable the sound module.
You can also run with --led-no-hardware-pulse to avoid the incompatibility,
but you will have more flicker.
Exiting; fix the above first or use --led-no-hardware-pulse
