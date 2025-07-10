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

// Display instances - Adafruit_ILI9341 supports separate instances!
Adafruit_ILI9341 tft1(TFT1_CS, TFT1_DC, TFT_RST);
Adafruit_ILI9341 tft2(TFT2_CS, TFT2_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS);

// Screen dimensions
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

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

// Menu structure
struct MenuItem {
  String title;
  String action;
  int x, y, w, h;
};

std::vector<MenuItem> menuItems;
String currentSearchString = "";
std::vector<String> csvResults;
int csvScrollIndex = 0;
std::vector<String> imageFiles;
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

std::vector<KeyButton> keyButtons;
KeyButton backButton, deleteButton;

// Colors (Adafruit_ILI9341 color constants)
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

// Function prototypes
void setupSPI();
void setActiveSPI(uint8_t activeCS);
void deselectAllSPI();
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
  
  // Initialize pins
  pinMode(TFT1_CS, OUTPUT);
  pinMode(TFT2_CS, OUTPUT);
  pinMode(TOUCH_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  pinMode(OFF_BUTTON, INPUT_PULLUP);
  
  // ✅ CRITICAL: Set all CS pins HIGH before any SPI operations
  deselectAllSPI();
  
  // Reset displays
  digitalWrite(TFT_RST, LOW);
  delay(100);
  digitalWrite(TFT_RST, HIGH);
  delay(100);
  
  // Initialize SPI
  setupSPI();
  
  // ✅ Initialize displays with proper CS management
  Serial.println("Initializing Display 1...");
  setActiveSPI(TFT1_CS);
  tft1.begin();
  tft1.setRotation(0);
  tft1.fillScreen(BLACK);
  tft1.setTextColor(WHITE);
  tft1.setTextSize(2);
  tft1.setCursor(10, 10);
  tft1.print("Display 1 Ready");
  deselectAllSPI();
  
  Serial.println("Initializing Display 2...");
  setActiveSPI(TFT2_CS);
  tft2.begin();
  tft2.setRotation(0);
  tft2.fillScreen(BLACK);
  tft2.setTextColor(WHITE);
  tft2.setTextSize(2);
  tft2.setCursor(10, 10);
  tft2.print("Display 2 Ready");
  deselectAllSPI();
  
  // ✅ Initialize touchscreen with proper CS isolation
  Serial.println("Initializing Touchscreen...");
  setActiveSPI(TOUCH_CS);
  ts.begin();
  ts.setRotation(0);
  deselectAllSPI();
  
  // ✅ Initialize SD card with proper CS isolation
  Serial.println("Initializing SD Card...");
  setActiveSPI(SD_CS);
  if (!SD.begin(SD_CS)) {
    deselectAllSPI();
    selectDisplay(DISPLAY_2);
    tft2.setCursor(10, 50);
    tft2.print("SD Card Failed");
    deselectAllSPI();
  } else {
    deselectAllSPI();
    selectDisplay(DISPLAY_2);
    tft2.setCursor(10, 50);
    tft2.print("SD Card OK");
    deselectAllSPI();
    loadMenuFromSD();
    loadImageFiles();
  }
  
  // Setup keyboard layout
  setupKeyboard();
  
  // Draw initial menu
  drawMenu();
  
  Serial.println("Setup complete!");
  deselectAllSPI();
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

// ✅ CRITICAL: Helper function for safe SPI device switching
void setActiveSPI(uint8_t activeCS) {
  // First, ensure ALL CS lines are HIGH
  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  
  // Small delay to ensure CS transitions are clean
  delayMicroseconds(10);
  
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

void selectDisplay(ActiveDisplay display) {
  if (display == DISPLAY_1) {
    setActiveSPI(TFT1_CS);
    currentDisplay = DISPLAY_1;
  } else {
    setActiveSPI(TFT2_CS);
    currentDisplay = DISPLAY_2;
  }
}

void loadMenuFromSD() {
  setActiveSPI(SD_CS);
  
  File file = SD.open("/menu.json");
  if (!file) {
    // Create default menu if file doesn't exist
    menuItems.clear();
    menuItems.push_back({"CSV Lookup", "csv", 20, 80, 200, 50});
    menuItems.push_back({"Image Viewer", "images", 20, 140, 200, 50});
    menuItems.push_back({"Settings", "settings", 20, 200, 200, 50});
    deselectAllSPI();
    return;
  }
  
  String jsonString = file.readString();
  file.close();
  deselectAllSPI();
  
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, jsonString);
  
  menuItems.clear();
  JsonArray items = doc["menu"];
  
  int y = 80;
  for (JsonObject item : items) {
    MenuItem menuItem;
    menuItem.title = item["title"].as<String>();
    menuItem.action = item["action"].as<String>();
    menuItem.x = 20;
    menuItem.y = y;
    menuItem.w = 200;
    menuItem.h = 50;
    menuItems.push_back(menuItem);
    y += 60;
  }
}

void drawMenu() {
  selectDisplay(DISPLAY_2);
  tft2.fillScreen(BLACK);
  tft2.setTextColor(WHITE);
  tft2.setTextSize(2);
  tft2.setCursor(50, 20);
  tft2.print("Main Menu");
  
  for (auto& item : menuItems) {
    tft2.drawRect(item.x, item.y, item.w, item.h, WHITE);
    tft2.setCursor(item.x + 10, item.y + 15);
    tft2.print(item.title);
  }
  
  deselectAllSPI();
}

void setupKeyboard() {
  keyButtons.clear();
  
  int startX = 5;
  int startY = 120;
  int keyWidth = 22;
  int keyHeight = 30;
  int spacing = 2;
  
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 10; col++) {
      KeyButton key;
      key.x = startX + col * (keyWidth + spacing);
      key.y = startY + row * (keyHeight + spacing);
      key.w = keyWidth;
      key.h = keyHeight;
      key.key = keyboard[row][col];
      keyButtons.push_back(key);
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
  for (auto& key : keyButtons) {
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
  
  deselectAllSPI();
}

void handleTouch() {
  // ✅ CRITICAL: Isolate touch reading from other SPI devices
  setActiveSPI(TOUCH_CS);
  
  if (!ts.touched()) {
    deselectAllSPI();
    return;
  }
  
  TS_Point p = ts.getPoint();
  deselectAllSPI();
  
  // Map touch coordinates to screen coordinates
  int x = map(p.x, 200, 3700, 0, SCREEN_WIDTH);
  int y = map(p.y, 200, 3700, 0, SCREEN_HEIGHT);
  
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
  
  delay(200); // Debounce
}

void handleMenuTouch(int x, int y) {
  for (auto& item : menuItems) {
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
  for (auto& key : keyButtons) {
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
  else if (y > 270 && csvScrollIndex < csvResults.size() - 8) {
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
    if (imageIndex < imageFiles.size() - 1) {
      imageIndex++;
      displayCurrentImage();
    }
  }
  
  // Back to menu (long press simulation)
  if (y > 280) {
    appState = MENU_STATE;
    drawMenu();
  }
}

void searchCSV(String query) {
  csvResults.clear();
  csvScrollIndex = 0;
  
  setActiveSPI(SD_CS);
  
  File file = SD.open("/data.csv");
  if (!file) {
    csvResults.push_back("CSV file not found");
    deselectAllSPI();
    return;
  }
  
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    
    if (line.indexOf(query) != -1) {
      csvResults.push_back(line);
    }
  }
  
  file.close();
  deselectAllSPI();
  
  if (csvResults.size() == 0) {
    csvResults.push_back("No results found");
  }
}

void displayCSVResults() {
  selectDisplay(DISPLAY_1);
  tft1.fillScreen(BLACK);
  tft1.setTextColor(WHITE);
  tft1.setTextSize(1);
  
  tft1.setCursor(10, 10);
  tft1.print("Search Results:");
  tft1.setCursor(10, 25);
  tft1.print("Query: " + currentSearchString);
  
  int y = 50;
  int maxResults = min(8, (int)csvResults.size());
  
  for (int i = 0; i < maxResults; i++) {
    int index = csvScrollIndex + i;
    if (index < csvResults.size()) {
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
  if (csvScrollIndex < csvResults.size() - 8) {
    tft1.setCursor(220, 280);
    tft1.print("v");
  }
  
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
  
  deselectAllSPI();
}

void loadImageFiles() {
  imageFiles.clear();
  
  setActiveSPI(SD_CS);
  
  File root = SD.open("/images");
  if (!root) {
    deselectAllSPI();
    return;
  }
  
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    
    if (!entry.isDirectory()) {
      String filename = entry.name();
      if (filename.endsWith(".bmp") || filename.endsWith(".jpg")) {
        imageFiles.push_back("/images/" + filename);
      }
    }
    entry.close();
  }
  
  root.close();
  deselectAllSPI();
}

void displayCurrentImage() {
  selectDisplay(DISPLAY_1);
  tft1.fillScreen(BLACK);
  
  if (imageFiles.size() == 0) {
    tft1.setTextColor(WHITE);
    tft1.setTextSize(2);
    tft1.setCursor(30, 150);
    tft1.print("No images found");
  } else {
    // Simple image display (would need proper BMP/JPG decoder)
    tft1.setTextColor(WHITE);
    tft1.setTextSize(1);
    tft1.setCursor(10, 10);
    tft1.print("Image: " + String(imageIndex + 1) + "/" + String(imageFiles.size()));
    tft1.setCursor(10, 25);
    tft1.print(imageFiles[imageIndex]);
    
    // Placeholder for actual image
    tft1.drawRect(50, 50, 140, 200, WHITE);
    tft1.setCursor(100, 140);
    tft1.print("IMAGE");
    tft1.setCursor(80, 160);
    tft1.print("PLACEHOLDER");
  }
  
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
  
  deselectAllSPI();
}

void checkOffButton() {
  if (digitalRead(OFF_BUTTON) == HIGH && appState != SLEEP_STATE) {
    enterSleepMode();
  } else if (digitalRead(OFF_BUTTON) == LOW && appState == SLEEP_STATE) {
    wakeUp();
  }
}

void enterSleepMode() {
  selectDisplay(DISPLAY_1);
  tft1.fillScreen(BLACK);
  
  selectDisplay(DISPLAY_2);
  tft2.fillScreen(BLACK);
  
  appState = SLEEP_STATE;
  deselectAllSPI();
}

void wakeUp() {
  appState = MENU_STATE;
  drawMenu();
}

// Additional helper function for proper image display
void displayImage(String filename) {
  setActiveSPI(SD_CS);
  
  File file = SD.open(filename);
  if (!file) {
    deselectAllSPI();
    return;
  }
  
  selectDisplay(DISPLAY_1);
  
  // This would require proper BMP/JPG decoding library
  // For now, just show filename
  tft1.fillScreen(BLACK);
  tft1.setTextColor(WHITE);
  tft1.setTextSize(2);
  tft1.setCursor(50, 150);
  tft1.print("Loading...");
  
  file.close();
  deselectAllSPI();
}