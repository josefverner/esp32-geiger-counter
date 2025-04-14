/*
TFT Display: 320×240
GPIO25 - CS
GPIO32 - DC
GPIO33 - RST
GPIO23 - MOSI
GPIO19 - MISO
GPIO18 - CLK
5V - LED

I2C: RTC
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
GPIO16 - Data
GPIO15 - Save button (toggle ON/OFF)
GPIO32 - LED save indicator
GND - GND

Button:
GPIO15 - Save button
GND - GND

1 MΩ Resistor:
GPIO16 - GND
*/

#include <Adafruit_ILI9341.h>
#include <Adafruit_NeoPixel.h>
#include <RTClib.h>
#include <SD.h>
#include <SPI.h>

#define DEBOUNCE_TIME 50
#define PIN_GEIGER_DATA 36
#define PIN_BUTTON_SAVE 15
#define PIN_NEO_PIXEL 2
#define NUM_NEO_PIXELS 1
#define INDEX_NEO_PIXEL 0
#define TFT_CS 25
#define TFT_DC 32
#define TFT_RST 33

Adafruit_NeoPixel NeoPixel(NUM_NEO_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);
RTC_DS1307 rtc;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

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

void IRAM_ATTR ISR_particles() {
  particleCount++;
}

void setup() {
  Serial.begin(115200);

  NeoPixel.begin();

  if (!rtc.begin()) {
    Serial.println("RTC not found");
    while (1);
  }

  NeoPixel.setPixelColor(INDEX_NEO_PIXEL, 0x00310044);
  NeoPixel.show();

  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  tft.begin();

  tft.fillScreen(ILI9341_BLACK);
  // 0 = 0° (Portrait), 1 = 90°, 2 = 180°, 3 = 270°
  tft.setRotation(3);
  delay(100);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.println("Geigerzaehler");

  NeoPixel.setPixelColor(INDEX_NEO_PIXEL, 0x0000298E);
  NeoPixel.show();
  delay(100);

  // SPI.begin(cardSCKPin, cardMISOPin, cardMOSIPin, cardCSPin);
  if (!SD.begin()) {
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

    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 0);
    tft.println(dateLog);
    tft.setCursor(0, 32);
    tft.println(timeLog);
    if (isSavingActive && logFile) {
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
    if (lastSteadyButtonState == HIGH && currentButtonState == LOW) {
      Serial.println("The button is pressed");

      saveToggle = !saveToggle;
      isSavingActive = saveToggle;

      if (isSavingActive) {
        logFilePath = "/" + String(rtc.now().unixtime()) + ".txt";

        Serial.println("Writing to file: " + logFilePath);

        logFile = SD.open(logFilePath, FILE_APPEND, true);
        if (!logFile) {
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
