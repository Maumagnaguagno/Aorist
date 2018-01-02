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
}

void loop()
{
  Time t = rtc.getTime();
  DISPLAY_2DIG(t.hour, 7, true);
  DISPLAY_2DIG(t.min,  5, true);
  DISPLAY_2DIG(t.sec,  3, true);
  if(t.sec == 0)
  {
    display_temperature();
  }
  delay(900);
}

inline void display_temperature()
{
  int temp = (int)rtc.getTemp();
  DISPLAY_2DIG(temp, 1, false);
}
