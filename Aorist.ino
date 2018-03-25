#include <DS3231.h>

#define SPI_MOSI A0
#define SPI_CLK  A1
#define SPI_CS   A2

#define MAX_DECODEMODE   9
#define MAX_INTENSITY   10
#define MAX_SCANLIMIT   11
#define MAX_SHUTDOWN    12
#define MAX_DISPLAYTEST 15
#define MAX_DP         128

DS3231 rtc(SDA, SCL);

void setup()
{
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(SPI_CLK,  OUTPUT);
  pinMode(SPI_CS,   OUTPUT);
  spiTransfer(MAX_DECODEMODE, 0xFF);
  spiTransfer(MAX_INTENSITY,  0);
  spiTransfer(MAX_SCANLIMIT,  7);
  spiTransfer(MAX_SHUTDOWN,   1);
  rtc.begin();
  display_temperature();
  // Uncomment the following lines to set date and time
  //rtc.setDOW(FRIDAY);
  //rtc.setTime( 3, 48, 0);
  //rtc.setDate(21,  4, 2017);
  noInterrupts();
  // Set timer1 interrupt at 1Hz
  TCCR1A = TCCR1B = 0;
  TCNT1 = 15623;
  // Set compare match register for 1Hz increments
  OCR1A = 15624; // 16_000_000 / 1024 - 1
  // Turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // Enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  interrupts();
}

void loop(){}

ISR(TIMER1_COMPA_vect)
{
  Time t = rtc.getTime();
  display_2dig(t.hour, 7, MAX_DP);
  display_2dig(t.min,  5, MAX_DP);
  display_2dig(t.sec,  3, MAX_DP);
  if(t.sec == 0)
  {
    display_temperature();
  }
}

inline void display_temperature()
{
  display_2dig(rtc.getTemp(), 1, 0);
}

void display_2dig(uint8_t value, uint8_t digit, uint8_t dp)
{
  spiTransfer(digit + 1, value / 10);
  spiTransfer(digit,     (value % 10) | dp);
}

void spiTransfer(volatile uint8_t opcode, volatile uint8_t data)
{
  digitalWrite(SPI_CS, LOW);
  shiftOut(SPI_MOSI, SPI_CLK, MSBFIRST, opcode);
  shiftOut(SPI_MOSI, SPI_CLK, MSBFIRST, data);
  digitalWrite(SPI_CS, HIGH);
}
