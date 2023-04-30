//-----------------------------------------------
// Aorist
//-----------------------------------------------
// Mau Magnaguagno
//-----------------------------------------------

#define TWI_FREQ 400000L
#define DS3231_ADDR 0x68
#define DS3231_TIME 0x00
#define DS3231_TEMP 0x11

#define TEMP 1 // 0: dot, 1: round up, other: truncate
#define FAST // Bare metal pins
//#define SPI_HARD // Hardware SPI, requires FAST

#ifdef FAST
#ifdef SPI_HARD

#define SPI_PORT PORTB
#define SPI_MOSI (1 << PB3)
#define SPI_CS   (1 << PB2)
#define SPI_CLK  (1 << PB5)
#define SPI_SETUP DDRB |= SPI_MOSI | SPI_CS | SPI_CLK; SPCR = (1 << SPE) | (1 << MSTR)//; SPSR = 1 << SPI2X

#else

#define SPI_PORT PORTC
#define SPI_MOSI (1 << PC0)
#define SPI_CS   (1 << PC1)
#define SPI_CLK  (1 << PC2)
#define SPI_SETUP DDRC |= SPI_MOSI | SPI_CS | SPI_CLK

#endif

#define SPI_MOSI_SET(value) SPI_PORT &= ~SPI_MOSI; if(value) SPI_PORT |= SPI_MOSI
#define SPI_CLK_TOGGLE SPI_PORT |= SPI_CLK; SPI_PORT &= ~SPI_CLK
#define SPI_CS_TOGGLE  SPI_PORT |= SPI_CS;  SPI_PORT &= ~SPI_CS
#define SPI_SHIFTOVER(d1,d2) __asm__("lsl %0" "\n\t" "rol %1" : "+r" (d2), "+r" (d1))

#define TWI_SETUP PORTC |= (1 << PC4) | (1 << PC5); TWBR = (F_CPU / TWI_FREQ - 16) / 2 // Internal pullups for SDA and SCL
#define TWI_START(v)             TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWSTA); TWI_WAIT; TWI_WRITE(v << 1)
#define TWI_REQUEST(v,l)         TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWSTA); TWI_WAIT; TWI_WRITE((v << 1) | 1); ((void)l)
#define TWI_WRITE(v) TWDR = (v); TWCR = (1 << TWEN) | (1 << TWINT);                TWI_WAIT
#define TWI_READ                 TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWEA);  TWI_WAIT; return TWDR
#define TWI_STOP                 TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWSTO)
#define TWI_WAIT         while(!(TWCR & (1 << TWINT)))
#define TWI_END          ((void)0)

#else

#define SPI_MOSI A0
#define SPI_CS   A1
#define SPI_CLK  A2
#define SPI_MOSI_SET(value) digitalWrite(SPI_MOSI, value)
#define SPI_CS_TOGGLE  digitalWrite(SPI_CS, HIGH);  digitalWrite(SPI_CS, LOW)
#define SPI_CLK_TOGGLE digitalWrite(SPI_CLK, HIGH); digitalWrite(SPI_CLK, LOW)
#define SPI_SETUP      pinMode(SPI_MOSI, OUTPUT); pinMode(SPI_CS, OUTPUT); pinMode(SPI_CLK, OUTPUT)
#define SPI_SHIFTOVER(d1,d2) d1 <<= 1; if(d2 & 0x80) d1 |= 1; d2 <<= 1

#include <Wire.h>
#define TWI_SETUP        Wire.begin(); Wire.setClock(TWI_FREQ)
#define TWI_START(v)     Wire.beginTransmission(v)
#define TWI_REQUEST(v,l) TWI_END; Wire.requestFrom((uint8_t)v, l)
#define TWI_WRITE(v)     Wire.write(v)
#define TWI_READ         return Wire.read()
#define TWI_STOP         ((void)0)
#define TWI_END          Wire.endTransmission()

#endif

#define MAX_DECODEMODE   9
#define MAX_INTENSITY   10
#define MAX_SCANLIMIT   11
#define MAX_SHUTDOWN    12
#define MAX_DISPLAYTEST 15
#define MAX_DP         128

int main(void)
{
  i2c_begin();
  spi_begin();
  // Uncomment to set RTC
  //i2c_write_rtc();
  // Set timer1 interrupt at 1Hz
  TCNT1 = F_CPU / 1024 - 2;
  // Set compare match register for 1Hz increments
  OCR1A = F_CPU / 1024 - 1;
  // Turn on CTC mode, CS10 and CS12 bits for 1024 prescaler
  //TCCR1A = 0;
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
  // Seconds, minutes, hours
  uint8_t digit = 2;
  do
  {
    uint8_t value = i2c_read();
    if(!value) __asm__("clt");
    spi_transfer(++digit, value | MAX_DP);
    spi_transfer(++digit, value >> 4 & 0xF);
    __asm__ goto("brts %l0" :::: skip);
  } while(digit < 8);
  display_temperature();
  __asm__("set");
skip:
  reti();
}

void display_temperature(void)
{
  i2c_setup_rtc(DS3231_TEMP, 2);
  // Float formula: ((temp_msb << 8) | temp_lsb) / 256.0
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

void spi_begin(void)
{
  SPI_SETUP;
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
  uint8_t i = 16;
  do
  {
    SPI_MOSI_SET(opcode & 0x80);
    SPI_CLK_TOGGLE;
    SPI_SHIFTOVER(opcode, data);
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

void i2c_write_rtc(void)
{
  TWI_START(DS3231_ADDR);
  TWI_WRITE(DS3231_TIME);
  TWI_WRITE(0x38); // Second
  TWI_WRITE(0x26); // Minute
  TWI_WRITE(0x23); // Hour
  TWI_WRITE(5);    // DoW (1-7)
  TWI_WRITE(0x13); // Date
  TWI_WRITE(0x04); // Month
  TWI_WRITE(0x17); // Year - 2000
  TWI_END;
}
