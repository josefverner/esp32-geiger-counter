/*
I2C: LCD, RTC
GPIO22 - CL
GPIO21 - DA

SPI: SD Card
GPIO05 - SS
GPIO18 - CLK
GPIO19 - MISO
GPIO23 - MOSI

Geiger:
GPIO16 - Data (change eventually)
GPIO15 - Save button (toggle ON/OFF)
GPIO32 - LED save indicatior

TODO:
Add other output devices

*/
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>

#define DEBOUNCE_TIME 50

RTC_DS1307 rtc;

const int geigerDataPin = 16;
const int saveButtonPin = 15;
const int saveIndicatorLedPin = 32;

volatile unsigned int particleCount = 0;
unsigned int lastParticleCount = 0;

bool isSavingActive = false;
bool saveToggle = false;
int lastSteadyButtonState = LOW;
int lastDebounceableButtonState = LOW;
int currentButtonState;
unsigned long lastDebounceTime = 0;
File logFile;
String logFilePath;

int lcdColumns = 16;
int lcdRows = 2;

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

void IRAM_ATTR ISR_particles() {
  particleCount++;
}


void setup() {
  Serial.begin(115200);

  if (! rtc.begin()) {
    Serial.println("RTC not found");
    while (1);
  }

  if (! rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  lcd.init();
  lcd.backlight();
  delay(100);
  lcd.setCursor(0, 0);
  lcd.print("Geigerzaehler");


  //SPI.begin(cardSCKPin, cardMISOPin, cardMOSIPin, cardCSPin);
  if(!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }

  pinMode(saveButtonPin, INPUT_PULLUP);
  pinMode(saveIndicatorLedPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(geigerDataPin), ISR_particles, FALLING);
}

void loop() {
  if (particleCount != lastParticleCount) {
    // Serial.println(particleCount);
    lastParticleCount = particleCount;

    DateTime now = rtc.now();

    String timeLog = now.timestamp(DateTime::TIMESTAMP_TIME);
    String dateLog = now.timestamp(DateTime::TIMESTAMP_DATE);
    Serial.println(dateLog + "T" + timeLog);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(dateLog);
    lcd.setCursor(0, 1);
    lcd.print(timeLog);
    if(isSavingActive && logFile) {
      logFile.println(dateLog + "T" + timeLog);
    }
  }

  currentButtonState = digitalRead(saveButtonPin);

  if (currentButtonState != lastDebounceableButtonState) {
    lastDebounceTime = millis();
    lastDebounceableButtonState = currentButtonState;
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {
    if(lastSteadyButtonState == HIGH && currentButtonState == LOW) {
      Serial.println("The button is pressed");

      saveToggle = !saveToggle;
      isSavingActive = saveToggle;


      if (isSavingActive) {
        //DateTime now = rtc.now().unixtime();
        logFilePath = "/" + String(rtc.now().unixtime()) + ".txt";

        Serial.println("Writing to file: " + logFilePath);

        logFile = SD.open(logFilePath, FILE_APPEND, true);
        if(!logFile){
          Serial.println("Failed to open file for appending");
          return;
        }

        digitalWrite(saveIndicatorLedPin, HIGH);
      } else {
        logFile.close();
        digitalWrite(saveIndicatorLedPin, LOW);
      }
    }

    lastSteadyButtonState = currentButtonState;
  }

}
