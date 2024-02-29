#include <PCA95x5.h>
#include "RTClib.h"
#include "GyverButton.h"
#include "GyverTimers.h"

RTC_DS3231 rtc;

#define HOURS        2
#define MINUTES      1
#define SECONDS      0

#define SHOW_TIME    0
#define SET_HOURS    1
#define SET_MINUTES  2
#define SET_DATE     3
#define SET_MONTH    4
#define SET_YEAR     5
#define SHOW_DATE    10

#define DOT_1        A1
#define DOT_2        A3
#define BEEP         A0

#define TONE         3100 //резонасная частота пищалки
#define SHORT_BEEP   50
#define LONG_BEEP    150

PCA9555 ioex[3];

byte mode = SHOW_TIME;

byte bright = 1; // 1-5, 1  ярко / 5 тускло 

byte hours = 0;
byte minutes = 0;
byte seconds = 0;

byte date = 1;
byte month = 1;
byte year = 24;


byte synh_count = 0;

GButton butt[4];

byte highlight_count = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);

    Wire.begin();
    ioex[0].attach(Wire,0x20); // секунды
    ioex[1].attach(Wire,0x24); // минуты
    ioex[2].attach(Wire,0x22); // часы
    for (int i=0; i<=2; ++i) {
      ioex[i].polarity(PCA95x5::Polarity::ORIGINAL_ALL);
      //ioex[i].polarity(PCA95x5::Polarity::INVERTED_ALL); 
      ioex[i].direction(PCA95x5::Direction::OUT_ALL);
      ioex[i].write(PCA95x5::Level::L_ALL);
    }

    rtc.begin();
    if (rtc.lostPower()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    
    for (int i=0; i<=3; ++i) {
      butt[i].setType(LOW_PULL);
    }
    pinMode(DOT_1, OUTPUT);
    pinMode(DOT_2, OUTPUT);

    digitalWrite(DOT_1, 1);
    digitalWrite(DOT_2, 1);

    //pinMode(BEEP, OUTPUT);
    //digitalWrite(BEEP, 0);
    
    synhTime();
    Timer1.setFrequency(2);
    Timer1.enableISR();
}

ISR(TIMER1_A) {  
  if (mode!=SHOW_TIME) return;
  
  byte current = !digitalRead(DOT_1);
  digitalWrite(DOT_1, current);
  digitalWrite(DOT_2, current);
  //digitalWrite(BEEP, current);
  
  if (current) {
      synh_count++;
      seconds++;
      if (seconds == 60) {
        seconds = 0;
        minutes++;
      }
      
      if (minutes==60) {
        minutes=0;
        hours++;
      }
    
      if (hours == 24) {
        hours=0;
      }
  }
}


/*
void loop() {
    for (size_t i = 0; i < 16; ++i) {
        Serial.print("set port high: ");
        Serial.println(i);

        ioex[0].write(static_cast<PCA95x5::Port::Port>(i), PCA95x5::Level::H);
        ioex[1].write(static_cast<PCA95x5::Port::Port>(i), PCA95x5::Level::H);
        ioex[2].write(static_cast<PCA95x5::Port::Port>(i), PCA95x5::Level::H);
        delay(100);
    }

    for (size_t i = 0; i < 16; ++i) {
        Serial.print("set port low: ");
        Serial.println(i);

        ioex[0].write(static_cast<PCA95x5::Port::Port>(i), PCA95x5::Level::L);
        ioex[1].write(static_cast<PCA95x5::Port::Port>(i), PCA95x5::Level::L);
        ioex[2].write(static_cast<PCA95x5::Port::Port>(i), PCA95x5::Level::L);
        delay(500);
    }
}
*/

/*
сементы еденицы - бит, старший точка на цифрф 7 бит 
+---+   5
|   | ----- 7
|   |   4
+---+   3
|   | ----- 6
|   |   1
+---+   2
      * 8    
*/
byte font_lo[10] = {
    0b1111011,
    0b1100000,
    0b1010111,
    0b1110110,
    0b1101100,
    0b0111110,
    0b0111111,    
    0b1110000,
    0b1111111,
    0b1111100                                        
};
/*
сементы деятки - бит, младьший точка на цифрф 7 бит 
+---+   8
|   | ----- 6 
|   |   5
+---+   4
|   | ----- 7
|   |   2 
+---+   3 
      * 1    
*/
byte font_hi[10] = {
    0b11110110,
    0b01100000,
    0b10101110,
    0b11101100,
    0b01111000,
    0b11011100,
    0b11011110,    
    0b11100000,
    0b11111110,
    0b11111000                                        
};



void clearSection(byte section) {
  ioex[section].write(0xFFFF);
}

void clearAll() {
  ioex[0].write(0xFFFF);
  ioex[1].write(0xFFFF);
  ioex[2].write(0xFFFF);
}

void clear(byte section) {
  ioex[section].write(0xFFFF);
}

void showDigits(word number, byte section, word mask=0) {

  byte hi  = font_hi[ number / 10 ]; // старший разряд
  byte low = font_lo[ number % 10 ]; // младший разряд
    
  ioex[section].write(~( (low<<8)+hi | mask ) );
}

void synhTime() {
    DateTime now = rtc.now();
    seconds = now.second();
    minutes= now.minute();
    hours = now.hour();
    year = now.year() - 2000;
    month = now.month();
    date = now.day();
}

void showHighlighted(byte hours,byte minutes,byte seconds,byte highlight) {
  highlight_count++;
  if (highlight_count % 2 ) {
    showDigits(hours,HOURS);
    showDigits(minutes,MINUTES);
    showDigits(seconds,SECONDS);
    delay(2);    
  } else {
    if (highlight == HOURS  ) { showDigits(hours,HOURS);     } else { clear(HOURS);   };
    if (highlight == MINUTES) { showDigits(minutes,MINUTES); } else { clear(MINUTES); };
    if (highlight == SECONDS) { showDigits(seconds,SECONDS); } else { clear(SECONDS); };
    delay(5);    
  }
}

void showTime() {
    showDigits(hours,HOURS);
    showDigits(minutes,MINUTES);
    showDigits(seconds,SECONDS);
}
void showDate() {
    showDigits(date, HOURS,   0b1000000000000000);
    showDigits(month,MINUTES, 0b1000000000000000);
    showDigits(year, SECONDS);
};


void beep(int duration){
    tone(BEEP,TONE,duration);
}

void dotsOff() {
    digitalWrite(DOT_1, 0);
    digitalWrite(DOT_2, 0);
}

void incrMode(){
  if (mode == SHOW_TIME) seconds = 0; // установка времени сбрасывем секунды в 0
  
  mode ++; // следующий режим
  
  if ( mode > SET_YEAR ) {
    mode = SHOW_TIME; 
    rtc.adjust(DateTime(2000 + year, month, date, hours, minutes, 0));
  }
}

void call_butons(){
  int analog = analogRead(2);

  // Кнопки 1 - 100, 2 - 220, 3 -780, 4 -1000 
  butt[0].tick(analog < 150 && analog > 90);
  butt[1].tick(analog < 280 && analog > 190);
  butt[2].tick(analog < 810 && analog > 750);  
  butt[3].tick(analog < 1100 && analog > 950);  
  
  if (butt[0].isHolded()) {
    incrMode();
    beep(LONG_BEEP);
  }
 
  if (butt[3].isHolded()) {
  }

  if (butt[0].isClick()){
    if ( (mode > SHOW_TIME) && ( mode <= SET_YEAR ) ) {
      incrMode();
      beep(SHORT_BEEP);
    }
    if ( mode == SHOW_TIME ) {
      mode = SHOW_DATE;
      dotsOff();
      beep(SHORT_BEEP);
    } else if ( mode == SHOW_DATE ) {
      mode = SHOW_TIME;
      beep(SHORT_BEEP);
      synhTime();
    }
  }
  
  if (butt[1].isClick()) {
    switch(mode){
      case SHOW_TIME: 
        if (bright < 4) bright++;
      break;  
      case SET_HOURS:
          if (hours > 0 ) hours--; else hours = 23;
      break;
      case SET_MINUTES:
          if (minutes > 0 ) minutes--; else minutes = 59;
      break;
      case SET_DATE:
          if (date > 1 ) date--; else date = 31;
      break;
      case SET_MONTH:
          if (month > 1 ) month--; else month = 12;
      break;
      case SET_YEAR:
          if (year > 0 ) year--; else year = 99;
      break;
      
    }
    beep(SHORT_BEEP);
  }
  
  if (butt[2].isClick()) {
    switch(mode){
      case SHOW_TIME: 
        if (bright > 1 ) bright--;
      break;  
      case SET_HOURS:
          if (hours < 23 ) hours++; else hours = 0;
      break;
      case SET_MINUTES:
          if (minutes < 59 ) minutes++; else minutes = 0;
      break;
      case SET_DATE:
          if (date < 31 ) date++; else date = 1;
      break;
      case SET_MONTH:
          if (month < 12 ) month++; else month = 1;
      break;
      case SET_YEAR:
          if (year < 99 ) year++; else year = 0;
      break;
    }
    beep(SHORT_BEEP);
  }
  
  if (butt[3].isClick()) {
  }

  
};

void loop() {
  call_butons();
  
  //delay(10);
  /*
  int analog = analogRead(2);
  byte low = analog % 100;
  byte hi = analog / 100;
  clearAll();
  showDigits(hi,MINUTES);
  showDigits(low,SECONDS);
  delay(50);
  */  
  
  switch(mode){
    case SHOW_TIME: 
      showTime();
      if (synh_count==0) { // раз в 256 сек синкаем врямя с RTC
        synhTime();
      }
    break;
    
    case SHOW_DATE: 
      showDate();
    break;
    
    case SET_HOURS:
      showHighlighted(hours,minutes,seconds,HOURS);
    break;
    case SET_MINUTES:
      showHighlighted(hours,minutes,seconds,MINUTES);
    break;
    case SET_DATE:
      showHighlighted(date,month,year,HOURS);
    break;
    case SET_MONTH:
      showHighlighted(date,month,year,MINUTES);
    break;
    case SET_YEAR:
      showHighlighted(date,month,year,SECONDS);
    break;

  }
  
}
