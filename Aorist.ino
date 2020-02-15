#define TWI_FREQ 400000L
#define DS3231_ADDR 0x68
#define DS3231_TIME 0x00
#define DS3231_TEMP 0x11

#define FAST // Comment to use Arduino ports
#ifdef FAST

#define I2C_PORT PORTC
#define I2C_SDA  1 << PC4
#define I2C_SCL  1 << PC5
#define SPI_PORT PORTC
#define SPI_MODE DDRC
#define SPI_MOSI 1 << PC0
#define SPI_CS   1 << PC1
#define SPI_CLK  1 << PC2
#define SPI_CS_TOGGLE SPI_PORT |= SPI_CS; SPI_PORT &= ~(SPI_CS)
#define SPI_DIR       SPI_MODE |= (SPI_MOSI) | (SPI_CS) | (SPI_CLK)

#else

#include <Wire.h>
#define SPI_MOSI A0
#define SPI_CS   A1
#define SPI_CLK  A2
#define SPI_CS_TOGGLE digitalWrite(SPI_CS, HIGH); digitalWrite(SPI_CS, LOW)
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
  i2c_close();
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
  i2c_close();
  // Float formula: (float)temp_msb + ((temp_lsb >> 6) * 0.25f)
  temp_msb += temp_lsb >> 7;
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
  SPI_CS_TOGGLE;
  // Shift out MSB
  uint16_t val = (opcode << 8) | data;
  uint8_t i = 16;
  do {
#ifdef FAST
    SPI_PORT &= ~(SPI_MOSI);
    if(val & 0x8000) SPI_PORT |= SPI_MOSI;
    SPI_PORT |= SPI_CLK;
    SPI_PORT &= ~(SPI_CLK);
#else
    digitalWrite(SPI_MOSI, val >> 15);
    digitalWrite(SPI_CLK, HIGH);
    digitalWrite(SPI_CLK, LOW);
#endif
    val <<= 1;
  } while(--i);
}

#define TWI_START (1 << TWEN) | (1 << TWINT) | (1 << TWEA) | (1 << TWSTA)
#define TWI_ACK   (1 << TWEN) | (1 << TWINT) | (1 << TWEA)
#define i2c_wait() while((TWCR & (1 << TWINT)) == 0)

void i2c_begin(void)
{
#ifdef FAST
  // TWI internal pullups for SDA and SCL
  I2C_PORT |= (I2C_SDA) | (I2C_SCL);
  // TWI frequency
  TWBR = (F_CPU / TWI_FREQ - 16) / 2;
  // TWI prescaler and bit rate
  TWSR = ~((1 << TWPS0) | (1 << TWPS1));
  // Enable TWI module and ack
  TWCR = (1 << TWEN) | (1 << TWEA);
#else
  Wire.begin();
  Wire.setClock(TWI_FREQ);
#endif
}

void i2c_setup_rtc(uint8_t address, uint8_t amount)
{
#ifdef FAST
  TWCR = TWI_START;
  i2c_wait();

  TWDR = DS3231_ADDR << 1;
  TWCR = TWI_ACK;
  i2c_wait();

  TWDR = address;
  TWCR = TWI_ACK;
  i2c_wait();

  TWCR = TWI_START;
  i2c_wait();

  TWDR = (DS3231_ADDR << 1) | 1;
  TWCR = TWI_ACK;
  i2c_wait();
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
  TWCR = TWI_ACK;
  i2c_wait();
  return TWDR;
#else
  return Wire.read();
#endif
}

void i2c_close(void)
{
#ifdef FAST
  TWCR = (1 << TWEN) | (1 << TWINT);
  i2c_wait();
  TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWSTO);
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
