#include <ArduinoJson.h>
#include <CronAlarms.h>

#include "secrets.h"
#include "api/api.h"
#include "draw/draw.h"
#include "util.h"

struct hashes
{
  String tafel1 = "";
  String tafel2 = "";
  String tafel3 = "";
  String cal = "";
  String temp = "";
  String forecast = "";
  String fitx = "";
};

struct hashes hashes;

void resetHashes()
{
  hashes.tafel1 = hashes.tafel2 = hashes.tafel3 = hashes.cal = hashes.temp = hashes.forecast = hashes.fitx = "";
}

JsonDocument globalDoc;

void partialUpdateCallback(void (*drawFunc)(int, int, JsonVariant), JsonVariant array, int x, int y, int w, int h, int dy = -14, int dh = -14)
{
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setPartialWindow(x, y + dy, w, h + dh);
    drawFunc(x, y, array);
  } while (display.nextPage());
}

template <typename T>
void partialUpdateCallbackIfHasChanged(const char *title, T &hash, JsonVariant data,
                                       void (*drawFunc)(int, int, JsonVariant),
                                       int x, int y, int w, int h, int dy = -14, int dh = -14)
{
  if (hasChanged(hash, data.as<String>()))
  {
    Serial.println(String(title) + " has changed");
    partialUpdateCallback(drawFunc, data, x, y, w, h, dy, dh);
  }
}

int x1 = 6 + 14;
int x12 = x1 + 293;
int x2 = x1 + 396;
int x3 = x2 + 180;
int x4 = x2 + 290;

int py1 = 32;
int py2 = py1 + 30 + 10; // 60
int py3 = py2 + 130;     // 190
int py4 = py3 + 135;     // 325

void drawTafel1(int x, int y, JsonVariant array) { drawTafel(BAHN_1, x, y, array); }
void drawTafel2(int x, int y, JsonVariant array) { drawTafel(BAHN_2, x, y, array); }
void drawTafel3(int x, int y, JsonVariant array) { drawTafel(BAHN_3, x, y, array); }

void drawEverything(JsonDocument &doc)
{
  partialUpdateCallbackIfHasChanged(BAHN_1, hashes.tafel1, doc["vrr"]["tafel1"], drawTafel1, x1, py2, 280, 130);
  partialUpdateCallbackIfHasChanged("fitx", hashes.fitx, doc["fitx"], drawFitX, x12, py2, 95, 130);
  partialUpdateCallbackIfHasChanged(BAHN_2, hashes.tafel2, doc["vrr"]["tafel2"], drawTafel2, x2, py2, 280, 130);
  partialUpdateCallbackIfHasChanged("forecast", hashes.forecast, doc["forecast"], drawForecast, x4, py2, 110, 130);
  partialUpdateCallbackIfHasChanged("weather", hashes.temp, doc["weather"], drawWeather, x4, py1, 70, 48, -30, 0);
  partialUpdateCallbackIfHasChanged(BAHN_3, hashes.tafel3, doc["vrr"]["tafel3"], drawTafel3, x1, py3, 280, 130);
  partialUpdateCallbackIfHasChanged("cal", hashes.cal, doc["cal"], drawCalendar, x1, py4, 385, 155 + 28);
}

void updateStopsData(JsonDocument &doc)
{
  doc["vrr"]["tafel1"] = getStops(URL_BAHN_1);
  doc["vrr"]["tafel2"] = getStops(URL_BAHN_2);
  doc["vrr"]["tafel3"] = getStops(URL_BAHN_3);
}

void updateDataExceptStops(JsonDocument &doc)
{
  doc["cal"] = getCal();
  doc["weather"] = getWeather();
  doc["forecast"] = getForecast();
  doc["fitx"][0] = getFitX1();
  doc["fitx"][1] = getFitX2();
}

JsonDocument updateAllData(JsonDocument &doc)
{
  updateStopsData(doc);
  updateDataExceptStops(doc);
  serializeJsonPretty(doc, Serial);
  return doc;
}

void showFirstTime()
{
  updateAllData(globalDoc);
  // serializeJsonPretty(globalDoc, Serial);
  clearScreen();
  drawClock(x1);
  drawEverything(globalDoc);
}

bool isFullRefresh = false;

void everyMinute()
{
  if (isFullRefresh)
  {
    clearScreen();
    isFullRefresh = false;
  }
  drawClock(x1);

  if (!hasWifi())
    reconnectWifi();
  if (hasWifi())
    updateStopsData(globalDoc);
  drawEverything(globalDoc);
  display.powerOff();
}

void every5Minute()
{
  if (!hasWifi())
    return;
  updateDataExceptStops(globalDoc);
}

void everyFullRefresh()
{
  resetHashes();
  isFullRefresh = true;
}

void setup()
{
  // Serial.begin();
  Serial.println();
  Serial.println("setup");
  initDisplay();

  // wifi
  connectToWifi(WIFI_SSID, WIFI_PASSWORD);

  // init and get the time
  configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
  printLocalTime();

  showFirstTime();
  Cron.create("45 59 * * * *", everyFullRefresh, false);   // full refresh every hour
  Cron.create("20 14-59/15 * * * *", every5Minute, false); // pull datat every 15 minutes starting at 14 min 20 sec
  Cron.create("0 * * * * *", everyMinute, false);          // refresh display every minute

  Serial.println("setup done");
}

void loop()
{
  Cron.delay(); // if the loop has nothing else to do, delay in ms
                // should be provided as argument to ensure immediate
                // trigger when the time is reached
  delay(1000);  // do other things... like waiting one second between clock display
}