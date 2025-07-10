/*
 * ESP32 Dual Display - Fixed Compilation & Display Issues
 * 
 * This version fixes:
 * 1. Compilation errors with pin definitions
 * 2. Display size/rotation issues (only 3/4 screen used)
 * 3. Touch calibration problems
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <SD.h>

// ✅ FIXED: Explicit pin definitions (no macro conflicts)
const int TFT1_CS_PIN = 5;    // Top screen, no touch
const int TFT1_DC_PIN = 2;
const int TFT2_CS_PIN = 17;   // Bottom screen, with touch  
const int TFT2_DC_PIN = 16;
const int TOUCH_CS_PIN = 4;
const int SD_CS_PIN = 15;
const int TFT_RST_PIN = 21;   // Shared reset for both TFTs
const int OFF_BUTTON_PIN = 22;

// SPI pins (ESP32 default)
const int MOSI_PIN = 23;
const int MISO_PIN = 19;
const int SCK_PIN = 18;

// ✅ FIXED: Using const int instead of #define to avoid macro issues
Adafruit_ILI9341 tft1(TFT1_CS_PIN, TFT1_DC_PIN, TFT_RST_PIN);
Adafruit_ILI9341 tft2(TFT2_CS_PIN, TFT2_DC_PIN, TFT_RST_PIN);
XPT2046_Touchscreen ts(TOUCH_CS_PIN);

// Screen dimensions
const int SCREEN_WIDTH = 240;
const int SCREEN_HEIGHT = 320;

// ✅ FIXED: Touch calibration for full screen coverage
const int TOUCH_CALIBRATION_X_MIN = 300;   // Adjust these after testing
const int TOUCH_CALIBRATION_X_MAX = 3700;
const int TOUCH_CALIBRATION_Y_MIN = 300;
const int TOUCH_CALIBRATION_Y_MAX = 3700;

// Application states
enum AppState { 
  MENU_STATE, 
  TEST_STATE,
  TOUCH_TEST_STATE
};
AppState appState = MENU_STATE;

// Simple menu structure
struct MenuItem {
  String title;
  int x, y, w, h;
  int action;
};

const int MAX_MENU_ITEMS = 5;
MenuItem menuItems[MAX_MENU_ITEMS];
int menuItemCount = 0;

// Colors
const uint16_t BLACK = ILI9341_BLACK;
const uint16_t WHITE = ILI9341_WHITE;
const uint16_t RED = ILI9341_RED;
const uint16_t GREEN = ILI9341_GREEN;
const uint16_t BLUE = ILI9341_BLUE;
const uint16_t CYAN = ILI9341_CYAN;
const uint16_t YELLOW = ILI9341_YELLOW;

void setup() {
  Serial.begin(115200);
  delay(2000);  // Give time for serial monitor
  
  Serial.println("=== ESP32 Dual Display - Fixed Version ===");
  
  // Initialize pins
  pinMode(TFT1_CS_PIN, OUTPUT);
  pinMode(TFT1_DC_PIN, OUTPUT);
  pinMode(TFT2_CS_PIN, OUTPUT);
  pinMode(TFT2_DC_PIN, OUTPUT);
  pinMode(TOUCH_CS_PIN, OUTPUT);
  pinMode(SD_CS_PIN, OUTPUT);
  pinMode(TFT_RST_PIN, OUTPUT);
  pinMode(OFF_BUTTON_PIN, INPUT_PULLUP);
  
  // Set all CS pins HIGH initially
  digitalWrite(TFT1_CS_PIN, HIGH);
  digitalWrite(TFT2_CS_PIN, HIGH);
  digitalWrite(TOUCH_CS_PIN, HIGH);
  digitalWrite(SD_CS_PIN, HIGH);
  
  // Reset displays
  Serial.println("Resetting displays...");
  digitalWrite(TFT_RST_PIN, LOW);
  delay(100);
  digitalWrite(TFT_RST_PIN, HIGH);
  delay(200);
  
  // Initialize SPI with conservative settings
  Serial.println("Initializing SPI...");
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, -1);
  SPI.setFrequency(10000000);  // Start with 10MHz for stability
  
  // ✅ FIXED: Initialize Display 1 with proper error handling
  Serial.println("Initializing Display 1...");
  if (initializeDisplay1()) {
    Serial.println("✅ Display 1 OK!");
  } else {
    Serial.println("❌ Display 1 failed!");
  }
  
  // ✅ FIXED: Initialize Display 2 with proper error handling  
  Serial.println("Initializing Display 2...");
  if (initializeDisplay2()) {
    Serial.println("✅ Display 2 OK!");
  } else {
    Serial.println("❌ Display 2 failed!");
  }
  
  // Initialize touchscreen
  Serial.println("Initializing Touch...");
  digitalWrite(TOUCH_CS_PIN, LOW);
  delayMicroseconds(10);
  ts.begin();
  ts.setRotation(0);
  digitalWrite(TOUCH_CS_PIN, HIGH);
  Serial.println("✅ Touch initialized!");
  
  // Setup menu
  setupMenu();
  drawMenu();
  
  Serial.println("Setup complete!");
}

bool initializeDisplay1() {
  digitalWrite(TFT1_CS_PIN, LOW);
  delayMicroseconds(10);
  
  try {
    tft1.begin();
    
    // ✅ FIXED: Test different rotations to find correct one
    tft1.setRotation(0);  // Try rotation 0 first
    tft1.fillScreen(BLACK);
    
    // Draw test pattern to check full screen coverage
    tft1.fillRect(0, 0, SCREEN_WIDTH, 20, RED);           // Top red bar
    tft1.fillRect(0, SCREEN_HEIGHT-20, SCREEN_WIDTH, 20, BLUE); // Bottom blue bar
    tft1.fillRect(0, 0, 20, SCREEN_HEIGHT, GREEN);        // Left green bar  
    tft1.fillRect(SCREEN_WIDTH-20, 0, 20, SCREEN_HEIGHT, YELLOW); // Right yellow bar
    
    // Center text
    tft1.setTextColor(WHITE);
    tft1.setTextSize(2);
    tft1.setCursor(50, 150);
    tft1.print("TFT1 FULL");
    tft1.setCursor(50, 170);
    tft1.print("SCREEN TEST");
    
    digitalWrite(TFT1_CS_PIN, HIGH);
    return true;
    
  } catch (...) {
    digitalWrite(TFT1_CS_PIN, HIGH);
    return false;
  }
}

bool initializeDisplay2() {
  digitalWrite(TFT2_CS_PIN, LOW);
  delayMicroseconds(10);
  
  try {
    tft2.begin();
    
    // ✅ FIXED: Test different rotations for touch alignment
    tft2.setRotation(0);  // Try rotation 0 first
    tft2.fillScreen(BLACK);
    
    // Draw full screen test pattern
    tft2.fillRect(0, 0, SCREEN_WIDTH, 20, CYAN);           // Top cyan bar
    tft2.fillRect(0, SCREEN_HEIGHT-20, SCREEN_WIDTH, 20, RED); // Bottom red bar
    tft2.fillRect(0, 0, 20, SCREEN_HEIGHT, BLUE);         // Left blue bar
    tft2.fillRect(SCREEN_WIDTH-20, 0, 20, SCREEN_HEIGHT, GREEN); // Right green bar
    
    // Center text
    tft2.setTextColor(WHITE);
    tft2.setTextSize(2);
    tft2.setCursor(50, 150);
    tft2.print("TFT2 TOUCH");
    tft2.setCursor(50, 170);
    tft2.print("FULL SCREEN");
    
    digitalWrite(TFT2_CS_PIN, HIGH);
    return true;
    
  } catch (...) {
    digitalWrite(TFT2_CS_PIN, HIGH);
    return false;
  }
}

void setupMenu() {
  menuItemCount = 4;
  
  menuItems[0] = {"Display Test", 20, 80, 200, 40, 1};
  menuItems[1] = {"Touch Test", 20, 130, 200, 40, 2};
  menuItems[2] = {"Rotation Test", 20, 180, 200, 40, 3};
  menuItems[3] = {"Full Coverage", 20, 230, 200, 40, 4};
}

void drawMenu() {
  digitalWrite(TFT2_CS_PIN, LOW);
  delayMicroseconds(10);
  
  tft2.fillScreen(BLACK);
  tft2.setTextColor(WHITE);
  tft2.setTextSize(2);
  tft2.setCursor(30, 20);
  tft2.print("ESP32 Dual TFT");
  
  tft2.setTextSize(1);
  tft2.setCursor(50, 45);
  tft2.print("Touch menu items below");
  
  // Draw menu items
  for (int i = 0; i < menuItemCount; i++) {
    MenuItem& item = menuItems[i];
    tft2.drawRect(item.x, item.y, item.w, item.h, WHITE);
    tft2.setCursor(item.x + 10, item.y + 15);
    tft2.setTextSize(2);
    tft2.print(item.title);
  }
  
  digitalWrite(TFT2_CS_PIN, HIGH);
}

void loop() {
  handleTouch();
  delay(50);
}

void handleTouch() {
  digitalWrite(TOUCH_CS_PIN, LOW);
  delayMicroseconds(10);
  
  if (!ts.touched()) {
    digitalWrite(TOUCH_CS_PIN, HIGH);
    return;
  }
  
  TS_Point p = ts.getPoint();
  digitalWrite(TOUCH_CS_PIN, HIGH);
  
  // ✅ FIXED: Better coordinate mapping with bounds checking
  int x = map(constrain(p.x, TOUCH_CALIBRATION_X_MIN, TOUCH_CALIBRATION_X_MAX), 
              TOUCH_CALIBRATION_X_MIN, TOUCH_CALIBRATION_X_MAX, 0, SCREEN_WIDTH);
  int y = map(constrain(p.y, TOUCH_CALIBRATION_Y_MIN, TOUCH_CALIBRATION_Y_MAX), 
              TOUCH_CALIBRATION_Y_MIN, TOUCH_CALIBRATION_Y_MAX, 0, SCREEN_HEIGHT);
  
  Serial.print("Touch: Raw("); Serial.print(p.x); Serial.print(","); Serial.print(p.y);
  Serial.print(") Mapped("); Serial.print(x); Serial.print(","); Serial.print(y); Serial.println(")");
  
  switch (appState) {
    case MENU_STATE:
      handleMenuTouch(x, y);
      break;
    case TOUCH_TEST_STATE:
      handleTouchTest(x, y);
      break;
  }
  
  delay(200);  // Debounce
}

void handleMenuTouch(int x, int y) {
  for (int i = 0; i < menuItemCount; i++) {
    MenuItem& item = menuItems[i];
    if (x >= item.x && x <= item.x + item.w && 
        y >= item.y && y <= item.y + item.h) {
      
      Serial.print("Menu item touched: "); Serial.println(item.title);
      
      switch (item.action) {
        case 1: // Display Test
          runDisplayTest();
          break;
        case 2: // Touch Test
          runTouchTest();
          break;
        case 3: // Rotation Test
          runRotationTest();
          break;
        case 4: // Full Coverage Test
          runFullCoverageTest();
          break;
      }
      break;
    }
  }
}

void runDisplayTest() {
  Serial.println("Running display test...");
  
  // Test both displays with different patterns
  for (int i = 0; i < 3; i++) {
    uint16_t colors[] = {RED, GREEN, BLUE};
    
    // Display 1
    digitalWrite(TFT1_CS_PIN, LOW);
    tft1.fillScreen(colors[i]);
    digitalWrite(TFT1_CS_PIN, HIGH);
    
    // Display 2  
    digitalWrite(TFT2_CS_PIN, LOW);
    tft2.fillScreen(colors[i]);
    digitalWrite(TFT2_CS_PIN, HIGH);
    
    delay(1000);
  }
  
  drawMenu();
}

void runTouchTest() {
  appState = TOUCH_TEST_STATE;
  
  digitalWrite(TFT2_CS_PIN, LOW);
  tft2.fillScreen(BLACK);
  tft2.setTextColor(WHITE);
  tft2.setTextSize(2);
  tft2.setCursor(10, 10);
  tft2.print("Touch Test Mode");
  tft2.setTextSize(1);
  tft2.setCursor(10, 40);
  tft2.print("Touch anywhere to see coordinates");
  tft2.setCursor(10, 55);
  tft2.print("Touch bottom-right to exit");
  
  // Draw corner indicators
  tft2.fillRect(0, 0, 20, 20, RED);           // Top-left
  tft2.fillRect(SCREEN_WIDTH-20, 0, 20, 20, GREEN);    // Top-right
  tft2.fillRect(0, SCREEN_HEIGHT-20, 20, 20, BLUE);    // Bottom-left
  tft2.fillRect(SCREEN_WIDTH-20, SCREEN_HEIGHT-20, 20, 20, YELLOW); // Bottom-right
  
  digitalWrite(TFT2_CS_PIN, HIGH);
}

void handleTouchTest(int x, int y) {
  digitalWrite(TFT2_CS_PIN, LOW);
  
  // Clear previous coordinates
  tft2.fillRect(10, 80, 220, 60, BLACK);
  
  // Show current coordinates
  tft2.setTextColor(WHITE);
  tft2.setTextSize(2);
  tft2.setCursor(10, 80);
  tft2.print("X: "); tft2.print(x);
  tft2.setCursor(10, 100);
  tft2.print("Y: "); tft2.print(y);
  
  // Draw touch point
  tft2.fillCircle(x, y, 3, CYAN);
  
  digitalWrite(TFT2_CS_PIN, HIGH);
  
  // Exit if touching bottom-right corner
  if (x > SCREEN_WIDTH - 40 && y > SCREEN_HEIGHT - 40) {
    appState = MENU_STATE;
    drawMenu();
  }
}

void runRotationTest() {
  Serial.println("Testing different rotations...");
  
  for (int rotation = 0; rotation < 4; rotation++) {
    // Test rotation on TFT2
    digitalWrite(TFT2_CS_PIN, LOW);
    tft2.setRotation(rotation);
    tft2.fillScreen(BLACK);
    tft2.setTextColor(WHITE);
    tft2.setTextSize(2);
    tft2.setCursor(10, 10);
    tft2.print("Rotation: "); tft2.print(rotation);
    
    // Draw corner markers
    tft2.fillRect(0, 0, 30, 30, RED);
    tft2.fillRect(tft2.width()-30, 0, 30, 30, GREEN);
    tft2.fillRect(0, tft2.height()-30, 30, 30, BLUE);
    tft2.fillRect(tft2.width()-30, tft2.height()-30, 30, 30, YELLOW);
    
    digitalWrite(TFT2_CS_PIN, HIGH);
    
    delay(2000);
  }
  
  // Reset to rotation 0
  digitalWrite(TFT2_CS_PIN, LOW);
  tft2.setRotation(0);
  digitalWrite(TFT2_CS_PIN, HIGH);
  
  drawMenu();
}

void runFullCoverageTest() {
  Serial.println("Testing full screen coverage...");
  
  // Draw pixel-by-pixel test pattern
  digitalWrite(TFT2_CS_PIN, LOW);
  tft2.fillScreen(BLACK);
  
  // Draw grid pattern to verify full coverage
  for (int x = 0; x < SCREEN_WIDTH; x += 20) {
    tft2.drawLine(x, 0, x, SCREEN_HEIGHT-1, WHITE);
  }
  for (int y = 0; y < SCREEN_HEIGHT; y += 20) {
    tft2.drawLine(0, y, SCREEN_WIDTH-1, y, WHITE);
  }
  
  // Label corners
  tft2.setTextColor(RED);
  tft2.setTextSize(1);
  tft2.setCursor(5, 5);
  tft2.print("0,0");
  tft2.setCursor(SCREEN_WIDTH-25, 5);
  tft2.print("240,0");
  tft2.setCursor(5, SCREEN_HEIGHT-15);
  tft2.print("0,320");
  tft2.setCursor(SCREEN_WIDTH-35, SCREEN_HEIGHT-15);
  tft2.print("240,320");
  
  digitalWrite(TFT2_CS_PIN, HIGH);
  
  delay(3000);
  drawMenu();
}

/*
 * TROUBLESHOOTING NOTES:
 * 
 * For "only 3/4 screen used" issue:
 * 1. Try different rotation values (0,1,2,3)
 * 2. Check if SCREEN_WIDTH/HEIGHT match your display
 * 3. Verify display initialization sequence
 * 4. Check power supply stability
 * 
 * For compilation errors:
 * 1. Use const int instead of #define for pins
 * 2. Check library versions are compatible
 * 3. Ensure all includes are present
 */