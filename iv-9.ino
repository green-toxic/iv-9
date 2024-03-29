#include <PCA95x5.h>
#include <RTClib.h>
#include <GyverButton.h>
#include <GyverTimers.h>
#include <EEPROMex.h>
#include <FastLED.h>

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
#define SET_ALARM_HOURS   6
#define SET_ALARM_MINUTES 7
#define SET_ALARM_SOUND   8
#define SHOW_DATE    10

#define DOT_1        A1
#define DOT_2        A3
#define BEEP         A0
#define LED_PIN      3

#define TONE         3100 //резонасная частота пищалки
#define SHORT_BEEP   50
#define LONG_BEEP    150
#define NUM_LEDS     6

PCA9555 ioex[3];

byte mode = SHOW_TIME;

byte bright = 1; // 1-5, 1  ярко / 5 тускло 

byte hours = 0;
byte minutes = 0;
byte seconds = 0;

byte date = 1;
byte month = 1;
byte year = 24;

byte alarm_hours   = 0;
byte alarm_minutes = 0;
byte alarm_melody  = 0; // задумка сделать несколько мелодий, пока просто пик
byte run_alarm     = 0;
byte skip_alarm    = 0;

byte synh_count = 0;

GButton butt[4];

//Светодиоды
CRGB leds[NUM_LEDS];
byte led_brightness = 128;
char led_mode = 0;
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

byte highlight_count = 0;

#include "led.h"

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
      butt[i].setClickTimeout(600);
    }
    pinMode(DOT_1, OUTPUT);
    pinMode(DOT_2, OUTPUT);

    digitalWrite(DOT_1, 1);
    digitalWrite(DOT_2, 1);

    if (EEPROM.readByte(100) != 89) {   // проверка на первый запуск
      EEPROM.writeByte(100, 89);
      EEPROM.writeByte(0, 8);     // часы будильника
      EEPROM.writeByte(1, 0);     // минуты будильника
      EEPROM.writeByte(2, 0);     // по умолчанию выключен
      EEPROM.writeByte(3, led_brightness); // яркость подстветки
      EEPROM.writeByte(4, led_mode);       // режим подсветки
      EEPROM.writeByte(5, gHue);           // цвет подсветки
    }
    
    alarm_hours    =  EEPROM.readByte(0);
    alarm_minutes  =  EEPROM.readByte(1);
    alarm_melody   =  EEPROM.readByte(2);
    led_brightness =  EEPROM.readByte(3);
    led_mode       =  EEPROM.readByte(4);
    gHue           =  EEPROM.readByte(5);

    synhTime();
    Timer1.setFrequency(2);
    Timer1.enableISR();
    
    //Настройка светодиодов
    FastLED.addLeds <WS2812B, LED_PIN, GRB> (leds, NUM_LEDS);
    
    offLed();    
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
  
  if (run_alarm) {
     beep(LONG_BEEP);        
     flasher(current);
  }
  
}

void led_effects() {
  if (led_mode >=0) {
    gPatterns[led_mode]();
    FastLED.setBrightness(led_brightness);
    FastLED.show();  
    // insert a delay to keep the framerate modest
    FastLED.delay(1000/FRAMES_PER_SECOND); 
    // do some periodic updates
    EVERY_N_MILLISECONDS( 1000 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  } else {
    offLed();
  }  
}

/*
сементы еденицы - бит, старший точка на цифру 7 бит 
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
сементы деятки - бит, младьший точка на цифру 7 бит 
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

void saveEEPROM() {
    EEPROM.writeByte(0, alarm_hours);     
    EEPROM.writeByte(1, alarm_minutes);     
    EEPROM.writeByte(2, alarm_melody);     
    EEPROM.writeByte(3, led_brightness);     
    EEPROM.writeByte(4, led_mode);
    EEPROM.writeByte(5, gHue);
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
  
  if ( mode < SET_ALARM_HOURS) {
    mode ++; // следующий режим
  }
  
  if ( mode > SET_YEAR) {
    mode = SHOW_TIME; 
    rtc.adjust(DateTime(2000 + year, month, date, hours, minutes, 0));
  }
}

void incrAlarmMode(){
  if (mode >= SET_ALARM_HOURS ) {
    mode ++;
  }
  if (mode > SET_ALARM_SOUND ) {
    mode = SHOW_TIME; 
    saveEEPROM();
    synhTime();
  }
}

void offAlarm(){
  skip_alarm = 1;
  offLed();
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
    if ( mode == SHOW_TIME ) {
      mode = SET_ALARM_HOURS;
      dotsOff();
    } else if (mode == SET_ALARM_HOURS) {
      mode = SHOW_TIME;
    }
    beep(LONG_BEEP);
  }
 
  if (butt[0].isClick()){
    offAlarm();
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

  if (butt[1].isSingle()) {
    offAlarm();
    switch(mode){
      case SET_HOURS:
          if (hours > 0 ) hours--; else hours = 23;
      break;
      case SET_MINUTES:
          if (minutes > 0 ) minutes--; else minutes = 59;
      break;
      
      case SET_ALARM_HOURS:
          if (alarm_hours > 0 ) alarm_hours--; else alarm_hours = 23;
      break;
      case SET_ALARM_MINUTES:
          if (alarm_minutes > 0 ) alarm_minutes--; else alarm_minutes = 59;
      break;
      case SET_ALARM_SOUND:
          if (alarm_melody > 0 ) alarm_melody--; else alarm_melody = 1;
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
  
  if (butt[2].isSingle()) {
    offAlarm();
    switch(mode){
      case SET_HOURS:
          if (hours < 23 ) hours++; else hours = 0;
      break;
      case SET_MINUTES:
          if (minutes < 59 ) minutes++; else minutes = 0;
      break;

      case SET_ALARM_HOURS:
          if (alarm_hours < 23 ) alarm_hours++; else alarm_hours = 0;
      break;
      case SET_ALARM_MINUTES:
          if (alarm_minutes < 59 ) alarm_minutes++; else alarm_minutes = 0;
      break;
      case SET_ALARM_SOUND:
          if (alarm_melody < 1 ) alarm_melody++; else alarm_melody = 0;
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
    offAlarm();
    incrAlarmMode();
  }

  if (butt[1].isDouble()) {
    if ( mode == SHOW_TIME ) {
      if (led_mode>=0 ) led_mode--;
      beep(SHORT_BEEP);
      saveEEPROM();
    }
  }
  
  if (butt[2].isDouble()) {
    if ( mode == SHOW_TIME ) {
      if (led_mode < LED_EFFECTS_NUM ) led_mode++;
      beep(SHORT_BEEP);
      saveEEPROM();
    }
  }
  
};

void loop() {
  call_butons();
  led_effects();
  
  switch(mode){
    case SHOW_TIME: 
      showTime();
      if (synh_count==0) { // раз в 256 сек синкаем врямя с RTC
        synhTime();
      }
      
    run_alarm = 0;      
    if (alarm_hours == hours && minutes == alarm_minutes ) { // логика включеня звука будильника
      if (!skip_alarm && alarm_melody ) {
          run_alarm = 1;        
      }
    } else {
      skip_alarm = 0;
    };
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
    case SET_ALARM_HOURS:
      showHighlighted(alarm_hours,alarm_minutes,alarm_melody,HOURS);
    break;
    case SET_ALARM_MINUTES:
      showHighlighted(alarm_hours,alarm_minutes,alarm_melody,MINUTES);
    break;
    case SET_ALARM_SOUND:
      showHighlighted(alarm_hours,alarm_minutes,alarm_melody,SECONDS);
    break;

  }

  
}
