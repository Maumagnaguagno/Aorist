#define TWI_FREQ 400000L
#define DS3231_ADDR 0x68
#define DS3231_TIME 0x00
#define DS3231_TEMP 0x11

#define TEMP 1 // 0: dot, 1: round up, other: truncate
#define FAST // Comment to use Arduino pins
//#define SPI_HARD // Comment to use soft SPI, requires FAST 

#ifdef FAST
#ifdef SPI_HARD

#define SPI_DDR  DDRB
#define SPI_PORT PORTB
#define SPI_MOSI (1 << PB3)
#define SPI_CS   (1 << PB2)
#define SPI_CLK  (1 << PB5)
#define SPI_DIR  SPI_DDR |= SPI_MOSI | SPI_CS | SPI_CLK; SPCR = (1 << SPE) | (1 << MSTR)

#else

#define SPI_DDR  DDRC
#define SPI_PORT PORTC
#define SPI_MOSI (1 << PC0)
#define SPI_CS   (1 << PC1)
#define SPI_CLK  (1 << PC2)
#define SPI_DIR  SPI_DDR |= SPI_MOSI | SPI_CS | SPI_CLK

#endif

#define SPI_MOSI_SET(value) SPI_PORT &= ~SPI_MOSI; if(value) SPI_PORT |= SPI_MOSI
#define SPI_CLK_TOGGLE SPI_PORT |= SPI_CLK; SPI_PORT &= ~SPI_CLK
#define SPI_CS_TOGGLE  SPI_PORT |= SPI_CS;  SPI_PORT &= ~SPI_CS

#define TWI_SETUP PORTC |= (1 << PC4) | (1 << PC5); TWBR = (F_CPU / TWI_FREQ - 16) / 2 // Internal pullups for SDA and SCL
#define TWI_START(v)             TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWSTA); TWI_WAIT; TWI_WRITE(v << 1)
#define TWI_WRITE(v) TWDR = (v); TWCR = (1 << TWEN) | (1 << TWINT);                TWI_WAIT
#define TWI_READ                 TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWEA);  TWI_WAIT; return TWDR
#define TWI_STOP                 TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWSTO)
#define TWI_WAIT         while((TWCR & (1 << TWINT)) == 0)
#define TWI_END          ((void)0)
#define TWI_REQUEST(v,l) TWI_START((v << 1) | 1)

#else

#define SPI_MOSI A0
#define SPI_CS   A1
#define SPI_CLK  A2
#define SPI_MOSI_SET(value) digitalWrite(SPI_MOSI, (value) >> 8)
#define SPI_CS_TOGGLE  digitalWrite(SPI_CS, HIGH);  digitalWrite(SPI_CS, LOW)
#define SPI_CLK_TOGGLE digitalWrite(SPI_CLK, HIGH); digitalWrite(SPI_CLK, LOW)
#define SPI_DIR        pinMode(SPI_MOSI, OUTPUT); pinMode(SPI_CS, OUTPUT); pinMode(SPI_CLK, OUTPUT)

#include <Wire.h>
#define TWI_SETUP        Wire.begin(); Wire.setClock(TWI_FREQ)
#define TWI_START(v)     Wire.beginTransmission(v)
#define TWI_WRITE(v)     Wire.write(v)
#define TWI_READ         return Wire.read()
#define TWI_STOP         ((void)0)
#define TWI_END          Wire.endTransmission()
#define TWI_REQUEST(v,l) TWI_END; Wire.requestFrom(v, l)

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
  // Set timer1 interrupt at 1Hz
#ifndef FAST
  cli();
  TCCR1A = 0;
#endif
  TCNT1 = F_CPU / 1024 - 2;
  // Set compare match register for 1Hz increments
  OCR1A = F_CPU / 1024 - 1;
  // Turn on CTC mode, CS10 and CS12 bits for 1024 prescaler
  TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
  // Enable timer compare interrupt
  TIMSK1 = 1 << OCIE1A;
  sei();
  while(1);
}

ISR(TIMER1_COMPA_vect, ISR_NAKED)
{
#ifndef FAST
  sei();
#endif
  i2c_setup_rtc(DS3231_TIME, 3);
  uint8_t sec = display_2dig(3);
  display_2dig(5); // min
  display_2dig(7); // hour
  if(sec == 0)
  {
    display_temperature();
  }
  reti();
}

void display_temperature(void)
{
  i2c_setup_rtc(DS3231_TEMP, 2);
  // Float formula: (float)temp_msb + ((temp_lsb >> 6) * 0.25f)
  uint8_t temp_msb = i2c_read();
#if TEMP == 0
  temp_msb |= i2c_read() & MAX_DP; // Dot
#elif TEMP == 1
  if(i2c_read() & 0x80) ++temp_msb; // Round up
#endif
  // Fast division approximation for small integers using 26 / 256
  spi_transfer(2, temp_msb * 26 >> 8); // temp_msb / 10
  spi_transfer(1, temp_msb - (temp_msb * 26 >> 8) * 10); // temp_msb % 10
}

uint8_t display_2dig(uint8_t digit)
{
  uint8_t value = i2c_read();
  spi_transfer(digit + 1, value >> 4);
  spi_transfer(digit, value | MAX_DP);
  return value;
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
#ifdef SPI_HARD
  SPDR = opcode;
  while(!(SPSR & (1 << SPIF)));
  SPDR = data;
  while(!(SPSR & (1 << SPIF)));
#else
  // Shift out MSB
  uint16_t val = (opcode << 8) | data;
  uint8_t i = 16;
  do {
    SPI_MOSI_SET(val & 0x8000);
    SPI_CLK_TOGGLE;
    val <<= 1;
  } while(--i);
#endif
  SPI_CS_TOGGLE;
}

void i2c_begin(void)   { TWI_SETUP; }
void i2c_close(void)   { TWI_STOP;  }
uint8_t i2c_read(void) { TWI_READ;  }

void i2c_setup_rtc(uint8_t address, uint8_t amount)
{
  TWI_START(DS3231_ADDR);
  TWI_WRITE(address);
  TWI_REQUEST(DS3231_ADDR, amount);
}

#define BCD(n) (n / 10 << 4) | (n % 10)
void rtc_write()
{
  TWI_START(DS3231_ADDR);
  TWI_WRITE(DS3231_TIME);
  TWI_WRITE(BCD(0));
  TWI_WRITE(BCD(27));
  TWI_WRITE(BCD(2));
  TWI_WRITE(5);
  TWI_WRITE(BCD(6));
  TWI_WRITE(BCD(12));
  TWI_WRITE(BCD(19));
  TWI_END;
}
