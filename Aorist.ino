#include <LedControl.h>
#include <DS3231.h>

#define DIN 5
#define CS  6
#define CLK 7

#define MAX7219_AMOUNT 1

LedControl lc(DIN,CLK,CS,MAX7219_AMOUNT);
DS3231     rtc(SDA, SCL);

void setup()
{
  lc.shutdown(0,false);
  lc.clearDisplay(0);
  lc.setIntensity(0,1);
  rtc.begin();
  // The following lines can be uncommented to set the date and time
  //rtc.setDOW(FRIDAY);
  //rtc.setTime( 3, 48, 0);
  //rtc.setDate(21,  4, 2017);
  display_temperature();
}

void loop()
{
  Time t = rtc.getTime();
  lc.setDigit(0,7,t.hour / 10,false);
  lc.setDigit(0,6,t.hour % 10,true);
  lc.setDigit(0,5,t.min  / 10,false);
  lc.setDigit(0,4,t.min  % 10,true);
  lc.setDigit(0,3,t.sec  / 10,false);
  lc.setDigit(0,2,t.sec  % 10,true);
  if(t.sec == 0)
  {
    display_temperature();
  }
  delay(900);
}

void display_temperature()
{
  int temp = (int)rtc.getTemp();
  lc.setDigit(0,1,temp / 10,false);
  lc.setDigit(0,0,temp % 10,false);
}
