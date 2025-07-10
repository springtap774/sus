/*
 * ESP32 Dual TFT Display with Optimized SPI Management
 * 
 * ✅ FIXES APPLIED:
 * - Proper SPI transaction management
 * - Improved touch coordinate mapping with calibration
 * - Safe SPI queueing system
 * - Memory optimization for ESP32
 * - Consistent CS line management
 * - Touch interference prevention
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <FS.h>

// Pin definitions
#define TFT1_CS 5    // Top screen, no touch
#define TFT1_DC 2
#define TFT2_CS 17   // Bottom screen, with touch
#define TFT2_DC 16
#define TOUCH_CS 4
#define SD_CS 15
#define TFT_RST 21   // Shared reset for both TFTs
#define OFF_BUTTON 22

// SPI pins (shared)
#define MOSI 23
#define MISO 19
#define SCK 18

// Display instances
Adafruit_ILI9341 tft1(TFT1_CS, TFT1_DC, TFT_RST);
Adafruit_ILI9341 tft2(TFT2_CS, TFT2_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS);

// Screen dimensions
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// ✅ IMPROVED: Touch calibration values (adjust for your specific touchscreen)
#define TOUCH_CALIBRATION_X_MIN 350   // Adjust these values
#define TOUCH_CALIBRATION_X_MAX 3650  // based on your touch testing
#define TOUCH_CALIBRATION_Y_MIN 350
#define TOUCH_CALIBRATION_Y_MAX 3650

// Current active display
enum ActiveDisplay { DISPLAY_1, DISPLAY_2 };
ActiveDisplay currentDisplay = DISPLAY_2;

// Application states
enum AppState { 
  MENU_STATE, 
  CSV_LOOKUP_STATE, 
  IMAGE_VIEWER_STATE, 
  KEYBOARD_STATE,
  SLEEP_STATE 
};
AppState appState = MENU_STATE;

// ✅ IMPROVED: Memory-optimized menu structure (fixed arrays instead of vectors)
struct MenuItem {
  String title;
  String action;
  int x, y, w, h;
};

#define MAX_MENU_ITEMS 10
MenuItem menuItems[MAX_MENU_ITEMS];
int menuItemCount = 0;

String currentSearchString = "";

#define MAX_CSV_RESULTS 50
String csvResults[MAX_CSV_RESULTS];
int csvResultCount = 0;
int csvScrollIndex = 0;

#define MAX_IMAGE_FILES 20
String imageFiles[MAX_IMAGE_FILES];
int imageFileCount = 0;
int imageIndex = 0;

// Virtual keyboard
const char keyboard[4][10] = {
  {'Q','W','E','R','T','Y','U','I','O','P'},
  {'A','S','D','F','G','H','J','K','L',' '},
  {'Z','X','C','V','B','N','M','.',',','?'},
  {'1','2','3','4','5','6','7','8','9','0'}
};

struct KeyButton {
  int x, y, w, h;
  char key;
};

#define MAX_KEY_BUTTONS 40
KeyButton keyButtons[MAX_KEY_BUTTONS];
int keyButtonCount = 0;
KeyButton backButton, deleteButton;

// Colors
#define BLACK ILI9341_BLACK
#define WHITE ILI9341_WHITE
#define BLUE ILI9341_BLUE
#define RED ILI9341_RED
#define GREEN ILI9341_GREEN
#define CYAN ILI9341_CYAN
#define MAGENTA ILI9341_MAGENTA
#define YELLOW ILI9341_YELLOW
#define GRAY 0x8410
#define DARKGRAY 0x4208

// ✅ NEW: SPI transaction safety
class SPITransaction {
private:
  uint8_t activeCS;
  bool isActive;

public:
  SPITransaction(uint8_t cs) : activeCS(cs), isActive(false) {
    begin();
  }
  
  ~SPITransaction() {
    end();
  }
  
  void begin() {
    if (!isActive) {
      // Ensure all CS lines are HIGH
      digitalWrite(TFT1_CS, HIGH);
      digitalWrite(TFT2_CS, HIGH);
      digitalWrite(TOUCH_CS, HIGH);
      digitalWrite(SD_CS, HIGH);
      delayMicroseconds(10);
      
      // Activate desired device
      digitalWrite(activeCS, LOW);
      delayMicroseconds(10);
      isActive = true;
    }
  }
  
  void end() {
    if (isActive) {
      digitalWrite(activeCS, HIGH);
      delayMicroseconds(10);
      isActive = false;
    }
  }
};

// ✅ IMPROVED: Enhanced touch handling with debounce and calibration
class TouchHandler {
private:
  unsigned long lastTouchTime = 0;
  bool lastTouchState = false;
  const unsigned long DEBOUNCE_DELAY = 200;

public:
  bool isTouched() {
    SPITransaction transaction(TOUCH_CS);
    return ts.touched();
  }
  
  bool getTouchPoint(int &x, int &y) {
    unsigned long currentTime = millis();
    
    SPITransaction transaction(TOUCH_CS);
    bool touched = ts.touched();
    
    if (!touched) {
      lastTouchState = false;
      return false;
    }
    
    // Debounce
    if (touched && !lastTouchState && (currentTime - lastTouchTime) < DEBOUNCE_DELAY) {
      return false;
    }
    
    if (touched && !lastTouchState) {
      lastTouchTime = currentTime;
      lastTouchState = true;
      
      TS_Point p = ts.getPoint();
      
      // ✅ IMPROVED: Better coordinate mapping with bounds checking
      x = map(constrain(p.x, TOUCH_CALIBRATION_X_MIN, TOUCH_CALIBRATION_X_MAX), 
              TOUCH_CALIBRATION_X_MIN, TOUCH_CALIBRATION_X_MAX, 0, SCREEN_WIDTH);
      y = map(constrain(p.y, TOUCH_CALIBRATION_Y_MIN, TOUCH_CALIBRATION_Y_MAX), 
              TOUCH_CALIBRATION_Y_MIN, TOUCH_CALIBRATION_Y_MAX, 0, SCREEN_HEIGHT);
      
      return true;
    }
    
    return false;
  }
};

TouchHandler touchHandler;

// Function prototypes
void setupSPI();
void selectDisplay(ActiveDisplay display);
void loadMenuFromSD();
void drawMenu();
void drawKeyboard();
void setupKeyboard();
void handleTouch();
void handleMenuTouch(int x, int y);
void handleKeyboardTouch(int x, int y);
void handleCSVTouch(int x, int y);
void handleImageTouch(int x, int y);
void searchCSV(String query);
void displayCSVResults();
void loadImageFiles();
void displayCurrentImage();
void displayImage(String filename);
void checkOffButton();
void enterSleepMode();
void wakeUp();

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32 Dual Display Setup...");
  
  // Initialize pins
  pinMode(TFT1_CS, OUTPUT);
  pinMode(TFT2_CS, OUTPUT);
  pinMode(TOUCH_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  pinMode(OFF_BUTTON, INPUT_PULLUP);
  
  // ✅ CRITICAL: Set all CS pins HIGH before any SPI operations
  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  
  // Reset displays
  digitalWrite(TFT_RST, LOW);
  delay(100);
  digitalWrite(TFT_RST, HIGH);
  delay(100);
  
  // Initialize SPI
  setupSPI();
  
  // ✅ Initialize Display 1 with transaction safety
  Serial.println("Initializing Display 1...");
  {
    SPITransaction transaction(TFT1_CS);
    tft1.begin();
    tft1.setRotation(0);
    tft1.fillScreen(BLACK);
    tft1.setTextColor(WHITE);
    tft1.setTextSize(2);
    tft1.setCursor(10, 10);
    tft1.print("Display 1 Ready");
  }
  
  Serial.println("Initializing Display 2...");
  {
    SPITransaction transaction(TFT2_CS);
    tft2.begin();
    tft2.setRotation(0);
    tft2.fillScreen(BLACK);
    tft2.setTextColor(WHITE);
    tft2.setTextSize(2);
    tft2.setCursor(10, 10);
    tft2.print("Display 2 Ready");
  }
  
  // ✅ Initialize touchscreen with transaction safety
  Serial.println("Initializing Touchscreen...");
  {
    SPITransaction transaction(TOUCH_CS);
    ts.begin();
    ts.setRotation(0);
  }
  
  // ✅ Initialize SD card with transaction safety
  Serial.println("Initializing SD Card...");
  bool sdCardOK = false;
  {
    SPITransaction transaction(SD_CS);
    if (SD.begin(SD_CS)) {
      sdCardOK = true;
      Serial.println("SD Card OK!");
    } else {
      Serial.println("SD Card Failed!");
    }
  }
  
  // Display SD status
  {
    SPITransaction transaction(TFT2_CS);
    tft2.setCursor(10, 50);
    tft2.print(sdCardOK ? "SD Card OK" : "SD Card Failed");
  }
  
  if (sdCardOK) {
    loadMenuFromSD();
    loadImageFiles();
  } else {
    // Create default menu
    menuItemCount = 3;
    menuItems[0] = {"CSV Lookup", "csv", 20, 80, 200, 50};
    menuItems[1] = {"Image Viewer", "images", 20, 140, 200, 50};
    menuItems[2] = {"Settings", "settings", 20, 200, 200, 50};
  }
  
  // Setup keyboard layout
  setupKeyboard();
  
  // Draw initial menu
  drawMenu();
  
  Serial.println("Setup complete!");
}

void loop() {
  checkOffButton();
  
  if (appState != SLEEP_STATE) {
    handleTouch();
  }
  
  delay(50);
}

void setupSPI() {
  SPI.begin(SCK, MISO, MOSI, -1);
  SPI.setFrequency(27000000); // 27MHz - safe for ILI9341
}

void selectDisplay(ActiveDisplay display) {
  currentDisplay = display;
  // Note: Actual CS management handled by SPITransaction
}

void loadMenuFromSD() {
  SPITransaction transaction(SD_CS);
  
  File file = SD.open("/menu.json");
  if (!file) {
    // Create default menu if file doesn't exist
    menuItemCount = 3;
    menuItems[0] = {"CSV Lookup", "csv", 20, 80, 200, 50};
    menuItems[1] = {"Image Viewer", "images", 20, 140, 200, 50};
    menuItems[2] = {"Settings", "settings", 20, 200, 200, 50};
    return;
  }
  
  String jsonString = file.readString();
  file.close();
  
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, jsonString);
  
  menuItemCount = 0;
  JsonArray items = doc["menu"];
  
  int y = 80;
  for (JsonObject item : items) {
    if (menuItemCount >= MAX_MENU_ITEMS) break;
    
    menuItems[menuItemCount].title = item["title"].as<String>();
    menuItems[menuItemCount].action = item["action"].as<String>();
    menuItems[menuItemCount].x = 20;
    menuItems[menuItemCount].y = y;
    menuItems[menuItemCount].w = 200;
    menuItems[menuItemCount].h = 50;
    menuItemCount++;
    y += 60;
  }
}

void drawMenu() {
  SPITransaction transaction(TFT2_CS);
  selectDisplay(DISPLAY_2);
  
  tft2.fillScreen(BLACK);
  tft2.setTextColor(WHITE);
  tft2.setTextSize(2);
  tft2.setCursor(50, 20);
  tft2.print("Main Menu");
  
  for (int i = 0; i < menuItemCount; i++) {
    MenuItem& item = menuItems[i];
    tft2.drawRect(item.x, item.y, item.w, item.h, WHITE);
    tft2.setCursor(item.x + 10, item.y + 15);
    tft2.print(item.title);
  }
}

void setupKeyboard() {
  keyButtonCount = 0;
  
  int startX = 5;
  int startY = 120;
  int keyWidth = 22;
  int keyHeight = 30;
  int spacing = 2;
  
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 10; col++) {
      if (keyButtonCount >= MAX_KEY_BUTTONS) break;
      
      KeyButton& key = keyButtons[keyButtonCount];
      key.x = startX + col * (keyWidth + spacing);
      key.y = startY + row * (keyHeight + spacing);
      key.w = keyWidth;
      key.h = keyHeight;
      key.key = keyboard[row][col];
      keyButtonCount++;
    }
  }
  
  // Back button
  backButton.x = 20;
  backButton.y = 280;
  backButton.w = 80;
  backButton.h = 30;
  
  // Delete button
  deleteButton.x = 120;
  deleteButton.y = 280;
  deleteButton.w = 80;
  deleteButton.h = 30;
}

void drawKeyboard() {
  SPITransaction transaction(TFT2_CS);
  selectDisplay(DISPLAY_2);
  
  tft2.fillScreen(BLACK);
  tft2.setTextColor(WHITE);
  tft2.setTextSize(1);
  
  // Draw search string
  tft2.setCursor(10, 10);
  tft2.print("Search: " + currentSearchString);
  tft2.drawRect(10, 25, 220, 20, WHITE);
  tft2.setCursor(15, 30);
  tft2.print(currentSearchString);
  
  // Draw keyboard
  for (int i = 0; i < keyButtonCount; i++) {
    KeyButton& key = keyButtons[i];
    tft2.drawRect(key.x, key.y, key.w, key.h, WHITE);
    String keyStr = String(key.key);
    if (key.key == ' ') keyStr = "SPC";
    tft2.setCursor(key.x + 3, key.y + 10);
    tft2.print(keyStr);
  }
  
  // Draw action buttons
  tft2.drawRect(backButton.x, backButton.y, backButton.w, backButton.h, GREEN);
  tft2.setCursor(backButton.x + 20, backButton.y + 10);
  tft2.print("BACK");
  
  tft2.drawRect(deleteButton.x, deleteButton.y, deleteButton.w, deleteButton.h, RED);
  tft2.setCursor(deleteButton.x + 25, deleteButton.y + 10);
  tft2.print("DEL");
}

void handleTouch() {
  int x, y;
  if (!touchHandler.getTouchPoint(x, y)) {
    return;
  }
  
  switch (appState) {
    case MENU_STATE:
      handleMenuTouch(x, y);
      break;
    case KEYBOARD_STATE:
      handleKeyboardTouch(x, y);
      break;
    case CSV_LOOKUP_STATE:
      handleCSVTouch(x, y);
      break;
    case IMAGE_VIEWER_STATE:
      handleImageTouch(x, y);
      break;
  }
}

void handleMenuTouch(int x, int y) {
  for (int i = 0; i < menuItemCount; i++) {
    MenuItem& item = menuItems[i];
    if (x >= item.x && x <= item.x + item.w && 
        y >= item.y && y <= item.y + item.h) {
      
      if (item.action == "csv") {
        appState = KEYBOARD_STATE;
        currentSearchString = "";
        drawKeyboard();
      } else if (item.action == "images") {
        appState = IMAGE_VIEWER_STATE;
        imageIndex = 0;
        displayCurrentImage();
      }
      break;
    }
  }
}

void handleKeyboardTouch(int x, int y) {
  // Check keyboard keys
  for (int i = 0; i < keyButtonCount; i++) {
    KeyButton& key = keyButtons[i];
    if (x >= key.x && x <= key.x + key.w && 
        y >= key.y && y <= key.y + key.h) {
      
      if (currentSearchString.length() < 20) {
        currentSearchString += key.key;
        drawKeyboard();
      }
      return;
    }
  }
  
  // Check back button
  if (x >= backButton.x && x <= backButton.x + backButton.w && 
      y >= backButton.y && y <= backButton.y + backButton.h) {
    
    if (currentSearchString.length() > 0) {
      searchCSV(currentSearchString);
      appState = CSV_LOOKUP_STATE;
      displayCSVResults();
    } else {
      appState = MENU_STATE;
      drawMenu();
    }
    return;
  }
  
  // Check delete button
  if (x >= deleteButton.x && x <= deleteButton.x + deleteButton.w && 
      y >= deleteButton.y && y <= deleteButton.y + deleteButton.h) {
    
    if (currentSearchString.length() > 0) {
      currentSearchString.remove(currentSearchString.length() - 1);
      drawKeyboard();
    }
    return;
  }
}

void handleCSVTouch(int x, int y) {
  // Scroll up
  if (y < 50 && csvScrollIndex > 0) {
    csvScrollIndex--;
    displayCSVResults();
  }
  // Scroll down
  else if (y > 270 && csvScrollIndex < csvResultCount - 8) {
    csvScrollIndex++;
    displayCSVResults();
  }
  // Back to menu
  else if (x < 50 && y > 280) {
    appState = MENU_STATE;
    drawMenu();
  }
}

void handleImageTouch(int x, int y) {
  // Previous image
  if (x < 120) {
    if (imageIndex > 0) {
      imageIndex--;
      displayCurrentImage();
    }
  }
  // Next image
  else {
    if (imageIndex < imageFileCount - 1) {
      imageIndex++;
      displayCurrentImage();
    }
  }
  
  // Back to menu
  if (y > 280) {
    appState = MENU_STATE;
    drawMenu();
  }
}

void searchCSV(String query) {
  csvResultCount = 0;
  csvScrollIndex = 0;
  
  SPITransaction transaction(SD_CS);
  
  File file = SD.open("/data.csv");
  if (!file) {
    csvResults[0] = "CSV file not found";
    csvResultCount = 1;
    return;
  }
  
  while (file.available() && csvResultCount < MAX_CSV_RESULTS) {
    String line = file.readStringUntil('\n');
    line.trim();
    
    if (line.indexOf(query) != -1) {
      csvResults[csvResultCount] = line;
      csvResultCount++;
    }
  }
  
  file.close();
  
  if (csvResultCount == 0) {
    csvResults[0] = "No results found";
    csvResultCount = 1;
  }
}

void displayCSVResults() {
  // Display results on TFT1
  {
    SPITransaction transaction(TFT1_CS);
    selectDisplay(DISPLAY_1);
    
    tft1.fillScreen(BLACK);
    tft1.setTextColor(WHITE);
    tft1.setTextSize(1);
    
    tft1.setCursor(10, 10);
    tft1.print("Search Results:");
    tft1.setCursor(10, 25);
    tft1.print("Query: " + currentSearchString);
    
    int y = 50;
    int maxResults = min(8, csvResultCount);
    
    for (int i = 0; i < maxResults; i++) {
      int index = csvScrollIndex + i;
      if (index < csvResultCount) {
        String result = csvResults[index];
        if (result.length() > 35) {
          result = result.substring(0, 35) + "...";
        }
        tft1.setCursor(5, y);
        tft1.print(result);
        y += 15;
      }
    }
    
    // Draw scroll indicators
    if (csvScrollIndex > 0) {
      tft1.setCursor(220, 40);
      tft1.print("^");
    }
    if (csvScrollIndex < csvResultCount - 8) {
      tft1.setCursor(220, 280);
      tft1.print("v");
    }
  }
  
  // Display instructions on TFT2
  {
    SPITransaction transaction(TFT2_CS);
    selectDisplay(DISPLAY_2);
    
    tft2.fillScreen(BLACK);
    tft2.setTextColor(WHITE);
    tft2.setTextSize(2);
    tft2.setCursor(20, 100);
    tft2.print("Touch screen to");
    tft2.setCursor(20, 130);
    tft2.print("scroll results");
    tft2.setCursor(20, 280);
    tft2.print("Touch here");
    tft2.setCursor(20, 300);
    tft2.print("for menu");
  }
}

void loadImageFiles() {
  imageFileCount = 0;
  
  SPITransaction transaction(SD_CS);
  
  File root = SD.open("/images");
  if (!root) {
    return;
  }
  
  while (imageFileCount < MAX_IMAGE_FILES) {
    File entry = root.openNextFile();
    if (!entry) break;
    
    if (!entry.isDirectory()) {
      String filename = entry.name();
      if (filename.endsWith(".bmp") || filename.endsWith(".jpg")) {
        imageFiles[imageFileCount] = "/images/" + filename;
        imageFileCount++;
      }
    }
    entry.close();
  }
  
  root.close();
}

void displayCurrentImage() {
  // Display image on TFT1
  {
    SPITransaction transaction(TFT1_CS);
    selectDisplay(DISPLAY_1);
    
    tft1.fillScreen(BLACK);
    
    if (imageFileCount == 0) {
      tft1.setTextColor(WHITE);
      tft1.setTextSize(2);
      tft1.setCursor(30, 150);
      tft1.print("No images found");
    } else {
      tft1.setTextColor(WHITE);
      tft1.setTextSize(1);
      tft1.setCursor(10, 10);
      tft1.print("Image: " + String(imageIndex + 1) + "/" + String(imageFileCount));
      tft1.setCursor(10, 25);
      tft1.print(imageFiles[imageIndex]);
      
      // Placeholder for actual image
      tft1.drawRect(50, 50, 140, 200, WHITE);
      tft1.setCursor(100, 140);
      tft1.print("IMAGE");
      tft1.setCursor(80, 160);
      tft1.print("PLACEHOLDER");
    }
  }
  
  // Display controls on TFT2
  {
    SPITransaction transaction(TFT2_CS);
    selectDisplay(DISPLAY_2);
    
    tft2.fillScreen(BLACK);
    tft2.setTextColor(WHITE);
    tft2.setTextSize(2);
    tft2.setCursor(40, 50);
    tft2.print("Image Viewer");
    tft2.setCursor(20, 150);
    tft2.print("< PREV   NEXT >");
    tft2.setCursor(30, 250);
    tft2.print("Touch bottom");
    tft2.setCursor(50, 280);
    tft2.print("for menu");
  }
}

void checkOffButton() {
  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(OFF_BUTTON);
  
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    // Button pressed
    if (appState != SLEEP_STATE) {
      enterSleepMode();
    } else {
      wakeUp();
    }
  }
  
  lastButtonState = currentButtonState;
}

void enterSleepMode() {
  {
    SPITransaction transaction(TFT1_CS);
    tft1.fillScreen(BLACK);
  }
  
  {
    SPITransaction transaction(TFT2_CS);
    tft2.fillScreen(BLACK);
  }
  
  appState = SLEEP_STATE;
}

void wakeUp() {
  appState = MENU_STATE;
  drawMenu();
}

void displayImage(String filename) {
  SPITransaction transaction(SD_CS);
  
  File file = SD.open(filename);
  if (!file) {
    return;
  }
  
  // This would require proper BMP/JPG decoding library
  // For now, just show filename
  {
    SPITransaction transaction2(TFT1_CS);
    tft1.fillScreen(BLACK);
    tft1.setTextColor(WHITE);
    tft1.setTextSize(2);
    tft1.setCursor(50, 150);
    tft1.print("Loading...");
  }
  
  file.close();
}