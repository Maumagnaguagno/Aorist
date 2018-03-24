#include <LedControl.h>
#include <DS3231.h>

#define DIN A0
#define CS  A1
#define CLK A2

LedControl lc(DIN,CLK,CS);
DS3231     rtc(SDA, SCL);

#define DISPLAY_2DIG(N,POS,DOT) \
  lc.setDigit(0, POS,   N / 10, false); \
  lc.setDigit(0, POS-1, N % 10, DOT);

void setup()
{
  lc.shutdown(0,false);
  lc.setIntensity(0,1);
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
  OCR1A = 15624; // (16_000_000) / 1024 - 1
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
  DISPLAY_2DIG(t.hour, 7, true);
  DISPLAY_2DIG(t.min,  5, true);
  DISPLAY_2DIG(t.sec,  3, true);
  if(t.sec == 0)
  {
    display_temperature();
  }
}

inline void display_temperature()
{
  byte temp = (byte)rtc.getTemp();
  DISPLAY_2DIG(temp, 1, false);
}
