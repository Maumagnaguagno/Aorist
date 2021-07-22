# Aorist
**DS3231 RTC with 7-segment 8 digits display using MAX7219**

The ATmega is interrupted every second to read the time from a RTC DS3231 using I2C and to write to a MAX7219 display using SPI.
MAX7219 code B decode mode is used to match the DS3231, every RTC byte represents two [BCD](https://en.wikipedia.org/wiki/Binary-coded_decimal) digits.

During setup and then once every minute (second equals to 0) the temperature is updated.
The temperature is represented by two bytes: ``(float)first_byte + (second_byte >> 6) * 0.25``.
One can select the temperature to round up, display the real part as a dot, or truncate using the **TEMP** flag.
The temperature is expected to stay between 0 and 68 degrees celsius, which matches the DS3231S operating range, but not the DS3231SN.
This limitation is only to conform with the fast division by 10 without hardware division: ``(temperature * 26) >> 8``, which only takes one multiply instruction while discarding the LSB.

Note that there are no delays in the code, as the TWI 400KHz for the DS3231 I2C waits a register change, and the MAX7219 can operate at 10MHz, faster than the hardware SPI.
A **FAST** flag is used to select between bare metal and Arduino code, with the bare metal version using less than 600 bytes of Flash.
The **SPI_HARD** can be enabled to make use of the faster SPI hardware, but requires specific pins.
Software SPI is useful to keep all pins in the same side of an Arduino Nano.

<img align=right src=Aorist.svg>

```
RTC DS3231 <--I2C--> Microcontroller --SPI--> MAX7219
```

## Parts
- ATmega8/48/88/168/328 compatible board
- [7-segment 8 digits MAX7219 module](https://datasheets.maximintegrated.com/en/ds/MAX7219-MAX7221.pdf)
- [RTC DS3231 module](https://datasheets.maximintegrated.com/en/ds/DS3231.pdf)

## Pins
Pin | Module
--- | ---
5V       | VCC MAX7219
A0 (PC0) or D11 (PB3) | SPI MOSI
A1 (PC1) or D10 (PB2) | SPI CS
A2 (PC2) or D13 (PB5) | SPI CLK
3.3V     | VCC DS3231
A4 (PC4) | I2C SDA
A5 (PC5) | I2C SCL
GND      | GND

## ToDo's
- [Support negative temperatures](https://arduinodiy.wordpress.com/2015/11/10/the-ds3231-rtc-temperature-sensor/)