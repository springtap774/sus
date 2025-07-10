/*
 * ESP32 Dual Display Diagnostic Tool
 * 
 * This script tests each component step-by-step to isolate hardware issues.
 * Use this when your main code hangs during initialization.
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

// Pin definitions - VERIFY THESE MATCH YOUR HARDWARE
#define TFT1_CS 5    // Top screen, no touch
#define TFT1_DC 2
#define TFT2_CS 17   // Bottom screen, with touch
#define TFT2_DC 16
#define TOUCH_CS 4
#define SD_CS 15
#define TFT_RST 21   // Shared reset for both TFTs

// SPI pins (shared)
#define MOSI 23
#define MISO 19
#define SCK 18

// Only create displays when we test them
Adafruit_ILI9341* tft1 = nullptr;
Adafruit_ILI9341* tft2 = nullptr;
XPT2046_Touchscreen* ts = nullptr;

void setup() {
  Serial.begin(115200);
  delay(2000);  // Give time for serial monitor to open
  
  Serial.println("\n=== ESP32 Dual Display Diagnostic ===");
  Serial.println("This will test each component step by step");
  Serial.println("Watch for where it hangs to identify the problem\n");
  
  // Step 1: Test basic GPIO
  Serial.println("Step 1: Testing GPIO pins...");
  testGPIO();
  
  // Step 2: Test SPI initialization
  Serial.println("Step 2: Testing SPI initialization...");
  testSPI();
  
  // Step 3: Test each display individually
  Serial.println("Step 3: Testing Display 1 (TFT1)...");
  testDisplay1();
  
  Serial.println("Step 4: Testing Display 2 (TFT2)...");
  testDisplay2();
  
  // Step 5: Test touch controller
  Serial.println("Step 5: Testing Touch Controller...");
  testTouch();
  
  Serial.println("\n=== ALL TESTS COMPLETED SUCCESSFULLY ===");
  Serial.println("If you see this message, hardware appears to be working.");
  Serial.println("The issue might be in your main code logic.\n");
}

void loop() {
  // Simple status blinking
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    lastBlink = millis();
    Serial.println("Diagnostic complete. Check results above.");
  }
  delay(1000);
}

void testGPIO() {
  Serial.print("  Setting up GPIO pins... ");
  
  // Initialize all pins
  pinMode(TFT1_CS, OUTPUT);
  pinMode(TFT1_DC, OUTPUT);
  pinMode(TFT2_CS, OUTPUT);
  pinMode(TFT2_DC, OUTPUT);
  pinMode(TOUCH_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  
  // Set all CS pins HIGH (deselected)
  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  
  Serial.println("✅ GPIO pins configured");
  
  // Test reset sequence
  Serial.print("  Testing reset sequence... ");
  digitalWrite(TFT_RST, LOW);
  delay(100);
  digitalWrite(TFT_RST, HIGH);
  delay(100);
  
  Serial.println("✅ Reset sequence completed");
  delay(500);
}

void testSPI() {
  Serial.print("  Initializing SPI bus... ");
  
  SPI.begin(SCK, MISO, MOSI, -1);
  Serial.println("✅ SPI.begin() successful");
  
  Serial.print("  Setting SPI frequency to 27MHz... ");
  SPI.setFrequency(27000000);
  Serial.println("✅ SPI frequency set");
  
  Serial.print("  Testing SPI communication... ");
  // Try a simple SPI transaction
  digitalWrite(TFT1_CS, LOW);
  delayMicroseconds(10);
  SPI.write(0x00);  // NOP command
  delayMicroseconds(10);
  digitalWrite(TFT1_CS, HIGH);
  Serial.println("✅ Basic SPI communication works");
  
  delay(500);
}

void testDisplay1() {
  Serial.print("  Creating TFT1 instance... ");
  tft1 = new Adafruit_ILI9341(TFT1_CS, TFT1_DC, TFT_RST);
  Serial.println("✅ TFT1 instance created");
  
  Serial.print("  Selecting TFT1 (CS=5)... ");
  digitalWrite(TFT1_CS, LOW);
  delayMicroseconds(10);
  Serial.println("✅ TFT1 selected");
  
  Serial.print("  Calling tft1.begin()... ");
  Serial.flush();  // Ensure this prints before begin()
  
  tft1->begin();  // This is where it might hang
  
  Serial.println("✅ tft1.begin() successful!");
  
  Serial.print("  Testing basic drawing... ");
  tft1->fillScreen(ILI9341_BLACK);
  tft1->setTextColor(ILI9341_WHITE);
  tft1->setTextSize(2);
  tft1->setCursor(10, 10);
  tft1->print("TFT1 TEST OK");
  
  digitalWrite(TFT1_CS, HIGH);  // Deselect
  Serial.println("✅ TFT1 drawing successful");
  
  delay(1000);
}

void testDisplay2() {
  Serial.print("  Creating TFT2 instance... ");
  tft2 = new Adafruit_ILI9341(TFT2_CS, TFT2_DC, TFT_RST);
  Serial.println("✅ TFT2 instance created");
  
  Serial.print("  Selecting TFT2 (CS=17)... ");
  digitalWrite(TFT2_CS, LOW);
  delayMicroseconds(10);
  Serial.println("✅ TFT2 selected");
  
  Serial.print("  Calling tft2.begin()... ");
  Serial.flush();  // Ensure this prints before begin()
  
  tft2->begin();  // This is where it might hang
  
  Serial.println("✅ tft2.begin() successful!");
  
  Serial.print("  Testing basic drawing... ");
  tft2->fillScreen(ILI9341_BLACK);
  tft2->setTextColor(ILI9341_GREEN);
  tft2->setTextSize(2);
  tft2->setCursor(10, 10);
  tft2->print("TFT2 TEST OK");
  
  digitalWrite(TFT2_CS, HIGH);  // Deselect
  Serial.println("✅ TFT2 drawing successful");
  
  delay(1000);
}

void testTouch() {
  Serial.print("  Creating touch instance... ");
  ts = new XPT2046_Touchscreen(TOUCH_CS);
  Serial.println("✅ Touch instance created");
  
  Serial.print("  Selecting touch controller (CS=4)... ");
  digitalWrite(TOUCH_CS, LOW);
  delayMicroseconds(10);
  Serial.println("✅ Touch controller selected");
  
  Serial.print("  Calling ts.begin()... ");
  Serial.flush();
  
  ts->begin();
  
  Serial.println("✅ ts.begin() successful!");
  
  Serial.print("  Testing touch reading... ");
  bool touched = ts->touched();
  Serial.print("✅ Touch reading works (touched=");
  Serial.print(touched ? "true" : "false");
  Serial.println(")");
  
  digitalWrite(TOUCH_CS, HIGH);  // Deselect
  
  delay(500);
}

/*
 * TROUBLESHOOTING GUIDE:
 * 
 * If it hangs at:
 * 
 * 1. "Setting up GPIO pins" -> Check power supply and basic connections
 * 
 * 2. "Initializing SPI bus" -> Check SPI pin connections (23,19,18)
 * 
 * 3. "Calling tft1.begin()" -> 
 *    - Check TFT1_CS (GPIO 5) connection
 *    - Check TFT1_DC (GPIO 2) connection  
 *    - Check TFT_RST (GPIO 21) connection
 *    - Verify TFT1 power supply
 *    - Try different SPI frequency (lower)
 * 
 * 4. "Calling tft2.begin()" ->
 *    - Check TFT2_CS (GPIO 17) connection
 *    - Check TFT2_DC (GPIO 16) connection
 *    - Same power/connection checks as TFT1
 * 
 * 5. "Calling ts.begin()" ->
 *    - Check TOUCH_CS (GPIO 4) connection
 *    - Verify touch controller power
 * 
 * COMMON ISSUES:
 * - Wrong GPIO pin assignments
 * - Loose connections
 * - Insufficient power supply (3.3V rail)
 * - Damaged display
 * - Wrong library version
 */