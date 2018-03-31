#include <Wire.h>

#define TWI_FREQ 400000L
#define DS3231_ADDR 0x68
#define DS3231_TIME 0x00
#define DS3231_TEMP 0x11

#define SPI_MOSI A0
#define SPI_CLK  A1
#define SPI_CS   A2

#define MAX_DECODEMODE   9
#define MAX_INTENSITY   10
#define MAX_SCANLIMIT   11
#define MAX_SHUTDOWN    12
#define MAX_DISPLAYTEST 15
#define MAX_DP         128

void setup(void)
{
  Wire.begin();
  // Same as Wire.setClock(TWI_FREQ);
  TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(SPI_CLK,  OUTPUT);
  pinMode(SPI_CS,   OUTPUT);
  spiTransfer(MAX_DECODEMODE, 0xFF);
  spiTransfer(MAX_INTENSITY,  0);
  spiTransfer(MAX_SCANLIMIT,  7);
  spiTransfer(MAX_SHUTDOWN,   1);
  display_temperature();
  // Uncomment to set RTC
  //rtc_write(0, 3, (2 << 4) | 3, 5, 3 << 4, 3, (1 << 4) | 8);
  noInterrupts();
  // Set timer1 interrupt at 1Hz
  TCCR1A = 0;
  TCNT1 = F_CPU / 1024 - 2;
  // Set compare match register for 1Hz increments
  OCR1A = F_CPU / 1024 - 1;
  // Turn on CTC mode, CS10 and CS12 bits for 1024 prescaler
  TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
  // Enable timer compare interrupt
  TIMSK1 |= 1 << OCIE1A;
  interrupts();
}

void loop(void){}

ISR(TIMER1_COMPA_vect)
{
  interrupts();
  rtc_read(DS3231_TIME, 3);
  uint8_t sec = Wire.read();
  display_2dig(sec,  3, MAX_DP);
  uint8_t min = Wire.read();
  display_2dig(min,  5, MAX_DP);
  uint8_t hour = Wire.read();
  display_2dig(hour, 7, MAX_DP);
  if(sec == 0)
  {
    display_temperature();
  }
}

void display_temperature(void)
{
  rtc_read(DS3231_TEMP, 2);
  uint8_t temp_msb = Wire.read();
  uint8_t temp_lsb = Wire.read();
  // Float formula: (float)temp_msb + ((temp_lsb >> 6) * 0.25f)
  uint8_t value = temp_msb + (temp_lsb >> 7);
  spiTransfer(2, value / 10);
  spiTransfer(1, value % 10);
}

void display_2dig(uint8_t value, uint8_t digit, uint8_t dp)
{
  spiTransfer(digit + 1, value >> 4);
  spiTransfer(digit,    (value & 0xF) | dp);
}

void spiTransfer(uint8_t opcode, uint8_t data)
{
  digitalWrite(SPI_CS, LOW);
  shiftOut(SPI_MOSI, SPI_CLK, MSBFIRST, opcode);
  shiftOut(SPI_MOSI, SPI_CLK, MSBFIRST, data);
  digitalWrite(SPI_CS, HIGH);
}

void rtc_read(uint8_t address, uint8_t amount)
{
  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(address);
  Wire.endTransmission();
  Wire.requestFrom(DS3231_ADDR, amount);
}

void rtc_write(uint8_t sec, uint8_t min, uint8_t hour, uint8_t dow, uint8_t date, uint8_t mon, uint8_t year)
{
  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(DS3231_TIME);
  Wire.write(sec);
  Wire.write(min);
  Wire.write(hour);
  Wire.write(dow); // Monday is 1
  Wire.write(date);
  Wire.write(mon);
  Wire.write(year); // Year - 2000
  Wire.endTransmission();
}
