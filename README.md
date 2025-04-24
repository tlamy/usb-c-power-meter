USB-C Power Meter Firmware

# Sources

The source code is single file and straightforward as it's only one file, main.cpp.

My current development environment is CLion using PlatformIO. As this is not easily replicable (there is no free CLion
version), I also provide premade binaries.

# Binaries

In /firmware_bin, you can find a number of premade, you guessed it, binaries.
They are named from this pattern:

**firmware_\<version\>_\<resistance\>.bin**

<a href="firmware_bin/firmware_v1.1.3_10mohm.bin">firmware_v1.1.3_10mohm.bin</a> is the first github released binary, as
I sent out a couple of meters having the wrong shunt resistor soldered on, effectively maxing out the current sensor at
around 1.6A. This version is to be used with a replacement shunt resistor of .01Ω, or 10mΩ.

## Uploading firmware

If you're unable to build it yourself, you should
follow <a href="https://github.com/SequoiaSan/Guide-How-To-Upload-bin-to-ESP8266-ESP32">SequoiaSan's guide</a> on
Github. 
