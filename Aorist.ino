#define TWI_FREQ 400000L
#define DS3231_ADDR 0x68
#define DS3231_TIME 0x00
#define DS3231_TEMP 0x11

#define FAST // Comment to use Arduino ports
#ifdef FAST

#define SPI_MOSI 1 << PC0
#define SPI_CS   1 << PC1
#define SPI_CLK  1 << PC2
#define SPI_MOSI_SET(value) PORTC &= ~(SPI_MOSI); if(value) PORTC |= SPI_MOSI
#define SPI_CS_TOGGLE  PORTC |= SPI_CS; PORTC &= ~(SPI_CS)
#define SPI_CLK_TOGGLE PORTC |= SPI_CLK; PORTC &= ~(SPI_CLK)
#define SPI_DIR        DDRC  |= (SPI_MOSI) | (SPI_CS) | (SPI_CLK)

#else

#include <Wire.h>
#define SPI_MOSI A0
#define SPI_CS   A1
#define SPI_CLK  A2
#define SPI_MOSI_SET(value) digitalWrite(SPI_MOSI, (value) >> 8)
#define SPI_CS_TOGGLE digitalWrite(SPI_CS, HIGH); digitalWrite(SPI_CS, LOW)
#define SPI_CLK_TOGGLE digitalWrite(SPI_CLK, HIGH); digitalWrite(SPI_CLK, LOW)
#define SPI_DIR       pinMode(SPI_MOSI, OUTPUT); pinMode(SPI_CS, OUTPUT); pinMode(SPI_CLK, OUTPUT)

#endif

#define MAX_DECODEMODE   9
#define MAX_INTENSITY   10
#define MAX_SCANLIMIT   11
#define MAX_SHUTDOWN    12
#define MAX_DISPLAYTEST 15
#define MAX_DP         128

int main(void)
{
#ifndef FAST
  init();
#endif
  i2c_begin();
  spi_begin();
  // Uncomment to set RTC
  //rtc_write();
  display_temperature();
  noInterrupts();
  // Set timer1 interrupt at 1Hz
#ifndef FAST
  TCCR1A = 0;
#endif
  TCNT1 = F_CPU / 1024 - 2;
  // Set compare match register for 1Hz increments
  OCR1A = F_CPU / 1024 - 1;
  // Turn on CTC mode, CS10 and CS12 bits for 1024 prescaler
  TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
  // Enable timer compare interrupt
  TIMSK1 = 1 << OCIE1A;
  interrupts();
  while(1);
}

ISR(TIMER1_COMPA_vect)
{
#ifndef FAST
  interrupts();
#endif
  i2c_setup_rtc(DS3231_TIME, 3);
  uint8_t sec = i2c_read();
  display_2dig(sec,  3);
  uint8_t min = i2c_read();
  display_2dig(min,  5);
  uint8_t hour = i2c_read();
  display_2dig(hour, 7);
  if(sec == 0)
  {
    display_temperature();
  }
}

void display_temperature(void)
{
  i2c_setup_rtc(DS3231_TEMP, 2);
  uint8_t temp_msb = i2c_read();
  uint8_t temp_lsb = i2c_read();
  // Float formula: (float)temp_msb + ((temp_lsb >> 6) * 0.25f)
  if(temp_lsb & 0x80) ++temp_msb;
  // Fast division approximation for small integers using 26 / 256
  spi_transfer(2, (temp_msb * 26) >> 8); // temp_msb / 10
  spi_transfer(1, temp_msb - ((temp_msb * 26) >> 8) * 10); // temp_msb % 10
}

void display_2dig(uint8_t value, uint8_t digit)
{
  spi_transfer(digit + 1, value >> 4);
  spi_transfer(digit,    (value & 0xF) | MAX_DP);
}

void spi_begin(void)
{
  SPI_DIR;
  spi_transfer(MAX_DISPLAYTEST, 0);
  spi_transfer(MAX_DECODEMODE,  0xFF);
  spi_transfer(MAX_INTENSITY,   0);
  spi_transfer(MAX_SCANLIMIT,   7);
  spi_transfer(MAX_SHUTDOWN,    1);
}

void spi_transfer(uint8_t opcode, uint8_t data)
{
  // Shift out MSB
  uint16_t val = (opcode << 8) | data;
  uint8_t i = 16;
  do {
    SPI_MOSI_SET(val & 0x8000);
    SPI_CLK_TOGGLE;
    val <<= 1;
  } while(--i);
  SPI_CS_TOGGLE;
}

#define TWI_PULLUP PORTC |= (1 << PC4) | (1 << PC5)
#define TWI_START                TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWSTA); TWI_WAIT
#define TWI_WRITE(v) TWDR = (v); TWCR = (1 << TWEN) | (1 << TWINT);                TWI_WAIT
#define TWI_READ                 TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWEA);  TWI_WAIT
#define TWI_STOP                 TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWSTO)
#define TWI_WAIT while((TWCR & (1 << TWINT)) == 0)

void i2c_begin(void)
{
#ifdef FAST
  // TWI internal pullups for SDA and SCL
  TWI_PULLUP;
  // TWI frequency
  TWBR = (F_CPU / TWI_FREQ - 16) / 2;
  // TWI prescaler and bit rate
  //TWSR = ~((1 << TWPS0) | (1 << TWPS1));
#else
  Wire.begin();
  Wire.setClock(TWI_FREQ);
#endif
}

void i2c_setup_rtc(uint8_t address, uint8_t amount)
{
#ifdef FAST
  TWI_START;
  TWI_WRITE(DS3231_ADDR << 1);
  TWI_WRITE(address);
  TWI_START;
  TWI_WRITE((DS3231_ADDR << 1) | 1);
#else
  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(address);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)DS3231_ADDR, amount);
#endif
}

uint8_t i2c_read(void)
{
#ifdef FAST
  TWI_READ;
  return TWDR;
#else
  return Wire.read();
#endif
}

void i2c_close(void)
{
#ifdef FAST
  TWI_STOP;
#endif
}

#ifndef FAST
#define BCD(n) (n / 10 << 4) | (n % 10)
void rtc_write()
{
  Wire.beginTransmission(DS3231_ADDR);
  Wire.write(DS3231_TIME);
  Wire.write(BCD(0));  // Second
  Wire.write(BCD(27)); // Minute
  Wire.write(BCD(2));  // Hour
  Wire.write(5);       // Monday is 1
  Wire.write(BCD(6));  // Date
  Wire.write(BCD(12)); // Month
  Wire.write(BCD(19)); // Year - 2000
  Wire.endTransmission();
}
#endif
