#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <Arduino_GFX_Library.h> 
#include <XPT2046_Touchscreen.h>
#include <src/F1_24_UDP.h>

// --- Farbcodes (Vollständig) ---
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_BLUE    0x001F
#define COLOR_GREY    0x2104

// --- Hardware Setup ---
#define TFT_BL 21
Arduino_DataBus *bus = new Arduino_ESP32SPI(2, 15, 14, 13, 12);
Arduino_GFX *gfx = new Arduino_ILI9341(bus, -1, 1);

#define WIFI_SET_PIN 0
const char* RPM_CFG = "RPM-Display-Config";


#define XPT2046_CS 33
#define XPT2046_IRQ 36
SPIClass kniSpi = SPIClass(VSPI);
XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ);

F1_24_Parser* parser;
int currentScreen = 0;
unsigned long lastTouch = 0;

// Speicher für Performance
uint16_t lSpeed = 999; int8_t lGear = 99; int lBarW = 0; 
float lFuel = -1.0; float lDelta = 99.0; float lErs = -1.0;
uint8_t lPos = 0;
uint8_t lT[4] = {0,0,0,0}; uint8_t lWear[4] = {0,0,0,0};

void setup() {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    pinMode(WIFI_SET_PIN, INPUT_PULLUP);
    gfx->begin();
    gfx->invertDisplay(true);
    gfx->fillScreen(COLOR_BLACK);
    
    kniSpi.begin(25, 39, 32, 33);
    touch.begin(kniSpi);
    touch.setRotation(1);

    WiFiManager wm;
    wm.autoConnect(RPM_CFG);

    gfx->fillScreen(COLOR_BLACK);
    parser = new F1_24_Parser();
    parser->begin();
}

void loop() {
    // Screen Wechsel per Touch
    if (touch.touched() && (millis() - lastTouch > 500)) {
        currentScreen = (currentScreen + 1) % 2;
        gfx->fillScreen(COLOR_BLACK);
        // Reset Speicher für komplettes Neuzeichnen
        lSpeed = 999; lGear = 99; lBarW = 0; lErs = -1.0; lDelta = 99.0;
        for(int i=0; i<4; i++) { lT[i]=0; lWear[i]=0; }
        lastTouch = millis();
    }

    if (digitalRead(WIFI_SET_PIN) == LOW) {
    WiFiManager wm;
    wm.setHostname("RPM-Display");
    wm.setTitle("Config Mode");
    wm.setDarkMode(true);
    wm.startConfigPortal(RPM_CFG);
  }

    parser->read();
    uint8_t pIdx = parser->packetCarTelemetryData()->m_playerCarIndex();

    if (currentScreen == 0) {
        // --- MAIN DASH ---
        uint16_t speed = parser->packetCarTelemetryData()->m_carTelemetryData(pIdx).m_speed;
        int8_t gear    = parser->packetCarTelemetryData()->m_carTelemetryData(pIdx).m_gear;
        uint16_t rpm   = parser->packetCarTelemetryData()->m_carTelemetryData(pIdx).m_engineRPM;
        float fuel     = parser->packetCarStatusData()->m_carStatusData(pIdx).m_fuelRemainingLaps;
        float ers      = parser->packetCarStatusData()->m_carStatusData(pIdx).m_ersStoreEnergy;
        float delta    = (float)parser->packetLapData()->m_lapData(pIdx).m_deltaToCarInFrontInMSPart / 1000.0f;
        uint8_t pos    = parser->packetLapData()->m_lapData(pIdx).m_carPosition;

        drawF12020Dash(speed, gear, rpm, fuel, delta, pos, ers);
    } else {
        // --- TYRE DASH ---
        uint8_t t[4], w[4];
        for(int i=0; i<4; i++) {
          t[i] = parser->packetCarTelemetryData()->m_carTelemetryData(pIdx).m_tyresSurfaceTemperature[i];
          w[i] = (uint8_t)parser->packetCarDamageData()->m_carDamageData(pIdx).m_tyresWear[i];
        }
        drawTyreDash(t, w);
    }
}

void drawF12020Dash(uint16_t s, int8_t g, uint16_t rpm, float fuel, float delta, uint8_t pos, float ers) {
    // 1. RPM BALKEN
    int barW = map(rpm, 0, 15000, 0, 320);
    if (barW != lBarW) {
        uint16_t c = (rpm > 13200) ? COLOR_MAGENTA : (rpm > 11800 ? COLOR_RED : COLOR_GREEN);
        gfx->fillRect(0, 0, barW, 18, c);
        gfx->fillRect(barW, 0, 320 - barW, 18, COLOR_BLACK);
        lBarW = barW;
    }

    // 2. GANG (Zentral)
    if (g != lGear) {
        gfx->setTextSize(10);
        gfx->setCursor(135, 55);
        gfx->setTextColor(COLOR_BLACK);
        gfx->print(lGear == 0 ? "N" : (lGear == -1 ? "R" : String(lGear)));
        gfx->setCursor(135, 55);
        gfx->setTextColor(COLOR_WHITE);
        gfx->print(g == 0 ? "N" : (g == -1 ? "R" : String(g)));
        lGear = g;
    }

    // 3. SPEED
    if (s != lSpeed) {
        gfx->setTextSize(3);
        gfx->setCursor(130, 140);
        gfx->setTextColor(COLOR_BLACK);
        gfx->print(String(lSpeed));
        gfx->setCursor(130, 140);
        gfx->setTextColor(COLOR_WHITE);
        gfx->print(String(s));
        lSpeed = s;
    }

    // 4. ERS BALKEN (Unten im 2020er Style)
    int ersW = map((long)ers, 0, 4000000, 0, 300);
    if (abs(ers - lErs) > 10000) {
        gfx->drawRect(10, 185, 300, 12, COLOR_WHITE);
        gfx->fillRect(11, 186, ersW, 10, COLOR_YELLOW);
        gfx->fillRect(11 + ersW, 186, 298 - ersW, 10, COLOR_BLACK);
        lErs = ers;
    }

    // 5. DELTA (Rechts unten)
    if (abs(delta - lDelta) > 0.05) {
        gfx->setTextSize(2);
        gfx->setCursor(245, 210);
        gfx->setTextColor(COLOR_BLACK);
        gfx->print(String(lDelta, 2));
        gfx->setCursor(245, 210);
        gfx->setTextColor(delta <= 0 ? COLOR_GREEN : COLOR_RED);
        gfx->print(String(delta, 2));
        lDelta = delta;
    }

    // 6. FUEL (Links unten)
    if (abs(fuel - lFuel) > 0.1) {
        gfx->setTextSize(2);
        gfx->setCursor(15, 210);
        gfx->setTextColor(COLOR_BLACK);
        gfx->print("F: " + String(lFuel, 1));
        gfx->setCursor(15, 210);
        gfx->setTextColor(COLOR_WHITE);
        gfx->print("F: " + String(fuel, 1));
        lFuel = fuel;
    }
}

void drawTyreDash(uint8_t t[], uint8_t w[]) {
    // Reifen Positionen
    int x[4] = {45, 195, 45, 195};
    int y[4] = {45, 45, 135, 135};

    for (int i = 0; i < 4; i++) {
        if (t[i] != lT[i] || w[i] != lWear[i]) {
            uint16_t c = (t[i] > 100) ? COLOR_RED : (t[i] < 78 ? COLOR_BLUE : COLOR_GREEN);
            gfx->fillRoundRect(x[i], y[i], 85, 80, 8, c);
            
            gfx->setTextColor(COLOR_BLACK);
            gfx->setTextSize(3); 
            gfx->setCursor(x[i] + 20, y[i] + 15); 
            gfx->print(String(t[i])); // Temperatur
            
            gfx->setTextSize(2); 
            gfx->setCursor(x[i] + 18, y[i] + 50); 
            gfx->print(String(w[i]) + "%"); // Verschleiß
            lT[i] = t[i]; lWear[i] = w[i];
        }
    }
}