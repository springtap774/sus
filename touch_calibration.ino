/*
 * Touch Calibration Utility for ESP32 Dual Display
 * 
 * This utility helps you determine the correct touch calibration values
 * for your XPT2046 touchscreen to ensure accurate touch mapping.
 * 
 * Instructions:
 * 1. Upload this code
 * 2. Open Serial Monitor at 115200 baud
 * 3. Touch the corners and center as instructed
 * 4. Copy the calibration values to your main code
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

// Pin definitions (same as your main setup)
#define TFT2_CS 17   // Touch display
#define TFT2_DC 16
#define TOUCH_CS 4
#define TFT_RST 21
#define MOSI 23
#define MISO 19
#define SCK 18

// Only using the touch display for calibration
Adafruit_ILI9341 tft(TFT2_CS, TFT2_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS);

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// Calibration data storage
struct CalibrationPoint {
  int screenX, screenY;
  int touchX, touchY;
  String description;
};

CalibrationPoint calibrationPoints[5] = {
  {20, 20, 0, 0, "Top-Left"},
  {220, 20, 0, 0, "Top-Right"},
  {220, 300, 0, 0, "Bottom-Right"},
  {20, 300, 0, 0, "Bottom-Left"},
  {120, 160, 0, 0, "Center"}
};

int currentPoint = 0;
bool waitingForTouch = true;
unsigned long lastTouchTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Touch Calibration Utility");
  Serial.println("================================");
  
  // Initialize pins
  pinMode(TFT2_CS, OUTPUT);
  pinMode(TOUCH_CS, OUTPUT);
  pinMode(TFT_RST, OUTPUT);
  
  // Set CS pins HIGH
  digitalWrite(TFT2_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  
  // Reset display
  digitalWrite(TFT_RST, LOW);
  delay(100);
  digitalWrite(TFT_RST, HIGH);
  delay(100);
  
  // Initialize SPI
  SPI.begin(SCK, MISO, MOSI, -1);
  SPI.setFrequency(27000000);
  
  // Initialize display
  digitalWrite(TFT2_CS, LOW);
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(ILI9341_BLACK);
  digitalWrite(TFT2_CS, HIGH);
  
  // Initialize touchscreen
  digitalWrite(TOUCH_CS, LOW);
  ts.begin();
  ts.setRotation(0);
  digitalWrite(TOUCH_CS, HIGH);
  
  Serial.println("Touch calibration starting...");
  Serial.println("Please touch the points as they appear on screen");
  
  drawCalibrationPoint(currentPoint);
}

void loop() {
  if (waitingForTouch) {
    digitalWrite(TOUCH_CS, LOW);
    bool touched = ts.touched();
    digitalWrite(TOUCH_CS, HIGH);
    
    if (touched && (millis() - lastTouchTime) > 1000) {
      digitalWrite(TOUCH_CS, LOW);
      TS_Point p = ts.getPoint();
      digitalWrite(TOUCH_CS, HIGH);
      
      // Store raw touch coordinates
      calibrationPoints[currentPoint].touchX = p.x;
      calibrationPoints[currentPoint].touchY = p.y;
      
      Serial.print("Point ");
      Serial.print(currentPoint + 1);
      Serial.print(" (");
      Serial.print(calibrationPoints[currentPoint].description);
      Serial.print("): Screen(");
      Serial.print(calibrationPoints[currentPoint].screenX);
      Serial.print(",");
      Serial.print(calibrationPoints[currentPoint].screenY);
      Serial.print(") -> Touch(");
      Serial.print(p.x);
      Serial.print(",");
      Serial.print(p.y);
      Serial.println(")");
      
      lastTouchTime = millis();
      currentPoint++;
      
      if (currentPoint < 5) {
        drawCalibrationPoint(currentPoint);
      } else {
        finishCalibration();
      }
    }
  }
}

void drawCalibrationPoint(int pointIndex) {
  digitalWrite(TFT2_CS, LOW);
  
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  
  // Instructions
  tft.setCursor(10, 10);
  tft.print("Touch Calibration");
  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.print("Touch the RED crosshair");
  tft.setCursor(10, 55);
  tft.print("Point ");
  tft.print(pointIndex + 1);
  tft.print(" of 5: ");
  tft.print(calibrationPoints[pointIndex].description);
  
  // Draw crosshair at calibration point
  int x = calibrationPoints[pointIndex].screenX;
  int y = calibrationPoints[pointIndex].screenY;
  
  // Red crosshair
  tft.drawLine(x - 10, y, x + 10, y, ILI9341_RED);
  tft.drawLine(x, y - 10, x, y + 10, ILI9341_RED);
  tft.fillCircle(x, y, 3, ILI9341_RED);
  
  // White border for visibility
  tft.drawLine(x - 12, y, x - 11, y, ILI9341_WHITE);
  tft.drawLine(x + 11, y, x + 12, y, ILI9341_WHITE);
  tft.drawLine(x, y - 12, x, y - 11, ILI9341_WHITE);
  tft.drawLine(x, y + 11, x, y + 12, ILI9341_WHITE);
  
  digitalWrite(TFT2_CS, HIGH);
  
  waitingForTouch = true;
}

void finishCalibration() {
  waitingForTouch = false;
  
  digitalWrite(TFT2_CS, LOW);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.setCursor(30, 150);
  tft.print("Calibration");
  tft.setCursor(50, 170);
  tft.print("Complete!");
  digitalWrite(TFT2_CS, HIGH);
  
  // Calculate calibration values
  calculateCalibration();
}

void calculateCalibration() {
  Serial.println("\n=== CALIBRATION RESULTS ===");
  
  // Find min/max touch values
  int minX = 4095, maxX = 0, minY = 4095, maxY = 0;
  
  for (int i = 0; i < 5; i++) {
    if (calibrationPoints[i].touchX < minX) minX = calibrationPoints[i].touchX;
    if (calibrationPoints[i].touchX > maxX) maxX = calibrationPoints[i].touchX;
    if (calibrationPoints[i].touchY < minY) minY = calibrationPoints[i].touchY;
    if (calibrationPoints[i].touchY > maxY) maxY = calibrationPoints[i].touchY;
  }
  
  // Add some margin for better mapping
  int marginX = (maxX - minX) * 0.05;
  int marginY = (maxY - minY) * 0.05;
  
  minX -= marginX;
  maxX += marginX;
  minY -= marginY;
  maxY += marginY;
  
  Serial.println("Raw touch coordinate ranges:");
  Serial.print("X: "); Serial.print(minX); Serial.print(" to "); Serial.println(maxX);
  Serial.print("Y: "); Serial.print(minY); Serial.print(" to "); Serial.println(maxY);
  
  Serial.println("\n=== COPY THESE VALUES TO YOUR MAIN CODE ===");
  Serial.println("#define TOUCH_CALIBRATION_X_MIN " + String(minX));
  Serial.println("#define TOUCH_CALIBRATION_X_MAX " + String(maxX));
  Serial.println("#define TOUCH_CALIBRATION_Y_MIN " + String(minY));
  Serial.println("#define TOUCH_CALIBRATION_Y_MAX " + String(maxY));
  Serial.println("============================================\n");
  
  // Test mapping
  Serial.println("Testing mapping with center point:");
  int centerTouchX = calibrationPoints[4].touchX;
  int centerTouchY = calibrationPoints[4].touchY;
  int mappedX = map(centerTouchX, minX, maxX, 0, SCREEN_WIDTH);
  int mappedY = map(centerTouchY, minY, maxY, 0, SCREEN_HEIGHT);
  
  Serial.print("Center touch raw: ("); Serial.print(centerTouchX); Serial.print(","); Serial.print(centerTouchY); Serial.println(")");
  Serial.print("Center mapped: ("); Serial.print(mappedX); Serial.print(","); Serial.print(mappedY); Serial.println(")");
  Serial.print("Expected center: (120,160)");
  Serial.print(" - Error X: "); Serial.print(abs(mappedX - 120));
  Serial.print(", Error Y: "); Serial.println(abs(mappedY - 160));
  
  Serial.println("\nCalibration complete! Reset to run again.");
}