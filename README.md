# airmon
Simple air quality monitor with CO<sub>2</sub>, temperature ang humidity sensors. Based on MSP430 + Energia. Pin-compatible to Texas Instruments' Launchpad (v1.5), but I recommend to use your own soldered PCB because of power quality sensitivity of MH-Z19 sensor with step-up converter. The Dallas Semiconductor DS18B20 sensor is hot-pluggable.

## Thank you - This is a mixture of others' projects and example codes

For DS18B20 driver
- GFDS18B20(StellarisOW)  Grant Forest 29 Jan 2013. From https://www.43oh.com . Original links doesn't work, so I found this library [there](https://old.domotronic.fr/120MesuredeTemperatureDS18B20.html) and published: https://github.com/z3q/GFDS18B20. 

For AM2320 and i2c LCD drivers: [E. N. Timofeev](https://github.com/Ten04031977) 
- AM2321 Arduino library  https://github.com/Ten04031977/AM2320-master (forked to https://github.com/z3q/AM2320-integer with integer math)
- LiquidCrystal_I2C Arduino library https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library

## Hardware

MCU: Texas Intruments' MSP430G2553

Temperature and humidity sensor: AM2320 

CO<sub>2</sub> sensor MH-Z19

External temperature Sensor: DS18B20

Power: Li-ion battery, solar cell (15s, 7.5V), chinese usb li-ion charger with overcharge/overdischarge protection

LCD 16x2 with i2c module 

## How to compile and upload to the board

just download the Energia IDE (similar to Arduino), and copy the libs(DS18B20) to proper path.

Compile and upload the code to MCU with MSP430G2 Launchpad.
