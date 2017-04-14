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
    int temp = (int)rtc.getTemp();
    lc.setDigit(0,1,temp / 10,false);
    lc.setDigit(0,0,temp % 10,false);
  }
  delay(900);
/*
  for(uint32_t i=0; i<100000000; ++i)
  {
    uint32_t j = i;
    uint8_t dot = i & 7;
    lc.setDigit(0,0, j       %10,dot == 0);
    lc.setDigit(0,1,(j /= 10)%10,dot == 1);
    lc.setDigit(0,2,(j /= 10)%10,dot == 2);
    lc.setDigit(0,3,(j /= 10)%10,dot == 3);
    lc.setDigit(0,4,(j /= 10)%10,dot == 4);
    lc.setDigit(0,5,(j /= 10)%10,dot == 5);
    lc.setDigit(0,6,(j /= 10)%10,dot == 6);
    lc.setDigit(0,7,(j /= 10)%10,dot == 7);
    lc.setIntensity(0,i);
    delay(1000);
  }
*/
}
