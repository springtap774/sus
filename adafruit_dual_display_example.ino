/*
 * Adafruit_ILI9341 Dual Display Example
 * 
 * This example demonstrates proper SPI management for multiple devices:
 * - Two ILI9341 displays
 * - XPT2046 touch controller 
 * - SD card
 * 
 * Key Features:
 * ✅ Proper CS pin isolation
 * ✅ Safe SPI device switching
 * ✅ Separate display instances (advantage of Adafruit over TFT_eSPI)
 * ✅ No SPI conflicts or white screens
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <SD.h>

// Pin definitions matching your setup
#define TFT1_CS 5    // Top display
#define TFT1_DC 2
#define TFT2_CS 17   // Bottom display with touch
#define TFT2_DC 16
#define TOUCH_CS 4
#define SD_CS 15
#define TFT_RST 21   // Shared reset

// Hardware SPI pins (ESP32)
#define MOSI 23
#define MISO 19
#define SCK 18

// ✅ Create separate instances for each display
Adafruit_ILI9341 tft1(TFT1_CS, TFT1_DC, TFT_RST);
Adafruit_ILI9341 tft2(TFT2_CS, TFT2_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS);

// ✅ CRITICAL: Helper function for safe SPI device switching
void setActiveSPI(uint8_t activeCS) {
  // First, ensure ALL CS lines are HIGH
  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  
  delayMicroseconds(10);  // Clean transition
  
  // Now activate the desired device
  digitalWrite(activeCS, LOW);
  delayMicroseconds(10);
}

void deselectAllSPI() {
  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  delayMicroseconds(10);
}

void setup() {
  Serial.begin(115200);
  
  // Initialize CS pins
  pinMode(TFT1_CS, OUTPUT);
  pinMode(TFT2_CS, OUTPUT);
  pinMode(TOUCH_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  
  // ✅ CRITICAL: Set all CS HIGH before any SPI operations
  deselectAllSPI();
  
  // Reset displays
  digitalWrite(TFT_RST, LOW);
  delay(100);
  digitalWrite(TFT_RST, HIGH);
  delay(100);
  
  // Initialize SPI
  SPI.begin(SCK, MISO, MOSI, -1);
  SPI.setFrequency(27000000); // 27MHz safe for ILI9341
  
  // ✅ Initialize Display 1
  Serial.println("Initializing Display 1...");
  setActiveSPI(TFT1_CS);
  tft1.begin();
  tft1.setRotation(0);
  tft1.fillScreen(ILI9341_BLACK);
  tft1.setTextColor(ILI9341_WHITE);
  tft1.setTextSize(2);
  tft1.setCursor(10, 10);
  tft1.print("Display 1 OK!");
  tft1.drawRect(20, 50, 200, 100, ILI9341_BLUE);
  deselectAllSPI();
  
  // ✅ Initialize Display 2  
  Serial.println("Initializing Display 2...");
  setActiveSPI(TFT2_CS);
  tft2.begin();
  tft2.setRotation(0);
  tft2.fillScreen(ILI9341_BLACK);
  tft2.setTextColor(ILI9341_GREEN);
  tft2.setTextSize(2);
  tft2.setCursor(10, 10);
  tft2.print("Display 2 OK!");
  tft2.drawRect(20, 50, 200, 100, ILI9341_RED);
  deselectAllSPI();
  
  // ✅ Initialize Touch (only on Display 2)
  Serial.println("Initializing Touch...");
  setActiveSPI(TOUCH_CS);
  ts.begin();
  ts.setRotation(0);
  deselectAllSPI();
  
  // ✅ Initialize SD Card
  Serial.println("Initializing SD Card...");
  setActiveSPI(SD_CS);
  if (SD.begin(SD_CS)) {
    Serial.println("SD Card OK!");
  } else {
    Serial.println("SD Card Failed!");
  }
  deselectAllSPI();
  
  Serial.println("Setup Complete!");
}

void loop() {
  // Example: Alternate updates between displays
  static unsigned long lastUpdate = 0;
  static bool useDisplay1 = true;
  static int counter = 0;
  
  if (millis() - lastUpdate > 2000) {
    lastUpdate = millis();
    counter++;
    
    if (useDisplay1) {
      // ✅ Update Display 1
      setActiveSPI(TFT1_CS);
      tft1.fillRect(50, 200, 140, 30, ILI9341_BLACK);
      tft1.setCursor(50, 200);
      tft1.setTextColor(ILI9341_CYAN);
      tft1.print("Count: ");
      tft1.print(counter);
      deselectAllSPI();
    } else {
      // ✅ Update Display 2
      setActiveSPI(TFT2_CS);
      tft2.fillRect(50, 200, 140, 30, ILI9341_BLACK);
      tft2.setCursor(50, 200);
      tft2.setTextColor(ILI9341_YELLOW);
      tft2.print("Count: ");
      tft2.print(counter);
      deselectAllSPI();
    }
    
    useDisplay1 = !useDisplay1;
  }
  
  // ✅ Check touch input (safely isolated)
  setActiveSPI(TOUCH_CS);
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    deselectAllSPI();
    
    // Map coordinates and display on Display 2
    int x = map(p.x, 200, 3700, 0, 240);
    int y = map(p.y, 200, 3700, 0, 320);
    
    setActiveSPI(TFT2_CS);
    tft2.fillRect(10, 250, 220, 30, ILI9341_BLACK);
    tft2.setCursor(10, 250);
    tft2.setTextColor(ILI9341_WHITE);
    tft2.print("Touch: ");
    tft2.print(x);
    tft2.print(",");
    tft2.print(y);
    deselectAllSPI();
  } else {
    deselectAllSPI();
  }
  
  delay(50);
}

/* 
 * ✅ BEST PRACTICES SUMMARY:
 * 
 * 1. Always use setActiveSPI(CS_PIN) before accessing a device
 * 2. Always call deselectAllSPI() when done
 * 3. Never have two CS lines LOW simultaneously
 * 4. Use separate Adafruit_ILI9341 instances for each display
 * 5. Keep SPI frequency at 27MHz max for stability
 * 6. Don't access touch while SD/displays are active
 * 7. Don't try to update both displays simultaneously
 * 
 * ❌ AVOID:
 * - Manual digitalWrite(CS, LOW/HIGH) without proper isolation
 * - Reading touch while SD file is open
 * - Simultaneous display updates
 * - Frequencies over 27MHz
 * - Using TFT_eSPI's shared instance approach
 */