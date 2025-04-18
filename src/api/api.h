#pragma once

// #include <WString.h>
// #include <ArduinoJson.h>
// #include <HTTPClient.h>
// #include <time.h> // for struct tm

JsonDocument getStops(const char *url);
JsonDocument getCal(const char *calendarID);
JsonDocument getWeather();
JsonDocument getForecast();
JsonDocument getNamaz(const char *ilceKodu);
String getFitXAuslastung(const char *studio, int studioID);
