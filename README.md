# ArduinoBluetooth
A sketch to interface from an Arduino (e.g. nano) to a Bluetooth device (e.g. HC-05, HC-06, etc.)

Most comments are in English, some may be in German.

The goal of this sketch is, to connect to any HC-05, HC-06, HM-10 or similar Bluetooth modul through a serial-interface on the arduino.
a) no matter to which baud-rate it is set (by testing different baud-rates).
b) no matter whether the AT-commands need to be terminated with '/n/r' (e.g. my HC-05), or not (e.g. my HC-06).
c) no matter which firmware is running (OK, here my list of detected firmware-versions is very limited).

After connecting to the Bluetooth device, this sketch will use the AT-commands, to get and set device parameters, by using different lists of commands.
Please edit this list as you please.
I've set it up,
such that my HC-06 is set up as a slave device (use ac=1+2+8=11), and
such that my HC-05 is set up as a master device (use ac=1+2+4=7).

The electrical connections you need to make are described in the comments of the sketch.

NOTE: This sketch was inspired by other sketches I found on the internet.

Use as you please, but I give you no waranty what so ever!
