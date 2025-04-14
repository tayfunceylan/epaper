#pragma once

#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>

// select the display class (only one), matching the kind of display panel
#define GxEPD2_DISPLAY_CLASS GxEPD2_BW
#define GxEPD2_DRIVER_CLASS GxEPD2_750_T7 // GDEW075T7   800x480, GD7965
#define MAX_DISPLAY_BUFFER_SIZE 65536ul // e.g.
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))
typedef GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)> Display;
extern Display display;

void initDisplay();
void drawFont(const char name[]);
void showFont(const char name[], const uint8_t *font);
void clearScreen();
void drawTafel(const char *title, int x, int y, JsonArray array);
void drawCalendar(int x, int y, JsonVariant array);
void drawForecast(int x, int y, JsonVariant obj);
void drawWeather(int x, int y, JsonVariant obj);
void drawFitX(int x, int y, JsonVariant obj);
void drawClock(int x);
