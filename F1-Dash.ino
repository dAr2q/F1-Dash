#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <Arduino_GFX_Library.h>
#include <XPT2046_Touchscreen.h>
#include <src/F1_25_UDP.h>  //Library MacManley/F1_25_UDP
#include <FastLED.h>

#define NUM_LEDS 15
#define BRIGHTNESS 20
#define DATA_PIN 22
CRGB leds[NUM_LEDS];

// --- Farbcodes ---
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_YELLOW 0xFFE0
#define COLOR_CYAN 0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_BLUE 0x001F
#define COLOR_GREY 0x2104

// --- Hardware Setup ---
#define TFT_BL 21
Arduino_DataBus* bus = new Arduino_ESP32SPI(2, 15, 14, 13, 12);
Arduino_GFX* gfx = new Arduino_ILI9341(bus, -1, 1);

#define WIFI_SET_PIN 0
const char* RPM_CFG = "F1-Dash-Config";

#define XPT2046_CS 33
#define XPT2046_IRQ 36
SPIClass kniSpi = SPIClass(VSPI);
XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ);

F1_25_Parser* parser;
int currentScreen = 0;
unsigned long lastTouch = 0;

// Speicher für Performance & Redraw-Logik
uint16_t lSpeed = 999;
int8_t lGear = 99;
int lBarW = 0;
float lFuel = -1.0;
float lDelta = 99.0;
float lErs = -1.0;
uint8_t lT[4] = { 0, 0, 0, 0 };
uint8_t lWear[4] = { 0, 0, 0, 0 };
uint8_t lVisualTyreID = 99; // Wichtig für Erkennung Mischungswechsel & Screen-Reset
uint8_t lActualTyreID = 99;  // C1,C2,C3,C4,C5,Inter,Wet Tyres for Temperature setting

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  pinMode(WIFI_SET_PIN, INPUT_PULLUP);
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  delay(250);
  
  gfx->begin();
  gfx->invertDisplay(true);
  gfx->fillScreen(COLOR_BLACK);
  delay(250);
  for (int i = 0; i < 5; i++) {
    leds[i] = (i < NUM_LEDS) ? CRGB::Green : CRGB::Black;
  }
  FastLED.show();
  config_mode();

  kniSpi.begin(25, 39, 32, 33);
  touch.begin(kniSpi);
  touch.setRotation(1);

  WiFiManager wm;
  wm.autoConnect(RPM_CFG);
  wm.setHostname("F1-Dash");
  wm.setTitle("Config Mode");
  wm.setDarkMode(true);
  wm.setConnectTimeout(60);

  switch (WiFi.status()) {
    case WL_CONNECTED:
    parser = new F1_25_Parser();
    gfx->setTextSize(1);
    gfx->setCursor(20, 210);
    gfx->setTextColor(COLOR_GREEN);
    gfx->print("connected");
    delay(250);
    gfx->setCursor(80, 210);
    gfx->setTextColor(COLOR_YELLOW);
    gfx->print(WiFi.localIP());
    for (int i = 5; i < 10; i++) {
      leds[i] = (i < NUM_LEDS) ? CRGB::Red : CRGB::Black;
    }
    FastLED.show();
    delay(250);
    parser->begin(20777);
    for (int i = 10; i < 15; i++) {
      leds[i] = (i < NUM_LEDS) ? CRGB::Purple : CRGB::Black;
    }     
    break;       
    case WL_CONNECT_FAILED:
    gfx->setTextSize(1);
    gfx->setCursor(20, 210);
    gfx->setTextColor(COLOR_RED);
    gfx->print("connect fail");
    break;
    default:
    gfx->setTextSize(1);
    gfx->setCursor(20, 210);
    gfx->setTextColor(COLOR_YELLOW);
    gfx->print(WiFi.status());
    break;
}
  FastLED.show();
  delay(250);
  FastLED.clear();
  gfx->fillScreen(COLOR_BLACK);
  delay(250);
}
void config_mode() {
  gfx->fillScreen(COLOR_BLACK);
  gfx->setTextSize(3);
  gfx->setCursor(20, 55);
  gfx->setTextColor(COLOR_WHITE);
  gfx->print("F1-Dash 2025");
  delay(250);
  gfx->setTextSize(2);
  gfx->setCursor(30, 90);
  gfx->setTextColor(COLOR_WHITE);
  gfx->print("If No Connect. Use AP:");
  gfx->setCursor(30, 115);
  gfx->setTextColor(COLOR_GREEN);
  gfx->print(RPM_CFG);
}

void loop() {
  // Screen Wechsel per Touch
  if (touch.touched() && (millis() - lastTouch > 500)) {
    currentScreen = (currentScreen + 1) % 2;
    gfx->fillScreen(COLOR_BLACK);
    
    // Alle Speicherwerte resetten, um Redraw zu erzwingen
    lSpeed = 999;
    lGear = 99;
    lBarW = 0;
    lErs = -1.0;
    lDelta = 99.0;
    lFuel = 1.0;
    lVisualTyreID = 99; 
    lActualTyreID = 99;
    for (int i = 0; i < 4; i++) {
      lT[i] = 0;
      lWear[i] = 0;
    }
    lastTouch = millis();
  }

  if (digitalRead(WIFI_SET_PIN) == LOW) {
    gfx->fillScreen(COLOR_BLACK);
    gfx->setTextSize(3);
    gfx->setCursor(20, 55);
    gfx->setTextColor(COLOR_WHITE);
    gfx->print("F1-Dash Config");
    gfx->setCursor(20, 95);
    gfx->setTextColor(COLOR_YELLOW);
    gfx->print(WiFi.localIP());

    WiFiManager wm;
    wm.setHostname("F1-Dash");
    wm.setTitle("Config Mode");
    wm.setDarkMode(true);
    wm.startConfigPortal(RPM_CFG);
    currentScreen = (currentScreen + 1) % 2;
    gfx->fillScreen(COLOR_BLACK);
  }

  parser->read();
  uint8_t pIdx = parser->packetCarTelemetryData()->m_playerCarIndex();
  uint16_t revBits = parser->packetCarTelemetryData()->m_carTelemetryData(pIdx).m_revLightsBitValue;
  updateRevLights(revBits);

  if (currentScreen == 0) {
    // --- MAIN DASH ---
    uint16_t speed = parser->packetCarTelemetryData()->m_carTelemetryData(pIdx).m_speed;
    int8_t gear = parser->packetCarTelemetryData()->m_carTelemetryData(pIdx).m_gear;
    float fuel = parser->packetCarStatusData()->m_carStatusData(pIdx).m_fuelRemainingLaps;
    float ers = parser->packetCarStatusData()->m_carStatusData(pIdx).m_ersStoreEnergy;
    float delta = (float)parser->packetLapData()->m_lapData(pIdx).m_deltaToCarInFrontInMSPart / 1000.0f;

    drawF1Dash(speed, gear, fuel, delta, ers);
  } else {
    // --- TYRE DASH ---
    uint8_t t[4], w[4];
    uint8_t visualTyreID = parser->packetCarStatusData()->m_carStatusData(pIdx).m_visualTyreCompound;
    uint8_t actualTyreID = parser->packetCarStatusData()->m_carStatusData(pIdx).m_actualTyreCompound;
    for (int i = 0; i < 4; i++) {
      t[i] = parser->packetCarTelemetryData()->m_carTelemetryData(pIdx).m_tyresInnerTemperature[i];
      w[i] = (uint8_t)parser->packetCarDamageData()->m_carDamageData(pIdx).m_tyresWear[i];
    }
    drawTyreDash(t, w, visualTyreID, actualTyreID);
  }
  FastLED.show();
}

void drawF1Dash(uint16_t s, int8_t g, float fuel, float delta, float ers) {
  // Gear
  if (g != lGear) {
    gfx->setTextSize(10);
    gfx->setCursor(135, 25);
    gfx->setTextColor(COLOR_BLACK);
    gfx->print(lGear == 0 ? "N" : (lGear == -1 ? "R" : String(lGear)));
    gfx->setCursor(135, 25);
    gfx->setTextColor(COLOR_WHITE);
    gfx->print(g == 0 ? "N" : (g == -1 ? "R" : String(g)));
    lGear = g;
  }

  // SPEED
  if (s != lSpeed) {
    gfx->setTextSize(3);
    gfx->setCursor(135, 140);
    gfx->setTextColor(COLOR_BLACK);
    gfx->print(String(lSpeed));
    gfx->setCursor(135, 140);
    gfx->setTextColor(COLOR_WHITE);
    gfx->print(String(s));
    lSpeed = s;
  }

  // ERS
  int ersW = map((long)ers, 0, 4000000, 0, 299);
  if (abs(ers - lErs) > 10000) {
    gfx->drawRect(10, 165, 300, 27, COLOR_WHITE);
    gfx->fillRect(11, 166, ersW, 25, COLOR_YELLOW);
    gfx->fillRect(11 + ersW, 166, 298 - ersW, 25, COLOR_BLACK);
    lErs = ers;
  }

  // DELTA
  if (abs(delta - lDelta) > 0.05) {
    gfx->setTextSize(2);
    gfx->setCursor(245, 210);
    gfx->setTextColor(COLOR_BLACK);
    gfx->print(String(lDelta, 3));
    gfx->setCursor(245, 210);
    gfx->setTextColor(delta <= 0 ? COLOR_GREEN : COLOR_RED);
    gfx->print(String(delta, 3));
    lDelta = delta;
  }

  // FUEL
  if (abs(fuel - lFuel) > 0.1) {
    gfx->setTextSize(2);
    gfx->setCursor(15, 210);
    gfx->setTextColor(COLOR_BLACK);
    gfx->print("F: " + String(lFuel, 2));
    gfx->setCursor(15, 210);
    gfx->setTextColor(COLOR_WHITE);
    gfx->print("F: " + String(fuel, 2));
    lFuel = fuel;
  }
}

void drawTyreDash(uint8_t t[], uint8_t w[], uint8_t tyreID, uint8_t tyreID2) {
  int x[4] = { 45, 195, 45, 195 };
  int y[4] = { 135, 135, 45, 45 };

  // --- REIFENTYP ANZEIGE (Zentriert auf 320px) ---
  if (tyreID != lVisualTyreID) {
    gfx->fillRect(80, 5, 320, 30, COLOR_BLACK);
    
    String tyreName = "";
    uint16_t tyreColor = COLOR_WHITE;

    switch (tyreID) {
      case 16:
        tyreName = "SOFT";
        tyreColor = COLOR_RED;
        break;
      case 17:
        tyreName = "MEDIUM";
        tyreColor = COLOR_YELLOW;
        break;
      case 18:
        tyreName = "HARD";
        tyreColor = COLOR_WHITE;
        break;
      case 7:
        tyreName = "INTER";
        tyreColor = COLOR_GREEN;
        break;
      case 8:
        tyreName = "WET";
        tyreColor = COLOR_BLUE;
        break;
      default: tyreName = "TYRE"; break;
    }

    gfx->setTextSize(2);
    gfx->setTextColor(tyreColor);
    // Zentrierung für 320px Breite
    int16_t x1, y1;
    uint16_t w_text, h_text;
    gfx->getTextBounds(tyreName, 0, 0, &x1, &y1, &w_text, &h_text);
    gfx->setCursor(160 - (w_text / 2), 10); 
    gfx->print(tyreName);
  }
  if (tyreID2 != lActualTyreID) {

    String tyreComp = "";
    switch (tyreID2) {
      case 16:
        tyreComp = "C5";
        break;
      case 17:
        tyreComp = "C4";
        break;
      case 18:
        tyreComp = "C3";
        break;
      case 19:
        tyreComp = "C2";
        break;
      case 20:
        tyreComp = "C1";
        break;
      case 21:
        tyreComp = "C0";
        break;
      case 22:
        tyreComp = "C6";
        break;
      default: tyreComp = ""; break;
    }
    gfx->setTextSize(2);
    gfx->setCursor(240, 10);
    gfx->print(tyreComp);
  }


  for (int i = 0; i < 4; i++) {
    if (t[i] != lT[i] || w[i] != lWear[i] || tyreID2 != lActualTyreID) {

      uint16_t color;
      int tempMin, tempMax;

      // Logik basierend auf ActualTyreID (F1 24 Standard)
      switch (tyreID2) {
        case 16:
          tempMin = 66;
          tempMax = 106;
          break;  // C5
        case 17:
          tempMin = 76;
          tempMax = 106;
          break;  // C4
        case 18:
          tempMin = 76;
          tempMax = 116;
          break;  // C3
        case 19:  
          tempMin = 86;
          tempMax = 126;
          break;  // C2
        case 20:  
          tempMin = 96;
          tempMax = 126;
          break;  // C1
        case 22:  
          tempMin = 56;
          tempMax = 96;
          break;  // C6
        case 7:
          tempMin = 46;
          tempMax = 106;
          break;  // Inter
        case 8:
          tempMin = 36;
          tempMax = 96;
          break;  // Wet
        default:
          tempMin = 65;
          tempMax = 100;
          break;
      }

      if (t[i] > tempMax) color = COLOR_RED;
      else if (t[i] < tempMin) color = COLOR_BLUE;
      else color = COLOR_GREEN;

      // Zeichnen
      gfx->fillRoundRect(x[i], y[i], 85, 80, 8, color);

      gfx->setTextColor(COLOR_BLACK);
      gfx->setTextSize(3);
      gfx->setCursor(x[i] + 20, y[i] + 15);
      gfx->print(String(t[i]));

      gfx->setTextSize(2);
      gfx->setCursor(x[i] + 18, y[i] + 50);
      gfx->print(String(w[i]) + "%");

      lT[i] = t[i];
      lWear[i] = w[i];
    }
  }
  lVisualTyreID = tyreID;
  lActualTyreID = tyreID2;
}

void updateRevLights(uint16_t bits) {
  for (int i = 0; i < 15; i++) {
    if (bits & (1 << i)) {
      if (i < 5) {
        leds[i] = CRGB::Green;
      } else if (i < 10) {
        leds[i] = CRGB::Red;
      } else {
        leds[i] = CRGB::Purple;
      }
    } else {
      leds[i] = CRGB::Black;
    }
  }
}