/*
 * ESP32 Minimal Display Test - Emergency Version
 * 
 * This is a stripped-down version that tries different approaches
 * to work around initialization issues.
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Pin definitions - DOUBLE CHECK THESE!
#define TFT1_CS 5    
#define TFT1_DC 2
#define TFT2_CS 17   
#define TFT2_DC 16
#define TFT_RST 21   

// Try lower SPI frequency first
#define SPI_FREQUENCY 10000000  // 10MHz instead of 27MHz

// Only test one display at a time initially
Adafruit_ILI9341 tft1(TFT1_CS, TFT1_DC, TFT_RST);

void setup() {
  Serial.begin(115200);
  delay(3000);  // Longer delay for serial monitor
  
  Serial.println("\n=== ESP32 MINIMAL DISPLAY TEST ===");
  Serial.println("Testing single display with conservative settings");
  
  // Method 1: Try with explicit pin setup
  Serial.println("Method 1: Testing with explicit pin setup...");
  if (testMethod1()) {
    Serial.println("✅ Method 1 SUCCESS!");
    runBasicTest();
    return;
  }
  
  // Method 2: Try with different SPI settings
  Serial.println("Method 2: Testing with different SPI settings...");
  if (testMethod2()) {
    Serial.println("✅ Method 2 SUCCESS!");
    runBasicTest();
    return;
  }
  
  // Method 3: Try manual initialization
  Serial.println("Method 3: Testing manual initialization...");
  if (testMethod3()) {
    Serial.println("✅ Method 3 SUCCESS!");
    runBasicTest();
    return;
  }
  
  Serial.println("❌ ALL METHODS FAILED");
  Serial.println("Check hardware connections:");
  Serial.println("  TFT1_CS = GPIO 5");
  Serial.println("  TFT1_DC = GPIO 2");
  Serial.println("  TFT_RST = GPIO 21");
  Serial.println("  MOSI = GPIO 23");
  Serial.println("  MISO = GPIO 19"); 
  Serial.println("  SCK = GPIO 18");
  Serial.println("  Power: 3.3V with adequate current");
}

bool testMethod1() {
  Serial.println("  Setting up pins manually...");
  
  // Manual pin setup
  pinMode(TFT1_CS, OUTPUT);
  pinMode(TFT1_DC, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  pinMode(23, OUTPUT); // MOSI
  pinMode(19, INPUT);  // MISO
  pinMode(18, OUTPUT); // SCK
  
  // Ensure CS is HIGH
  digitalWrite(TFT1_CS, HIGH);
  delay(10);
  
  // Reset sequence
  digitalWrite(TFT_RST, LOW);
  delay(100);
  digitalWrite(TFT_RST, HIGH);
  delay(150);
  
  Serial.println("  Initializing SPI...");
  SPI.begin(18, 19, 23, -1);  // SCK, MISO, MOSI, SS
  SPI.setFrequency(SPI_FREQUENCY);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  
  Serial.println("  Attempting tft1.begin()...");
  Serial.flush();
  
  try {
    tft1.begin();
    return true;
  } catch (...) {
    Serial.println("  Method 1 failed - exception caught");
    return false;
  }
}

bool testMethod2() {
  Serial.println("  Trying ultra-conservative SPI settings...");
  
  // Even lower frequency
  SPI.setFrequency(1000000);  // 1MHz
  delay(100);
  
  // Multiple reset attempts
  for (int i = 0; i < 3; i++) {
    digitalWrite(TFT_RST, LOW);
    delay(200);
    digitalWrite(TFT_RST, HIGH);
    delay(200);
  }
  
  Serial.println("  Attempting tft1.begin() with 1MHz SPI...");
  Serial.flush();
  
  try {
    tft1.begin();
    return true;
  } catch (...) {
    Serial.println("  Method 2 failed");
    return false;
  }
}

bool testMethod3() {
  Serial.println("  Trying without library initialization...");
  
  // Manual SPI test without library
  digitalWrite(TFT1_CS, LOW);
  delay(1);
  
  // Send simple command
  SPI.transfer(0x01);  // Software reset
  delay(1);
  
  digitalWrite(TFT1_CS, HIGH);
  delay(120);
  
  // Try again with wake up
  digitalWrite(TFT1_CS, LOW);
  delay(1);
  SPI.transfer(0x11);  // Sleep out
  delay(1);
  digitalWrite(TFT1_CS, HIGH);
  delay(120);
  
  Serial.println("  Now attempting tft1.begin()...");
  Serial.flush();
  
  try {
    tft1.begin();
    return true;
  } catch (...) {
    Serial.println("  Method 3 failed");
    return false;
  }
}

void runBasicTest() {
  Serial.println("Running basic functionality test...");
  
  // Test basic drawing
  tft1.fillScreen(ILI9341_BLACK);
  delay(100);
  
  tft1.fillScreen(ILI9341_RED);
  delay(500);
  
  tft1.fillScreen(ILI9341_GREEN);
  delay(500);
  
  tft1.fillScreen(ILI9341_BLUE);
  delay(500);
  
  tft1.fillScreen(ILI9341_BLACK);
  tft1.setTextColor(ILI9341_WHITE);
  tft1.setTextSize(2);
  tft1.setCursor(10, 10);
  tft1.print("ESP32 TEST");
  tft1.setCursor(10, 40);
  tft1.print("TFT1 WORKS!");
  
  Serial.println("✅ Basic test completed successfully!");
  Serial.println("TFT1 is working. You can now test TFT2.");
  
  // Test pattern
  for (int i = 0; i < 240; i += 20) {
    tft1.drawLine(0, 0, i, 319, ILI9341_CYAN);
    delay(50);
  }
}

void loop() {
  // Keep running basic animations to verify display works
  static unsigned long lastUpdate = 0;
  static int color = 0;
  
  if (millis() - lastUpdate > 2000) {
    lastUpdate = millis();
    
    uint16_t colors[] = {ILI9341_RED, ILI9341_GREEN, ILI9341_BLUE, ILI9341_YELLOW, ILI9341_CYAN, ILI9341_MAGENTA};
    
    tft1.fillRect(200, 280, 30, 30, colors[color % 6]);
    color++;
    
    Serial.println("Display still working... Color block updated.");
  }
}

/*
 * QUICK TROUBLESHOOTING:
 * 
 * If this ALSO hangs:
 * 1. Check power supply (measure 3.3V with multimeter)
 * 2. Verify GPIO connections with multimeter
 * 3. Try different GPIO pins for CS/DC
 * 4. Check if displays work individually (disconnect one)
 * 5. Try with a single display module first
 * 
 * Alternative pin assignments to try:
 * TFT1_CS = GPIO 15 (instead of 5)
 * TFT1_DC = GPIO 4 (instead of 2)
 * 
 * Common issues:
 * - GPIO 2 used by bootloader (try GPIO 4 for DC)
 * - GPIO 5 conflicts (try GPIO 15)
 * - Insufficient power (each display needs 20-40mA)
 * - Bad solder joints on display modules
 */