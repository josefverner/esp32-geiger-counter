/*
I2C: LCD, RTC
GPIO22 - CL
GPIO21 - DA
GND - GND

SPI: SD Card
GPIO05 - SS/CS
GPIO18 - CLK
GPIO19 - MISO
GPIO23 - MOSI
GND - GND

Geiger:
GPIO16 - Data (change eventually)
GPIO15 - Save button (toggle ON/OFF)
GPIO32 - LED save indicator
GND - GND

Button:
GPIO15 - Save button
GND - GND

1 MΩ Resistor:
GPIO16 - GND
*/

#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <Adafruit_NeoPixel.h>

#define DEBOUNCE_TIME 50
#define PIN_GEIGER_DATA 16
#define PIN_BUTTON_SAVE 15
#define PIN_LED_DAVE_INDICATOR 32
#define PIN_NEO_PIXEL 17
#define NUM_NEO_PIXELS 1
#define INDEX_NEO_PIXEL 0

Adafruit_NeoPixel NeoPixel(NUM_NEO_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);
RTC_DS1307 rtc;

volatile unsigned int particleCount = 0;
unsigned int lastParticleCount = 0;

bool isSavingActive = false;
bool saveToggle = false;
int lastSteadyButtonState = LOW;
int lastDebounceableButtonState = LOW;
int currentButtonState;
unsigned long lastDebounceTime = 0;

uint32_t neoPixel_Red = 0x00250000;
uint32_t neoPixel_Orange = 0x00690400;
uint32_t neoPixel_Green = 0x0000200;
uint32_t neoPixel_CurrentIdleColor = neoPixel_Green;

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

  NeoPixel.begin();

  if (! rtc.begin()) {
    Serial.println("RTC not found");
    while (1);
  }

  NeoPixel.setPixelColor(INDEX_NEO_PIXEL, 0x00310044);
  NeoPixel.show();

  if (! rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  lcd.init();
  lcd.backlight();
  delay(100);
  lcd.setCursor(0, 0);
  lcd.print("Geigerzaehler");

  NeoPixel.setPixelColor(INDEX_NEO_PIXEL, 0x0000298E);
  NeoPixel.show();
  delay(100);

  //SPI.begin(cardSCKPin, cardMISOPin, cardMOSIPin, cardCSPin);
  if(!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }

  pinMode(PIN_BUTTON_SAVE, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_GEIGER_DATA), ISR_particles, FALLING);

  NeoPixel.setPixelColor(INDEX_NEO_PIXEL, neoPixel_Green);
  NeoPixel.show();
}

void loop() {
  if (particleCount != lastParticleCount) {

    NeoPixel.setPixelColor(INDEX_NEO_PIXEL, neoPixel_Orange);
    NeoPixel.show();

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

    NeoPixel.setPixelColor(INDEX_NEO_PIXEL, neoPixel_CurrentIdleColor);
    NeoPixel.show();
  }

  currentButtonState = digitalRead(PIN_BUTTON_SAVE);

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
        logFilePath = "/" + String(rtc.now().unixtime()) + ".txt";

        Serial.println("Writing to file: " + logFilePath);

        logFile = SD.open(logFilePath, FILE_APPEND, true);
        if(!logFile){
          Serial.println("Failed to open file for appending");
          return;
        }

        NeoPixel.setPixelColor(INDEX_NEO_PIXEL, neoPixel_Red);
        NeoPixel.show();

        neoPixel_CurrentIdleColor = neoPixel_Red;
      } else {
        logFile.close();
        
        NeoPixel.setPixelColor(INDEX_NEO_PIXEL, neoPixel_Green);
        NeoPixel.show();

        neoPixel_CurrentIdleColor = neoPixel_Green;
      }
    }

    lastSteadyButtonState = currentButtonState;
  }

}
