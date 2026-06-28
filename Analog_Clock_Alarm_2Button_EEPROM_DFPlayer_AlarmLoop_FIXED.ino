/****************************************************
        ESP32 ANALOG WALL CLOCK + AUTO DATE
        ILI9341 TFT 240x320 SPI
        24 HOUR FORMAT
        AUTO DATE UPDATE AT MIDNIGHT
        SHOW NAMES IN CORNER (Alex, Jen, Jeni, Advik)
****************************************************/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <math.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <DFRobotDFPlayerMini.h>
RTC_DS3231 rtc;

HardwareSerial mp3Serial(2);
DFRobotDFPlayerMini dfPlayer;
int lastVoiceHour = -1;

void checkHourlyVoice();

void drawClockFace();
void drawHands();
void showName();
void handleAlarmButtons(); 

/**************** TFT PINS ****************/
#define TFT_CS   5
#define TFT_DC   27
#define TFT_RST  33

/**************** TFT OBJECT ****************/
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

/**************** CLOCK POSITION ****************/
int centerX = 120;
int centerY = 120;
int radius  = 118;

/**************** START TIME ****************/
int secondVal = 0;
int minuteVal = 0;
int hourVal   = 4;      // 24 Hour Format

/**************** START DATE ****************/
int dayVal    = 6;
int monthVal  = 6;
int yearVal   = 2026;

char daysOfWeek[7][12] =
{
  "SUN","MON","TUE","WED","THU","FRI","SAT"
};

int dayOfWeek = 6;   // SATURDAY

/**************** TIMERS ****************/
unsigned long previousMillis = 0;        // For clock update
unsigned long previousNameMillis = 0;    // For name update

/**************** NAMES ****************/
const char* names[] = {"Alex", "Jen", "Jeni", "Advik"};
int nameIndex = 0;

/**************** ALARM ****************/
#define BTN_SET    13
#define BTN_UP     26
#define BUZZER_PIN 4

int alarmHour = 0;
int alarmMinute = 0;
bool alarmEnabled = false;

byte setMode = 0;
bool lastSetState = HIGH;
bool lastUpState  = HIGH;

bool alarmTriggered = false;
unsigned long alarmStartTime = 0;


void handleAlarmButtons()
{
  bool setState = digitalRead(BTN_SET);
  bool upState  = digitalRead(BTN_UP);

  if(lastSetState == HIGH && setState == LOW)
  {
    setMode++;
    if(setMode > 2) setMode = 0;
    delay(200);
  }

  if(lastUpState == HIGH && upState == LOW)
  {
    if(setMode == 1)
    {
      alarmHour++;
      if(alarmHour > 23) alarmHour = 0;

      alarmEnabled = true;
      EEPROM.write(0, alarmHour);
      EEPROM.write(2, 1);
      EEPROM.commit();
    }

    if(setMode == 2)
    {
      alarmMinute++;
      if(alarmMinute > 59) alarmMinute = 0;

      alarmEnabled = true;
      EEPROM.write(1, alarmMinute);
      EEPROM.write(2, 1);
      EEPROM.commit();
    }

    delay(200);
  }

  lastSetState = setState;
  lastUpState  = upState;
}


/****************************************************/
void setup()
{
  Wire.begin(21,22);
  rtc.begin();

  EEPROM.begin(10);
  alarmHour = EEPROM.read(0);
  alarmMinute = EEPROM.read(1);
  alarmEnabled = EEPROM.read(2);

  if(alarmHour > 23) alarmHour = 0;
  if(alarmMinute > 59) alarmMinute = 0;

  pinMode(BTN_SET, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  mp3Serial.begin(9600, SERIAL_8N1, 16, 17);
  dfPlayer.begin(mp3Serial);
  dfPlayer.volume(25);
   rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Uncomment once to set RTC

  SPI.begin(18, 19, 23);
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(ILI9341_BLACK);

  drawClockFace();
  drawHands();
  showName();
}

/****************************************************/
void loop()
{
  handleAlarmButtons();
  checkHourlyVoice();
  unsigned long currentMillis = millis();

  // Update clock every second
  if (currentMillis - previousMillis >= 1000)
  {
    previousMillis = currentMillis;

drawHands();
  }

  // Update name every 5 seconds
  if (currentMillis - previousNameMillis >= 5000)
  {
    previousNameMillis = currentMillis;

    nameIndex++;
    if (nameIndex >= 4) nameIndex = 0;

    showName();
  }
}

/****************************************************/
void drawClockFace()
{
  tft.fillScreen(ILI9341_BLACK);

  /******** OUTER CIRCLE ********/
  tft.drawCircle(centerX, centerY, radius, ILI9341_WHITE);
  tft.drawCircle(centerX, centerY, radius - 1, ILI9341_WHITE);
  tft.drawCircle(centerX, centerY, radius - 2, ILI9341_WHITE);

  /******** HOUR MARKS ********/
  for (int i = 0; i < 60; i++)
  {
    float angle = (i * 6 - 90) * DEG_TO_RAD;
    int x1 = centerX + cos(angle) * (radius - 4);
    int y1 = centerY + sin(angle) * (radius - 4);
    int len = (i % 5 == 0) ? 12 : 5;
    int x2 = centerX + cos(angle) * (radius - len);
    int y2 = centerY + sin(angle) * (radius - len);
    tft.drawLine(x1, y1, x2, y2, ILI9341_WHITE);
  }

  /******** NUMBERS ********/
  for (int i = 1; i <= 12; i++)
  {
    float angle = (i * 30 - 90) * DEG_TO_RAD;
    int x = centerX + cos(angle) * 80;
    int y = centerY + sin(angle) * 80;
    tft.setTextColor(ILI9341_YELLOW);
    tft.setTextSize(2);
    if (i >= 10) tft.setCursor(x - 12, y - 8);
    else tft.setCursor(x - 6, y - 8);
    tft.print(i);
  }
}

/****************************************************/
void drawHands()
{
  DateTime now = rtc.now();
  secondVal = now.second();
  minuteVal = now.minute();
  hourVal   = now.hour();
  dayVal    = now.day();
  monthVal  = now.month();
  yearVal   = now.year();
  dayOfWeek = now.dayOfTheWeek();

  /******** ALARM CHECK ********/
  if(alarmEnabled && hourVal == alarmHour && minuteVal == alarmMinute && secondVal == 0 && !alarmTriggered)
  {
    alarmTriggered = true;
    alarmStartTime = millis();
  }

  if(hourVal != alarmHour || minuteVal != alarmMinute)
  {
    if(secondVal > 5) alarmTriggered = false;
  }

  static bool alarmMp3Started = false;
  static unsigned long alarmReplayTime = 0;

  if(alarmTriggered)
  {
    digitalWrite(BUZZER_PIN, LOW);

    if(!alarmMp3Started)
    {
      dfPlayer.stop();
      delay(200);
      dfPlayer.play(20);   // Start alarm MP3
      alarmMp3Started = true;
      alarmReplayTime = millis();
    }


    if(alarmMp3Started && millis() - alarmReplayTime >= 29000)
    {
      dfPlayer.play(20);
      alarmReplayTime = millis();
    }
    if(digitalRead(BTN_SET)==LOW || digitalRead(BTN_UP)==LOW)
    {
      dfPlayer.stop();
      alarmMp3Started = false;
      alarmTriggered = false;
      alarmEnabled = false;
      EEPROM.write(2,0);
      EEPROM.commit();
      delay(250);
    }
  }
  else
  {
    digitalWrite(BUZZER_PIN, LOW);
    alarmMp3Started = false;
  }


  /******** CLEAR CLOCK INSIDE ********/
  tft.fillCircle(centerX, centerY, radius - 18, ILI9341_BLACK);

  /******** REDRAW MARKS ********/
  for (int i = 0; i < 60; i++)
  {
    float angle = (i * 6 - 90) * DEG_TO_RAD;
    int x1 = centerX + cos(angle) * (radius - 4);
    int y1 = centerY + sin(angle) * (radius - 4);
    int len = (i % 5 == 0) ? 12 : 5;
    int x2 = centerX + cos(angle) * (radius - len);
    int y2 = centerY + sin(angle) * (radius - len);
    tft.drawLine(x1, y1, x2, y2, ILI9341_WHITE);
  }

  /******** REDRAW NUMBERS ********/
  for (int i = 1; i <= 12; i++)
  {
    float angle = (i * 30 - 90) * DEG_TO_RAD;
    int x = centerX + cos(angle) * 98;
    int y = centerY + sin(angle) * 98;
    tft.setTextColor(ILI9341_YELLOW);
    tft.setTextSize(2);
    if (i >= 10) tft.setCursor(x - 12, y - 8);
    else tft.setCursor(x - 6, y - 8);
    tft.print(i);
  }

  /******** SECOND HAND ********/
  float secondAngle = (secondVal * 6 - 90) * DEG_TO_RAD;
  int sx = centerX + cos(secondAngle) * 106;
  int sy = centerY + sin(secondAngle) * 106;
  tft.drawLine(centerX, centerY, sx, sy, ILI9341_RED);

  /******** MINUTE HAND ********/
  float minuteAngle = (minuteVal * 6 - 90) * DEG_TO_RAD;
  int mx = centerX + cos(minuteAngle) * 96;
  int my = centerY + sin(minuteAngle) * 96;
  for(int dx = -2; dx <= 2; dx++)
    for(int dy = -2; dy <= 2; dy++)
      tft.drawLine(centerX + dx, centerY + dy, mx + dx, my + dy, ILI9341_GREEN);

  /******** HOUR HAND ********/
  float hourAngle = (((hourVal % 12) * 30) + (minuteVal * 0.5) - 90) * DEG_TO_RAD;
  int hx = centerX + cos(hourAngle) * 74;
  int hy = centerY + sin(hourAngle) * 74;
  for(int dx = -3; dx <= 3; dx++)
    for(int dy = -2; dy <= 2; dy++)
      tft.drawLine(centerX + dx, centerY + dy, hx + dx, hy + dy, ILI9341_CYAN);

  /******** CENTER HUB ********/
  tft.fillCircle(centerX, centerY, 6, ILI9341_WHITE);

  /****************************************************
                    DATE DISPLAY
  ****************************************************/
  tft.fillRect(0, 240, 250, 60, ILI9341_BLACK);
   tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(3);
  tft.setCursor(30, 250);

  if (dayVal < 10) tft.print("0");
  tft.print(dayVal);
  tft.print("/");
    if (monthVal < 10) tft.print("0");
  tft.print(monthVal);
  tft.print("/");
  tft.print(yearVal);

  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(2);
  tft.setCursor(30,280);
  tft.print("TEMP:");
  tft.print(rtc.getTemperature(),1);
  tft.print("C");

  /******** DAY NAME ********/
  tft.setTextColor(ILI9341_RED);
  tft.setTextSize(2);
  tft.setCursor(170, 280);
  tft.print(daysOfWeek[dayOfWeek]);

  // redraw name after date area refresh
  showName();
}


void checkHourlyVoice()
{
  DateTime now = rtc.now();

  int hr = now.hour();
  int mn = now.minute();
  int sc = now.second();

  if(mn == 0 && sc == 0 && hr != lastVoiceHour)
  {
    if(hr >= 6 && hr <= 21)
    {
      int fileNo = hr - 5;   // 6->0001 ... 21->0016
      dfPlayer.play(fileNo);
      lastVoiceHour = hr;
    }
  }

  if(mn > 0 && hr != lastVoiceHour)
    lastVoiceHour = -1;
}


/****************************************************
                    NAME DISPLAY
****************************************************/



void showName()
{
 tft.fillRect(0, 295, 240, 25, ILI9341_BLACK);

 tft.setTextColor(ILI9341_WHITE);
 tft.setTextSize(2);
 tft.setCursor(30, 300);
 tft.print(names[nameIndex]);

 if(setMode == 1)
   tft.setTextColor(ILI9341_RED);
 else if(setMode == 2)
   tft.setTextColor(ILI9341_CYAN);
 else
   tft.setTextColor(ILI9341_ORANGE);

 tft.setCursor(115,300);

 if(alarmEnabled)
 {
   tft.print("A:");
   if(alarmHour < 10) tft.print("0");
   tft.print(alarmHour);
   tft.print(":");
   if(alarmMinute < 10) tft.print("0");
   tft.print(alarmMinute);
 }
 else
 {
   tft.print("A:__:__");
 }
}

