#include <ArduinoJson.h>
#include <CronAlarms.h>

#include "secrets.h"
#include "api/api.h"
#include "draw/draw.h"
#include "util/util.h"

JsonDocument globalDoc;

void partialUpdateCallback(void (*drawFunc)(const char*, int, int, JsonVariant), const char *title, JsonVariant array, int x, int y, int w, int h, int dy = -14, int dh = -14)
{
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setPartialWindow(x, y + dy, w, h + dh);
    drawFunc(title, x, y, array);
  } while (display.nextPage());
}

void partialUpdateCallbackIfHasChanged(const char *title, JsonVariant data,
                                       void (*drawFunc)(const char*, int, int, JsonVariant),
                                       int x, int y, int w, int h, int dy = -14, int dh = -14)
{
  if (hasChanged(title, data.as<String>()))
  {
    Serial.printf("%s has changed\n", title);
    partialUpdateCallback(drawFunc, title, data, x, y, w, h, dy, dh);
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

void drawEverything(JsonDocument &doc)
{
  partialUpdateCallbackIfHasChanged(BAHN_1, doc["vrr"]["tafel1"], drawTafel, x1, py2, 280, 130);
  partialUpdateCallbackIfHasChanged("FitX", doc["fitx"], drawFitX, x12, py2, 95, 130);
  partialUpdateCallbackIfHasChanged(BAHN_2, doc["vrr"]["tafel2"], drawTafel, x2, py2, 280, 130);
  partialUpdateCallbackIfHasChanged("Wetter", doc["forecast"], drawForecast, x4, py2, 110, 130);
  partialUpdateCallbackIfHasChanged("CurrentTemp", doc["weather"], drawCurrentTemp, x4, py1, 70, 48, -30, 0);
  partialUpdateCallbackIfHasChanged(BAHN_3, doc["vrr"]["tafel3"], drawTafel, x1, py3, 280, 130);
  partialUpdateCallbackIfHasChanged("Namaz Vakti", doc["namaz"], drawNamaz, x2, py3, 280, 130);
  partialUpdateCallbackIfHasChanged("Kalendar", doc["cal"], drawCalendar, x1, py4, 385, 155 + 28);
  partialUpdateCallbackIfHasChanged("Plan", doc["cal2"], drawCalendar, x2, py4, 385, 155 + 28);
}

void updateStopsData(JsonDocument &doc)
{
  doc["vrr"]["tafel1"] = getStops(URL_BAHN_1);
  doc["vrr"]["tafel2"] = getStops(URL_BAHN_2);
  doc["vrr"]["tafel3"] = getStops(URL_BAHN_3);
}

void updateDataExceptStops(JsonDocument &doc)
{
  doc["cal"] = getCal(GOOGLE_CALENDAR_ID);
  doc["cal2"] = getCal(GOOGLE_CALENDAR_ID_2);
  doc["weather"] = getWeather();
  doc["forecast"] = getForecast();
  doc["fitx"][0] = getFitXAuslastung(FITX_STUDIO_NAME_1, FITX_STUDIO_ID_1);
  doc["fitx"][1] = getFitXAuslastung(FITX_STUDIO_NAME_2, FITX_STUDIO_ID_2);
}

void updateOnlyWeekly(JsonDocument &doc)
{
  doc["namaz"] = getNamaz(ILCE_KODU);
}

void updateAllData(JsonDocument &doc)
{
  updateStopsData(doc);
  updateDataExceptStops(doc);
  updateOnlyWeekly(doc);
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

void every15Minute()
{
  if (!hasWifi())
    return;
  updateDataExceptStops(globalDoc);
}

void everyWeek()
{
  if (!hasWifi())
    return;
  updateOnlyWeekly(globalDoc);
}

void everyFullRefresh()
{
  hashesDoc.clear();
  isFullRefresh = true;
}

void setup()
{
  Serial.println("setup");
  initDisplay();

  // wifi
  connectToWifi(WIFI_SSID, WIFI_PASSWORD);

  // init and get the time
  configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
  printLocalTime();

  showFirstTime();
  Cron.create("45 59 * * * *", everyFullRefresh, false);   // full refresh every hour
  Cron.create("20 14-59/15 * * * *", every15Minute, false); // pull datat every 15 minutes starting at 14 min 20 sec
  Cron.create("20 0 1 * * 5", everyWeek, false); // pull datat every week
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