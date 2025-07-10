//                            USER DEFINED SETTINGS
//   Set driver type, fonts to be loaded, pins used and SPI control method etc
//
//   See the User_Setup_Select.h file if you wish to be able to define multiple
//   setups and then easily select which setup file is used by the compiler.
//
//   If this file is edited correctly then all the library example sketches should
//   run without the need to make any more changes for a particular hardware setup!
//   Note that some sketches are designed for a particular TFT pixel width/height

// ##################################################################################
//
// Section 1. Call up the right driver file and any options for it
//
// ##################################################################################

// Define to disable all #warnings in library (can be put in User_Setup_Select.h)
//#define DISABLE_ALL_LIBRARY_WARNINGS

// Only define one driver, the other ones must be commented out
#define ILI9341_DRIVER       // Generic driver for common displays

//#define ILI9341_2_DRIVER     // Alternative ILI9341 driver, see https://github.com/Bodmer/TFT_eSPI/issues/1172
//#define ST7735_DRIVER      // Define additional parameters below for this display
//#define ILI9163_DRIVER     // Define additional parameters below for this display
//#define S6D02A1_DRIVER
//#define RPI_ILI9486_DRIVER // 20MHz maximum SPI
//#define HX8357D_DRIVER
//#define ILI9481_DRIVER
//#define ILI9486_DRIVER
//#define ILI9488_DRIVER     // WARNING: Do not connect ILI9488 display SDO to MISO if other devices share the SPI bus (TFT SDO does NOT tristate when CS is high)
//#define ST7789_DRIVER      // Full configuration option, define additional parameters below for this display
//#define ST7789_2_DRIVER    // Minimal configuration option, define additional parameters below for this display
//#define R61581_DRIVER
//#define RM68140_DRIVER
//#define ST7796_DRIVER
//#define SSD1351_DRIVER
//#define SSD1963_480_DRIVER
//#define SSD1963_800_DRIVER
//#define SSD1963_800ALT_DRIVER
//#define ILI9225_DRIVER
//#define GC9A01_DRIVER

// Some displays support SPI reads via the MISO pin, other displays have a single
// bi-directional SDA pin and the library will try to read this via the MOSI line.
// To use the SDA line for reading data from the TFT uncomment the following line:

//#define TFT_SDA_READ      // This option is for ESP32 ONLY, tested with ST7789 and GC9A01 displays only

// For ST7735, ST7789 and ILI9341 ONLY, define the colour order IF the blue and red are swapped on your display
// Try ONE option at a time to find the correct colour order for your display

//  #define TFT_RGB_ORDER TFT_RGB  // Colour order Red-Green-Blue
//  #define TFT_RGB_ORDER TFT_BGR  // Colour order Blue-Green-Red

// For M5Stack ESP32 module with integrated ILI9341 display ONLY, remove // in line below

// #define M5STACK

// For ST7789, ST7735, ILI9163 and GC9A01 ONLY, define the pixel width and height in portrait orientation
// #define TFT_WIDTH  80
// #define TFT_WIDTH  128
// #define TFT_WIDTH  160
// #define TFT_WIDTH  240
// #define TFT_HEIGHT 160
// #define TFT_HEIGHT 128
// #define TFT_HEIGHT 240
// #define TFT_HEIGHT 320

// For ST7789 ONLY, define the init sequence code, uncomment the option that works for your display
// #define TFT_INIT_SEQUENCE 1 // Generic sequence that works for most ST7789 displays
// #define TFT_INIT_SEQUENCE 2 // For ESP32 TTGO T-Display
// #define TFT_INIT_SEQUENCE 3 // For ESP32 TTGO T-Display (rotation 1 & 3 work best)

// ##################################################################################
//
// Section 2. Define the pins that are used to interface with the display here
//
// ##################################################################################

// If a backlight control signal is available then define the TFT_BL pin in Section 2
// below. The backlight will be turned ON when tft.begin() is called, but the library
// needs to know if the LEDs are ON with the pin HIGH or LOW. If the LEDs are to be
// driven with a PWM signal or turned OFF/ON then this must be handled by the user
// sketch. e.g. with digitalWrite(TFT_BL, LOW);

// #define TFT_BL   32            // LED back-light control pin
// #define TFT_BACKLIGHT_ON HIGH  // Level to turn ON back-light (HIGH or LOW)

// We must use hardware SPI, a minimum of 3 GPIO pins is needed.
// Typical setup for ESP32 dev board (only tested with ILI9341 display)
// The hardware SPI can be mapped to any pins

// For ESP32 Dev board (only tested with ILI9341 display)
// The hardware SPI can be mapped to any pins
#define TFT_MISO 19  // Automatically assigned if not defined
#define TFT_MOSI 23  // Automatically assigned if not defined
#define TFT_SCLK 18  // Automatically assigned if not defined

// For the ESP32 the default SPI port is VSPI with MOSI 23, MISO 19, SCK 18, CS 5
// The alternative SPI port is HSPI with MOSI 13, MISO 12, SCK 14, CS 15
// To use the HSPI port, uncomment the following line:
//#define USE_HSPI_PORT

// Pin assignments for the dual display setup
// Display 1 (Top screen - no touch)
#define TFT_CS   5  // Chip select control pin for display 1
#define TFT_DC   2  // Data Command control pin for display 1
#define TFT_RST  21 // Reset pin (shared between both displays)

// For ESP32 Dev board (only tested with ILI9341 display)
// If you are using a different ESP32 board, please check the pin assignments

// Alternative pins for second display (uncomment when needed for display switching)
// These would be used when switching between displays in software
//#define TFT2_CS   17  // Chip select for display 2 
//#define TFT2_DC   16  // Data Command for display 2

// ##################################################################################
//
// Section 3. Define the fonts that are to be used here
//
// ##################################################################################

// Comment out the #defines below with // to stop that font being loaded
// The ESP8266 and ESP32 have plenty of memory so commenting out fonts is not
// normally necessary. If all fonts are loaded the extra FLASH space required is
// about 17Kbytes. To save FLASH space only enable the fonts you need!

#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
//#define LOAD_FONT8N // Font 8. Alternative to Font 8 above, slightly narrower, so 3 digits fit a 160 pixel TFT
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

// Comment out the #define below to stop the SPIFFS filing system and smooth font code being loaded
// this will save ~20kbytes of FLASH
#define SMOOTH_FONT

// ##################################################################################
//
// Section 4. Other options
//
// ##################################################################################

// For RP2040 processor and SPI displays, uncomment the following line to use the PIO interface.
//#define RP2040_PIO_SPI // Leave commented out to use standard RP2040 SPI port interface

// For RP2040 processor and 8 bit parallel displays:
// The default 8 bit parallel interface uses GPIO 0-7 on the RP2040 (Pico)
// Uncomment the next line if you want to use the alternative GPIO 16-23
//#define RP2040_PIO_INTERFACE // Leave commented out to use standard interface

// For ESP32 ONLY. Define the SPI port channel used (ESP32 has 2 channels)
// 1 for HSPI and 2 for VSPI
#define TFT_SPI_PORT 2 // Using VSPI (default ESP32 SPI port)

// Comment out the following #define if "SPI Transactions" do not need to be
// supported. When commented out the code size will be smaller and sketches will
// run slightly faster, so leave it commented out unless you need it!

// Transaction support is needed to work with SD library but not needed with TFT_SdFat
// Transaction support is required if other SPI devices are connected.

#define SUPPORT_TRANSACTIONS