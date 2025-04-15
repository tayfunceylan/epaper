#include <WString.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "util/util.h"
#include "secrets.h"

String httpGETRequest(const String serverName, String auth)
{
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(serverName);
  if (auth.length() != 0)
    http.addHeader("Authorization", "Bearer " + auth);

  // Send HTTP POST request
  int httpResponseCode = http.GET();
  String payload = "{}";

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

String httpGETRequest(const String serverName)
{
  return httpGETRequest(serverName, "");
}

JsonDocument getStops(String url)
{
  String res = httpGETRequest(url);

  // create filter to only get the wanted values
  JsonDocument filter;
  filter["stopEvents"][0]["departureTimePlanned"] = true;
  filter["stopEvents"][0]["departureTimeEstimated"] = true;
  filter["stopEvents"][0]["transportation"]["destination"]["name"] = true;
  filter["stopEvents"][0]["transportation"]["disassembledName"] = true;

  // create json document from text input
  JsonDocument doc;
  deserializeJson(doc, res, DeserializationOption::Filter(filter));

  // create json which will be returned
  JsonDocument output;

  JsonArray array = doc["stopEvents"].as<JsonArray>(); // read from
  for (JsonVariant stopEvent : array)
  {
    JsonDocument element;
    element["departureTimePlanned"] = stopEvent["departureTimePlanned"];
    element["departureTimeEstimated"] = stopEvent["departureTimeEstimated"];
    element["destination"] = stopEvent["transportation"]["destination"]["name"];
    element["linie"] = stopEvent["transportation"]["disassembledName"];
    output.add(element);
  }
  return output;
}

String refreshToken(String server, String payload)
{
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(server);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  if (server == "https://login.microsoftonline.com/common/oauth2/v2.0/token")
    http.addHeader("Origin", "https://developer.microsoft.com");

  // Send HTTP POST request
  int httpResponseCode = http.POST(payload);

  String res = "{}";

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    res = http.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
  return res;
}

String getToken(String server, String payload)
{
  String res = refreshToken(server, payload);

  // create filter to only get the wanted values
  JsonDocument filter;
  filter["access_token"] = true;
  filter["refresh_token"] = true;

  // create json document from text input
  JsonDocument doc;
  deserializeJson(doc, res, DeserializationOption::Filter(filter));

  return doc["access_token"].as<String>();
}

String getTokenGoogle()
{
  String server = "https://oauth2.googleapis.com/token";
  String payload = GOOGLE_REFRESH_TOKEN_PAYLOAD;
  return getToken(server, payload);
}

JsonDocument getCal(const char *calendarID)
{
  // Serial.println("String instead of json");
  String url = String("https://www.googleapis.com/calendar/v3/calendars/") + calendarID + "/events" + "?orderBy=startTime" + "&singleEvents=True" + "&maxResults=8" + "&timeMin=" + getLocalTimeFull();

  String res = httpGETRequest(url, getTokenGoogle());

  // create filter to only get the wanted values
  JsonDocument filter;
  filter["items"][0]["summary"] = true;
  filter["items"][0]["start"] = true;
  filter["items"][0]["end"] = true;

  // create json document from text input
  JsonDocument doc;
  deserializeJson(doc, res, DeserializationOption::Filter(filter));

  return doc["items"];
}

JsonDocument getForecast()
{
  String res = httpGETRequest(API_WEATHER_FORECAST_URL);

  // create filter to only get the wanted values
  JsonDocument filter;
  filter["calendarDayTemperatureMax"] = true;
  filter["calendarDayTemperatureMin"] = true;
  filter["dayOfWeek"] = true;
  filter["daypart"][0]["wxPhraseLong"] = true;

  // create json document from text input
  JsonDocument doc;
  deserializeJson(doc, res, DeserializationOption::Filter(filter));

  return doc;
}

JsonDocument getWeather()
{
  String res = httpGETRequest(API_WEATHER_URL);

  // create filter to only get the wanted values
  JsonDocument filter;
  filter["observations"][0]["humidity"] = true;
  filter["observations"][0]["metric"]["temp"] = true;
  filter["observations"][0]["metric"]["windSpeed"] = true;

  // create json document from text input
  JsonDocument doc;
  deserializeJson(doc, res, DeserializationOption::Filter(filter));

  return doc["observations"][0];
}

int getFitXAuslastung(int studio)
{
  // Serial.println("String instead of json");
  String url = String("https://mein.fitx.de/nox/public/v1/studios/") + studio + "/utilization";

  HTTPClient http;
  http.begin(url);
  http.addHeader("X-Tenant", "fitx");
  if (http.GET() != 200)
    return 0;
  String res = http.getString();

  // create filter to only get the wanted values
  JsonDocument filter;
  filter["items"][0]["percentage"] = true;
  filter["items"][0]["isCurrent"] = true;

  // create json document from text input
  JsonDocument doc;
  deserializeJson(doc, res, DeserializationOption::Filter(filter));

  for (JsonVariant item : doc["items"].as<JsonArray>())
  {
    if (item["isCurrent"].as<bool>() == true)
      return item["percentage"].as<int>();
  }

  return 0;
}

String getFitXAuslastung(const char *studio, int studioID)
{
  char buffer[20];
  sprintf(buffer, "%s: %2d%%", studio, getFitXAuslastung(studioID));
  return String(buffer);
}
