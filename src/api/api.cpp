#include <WString.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "util/util.h"
#include "secrets.h"

JsonDocument httpGETRequest(const String &serverName, JsonDocument &filter, const String auth = "", bool debug = false)
{
  HTTPClient client;
  client.useHTTP10(true);

  JsonDocument doc;
  client.begin(serverName);
  if (auth.length() != 0)
    client.addHeader("Authorization", "Bearer " + auth);

  int httpResponseCode = client.GET();
  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    deserializeJson(doc, client.getStream(), DeserializationOption::Filter(filter));
    if (debug)
      serializeJsonPretty(doc, Serial);
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  client.end();
  return doc;
}

JsonDocument getStops(const char *url)
{
  JsonDocument filter;
  filter["stopEvents"][0]["departureTimePlanned"] = true;
  filter["stopEvents"][0]["departureTimeEstimated"] = true;
  filter["stopEvents"][0]["transportation"]["destination"]["name"] = true;
  filter["stopEvents"][0]["transportation"]["disassembledName"] = true;

  JsonDocument doc = httpGETRequest(url, filter);

  JsonDocument output;

  JsonArray array = doc["stopEvents"].as<JsonArray>();
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

String refreshToken(const char *server, const char *payload)
{
  HTTPClient client;
  client.begin(server);
  client.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpResponseCode = client.POST(payload);

  String res = "{}";

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    res = client.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  client.end();
  return res;
}

String getToken(const char *server, const char *payload)
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
  return getToken("https://oauth2.googleapis.com/token", GOOGLE_REFRESH_TOKEN_PAYLOAD);
}

JsonDocument getCal(const char *calendarID)
{
  // Serial.println("String instead of json");
  String url = String("https://www.googleapis.com/calendar/v3/calendars/") + calendarID + "/events" + "?orderBy=startTime" + "&singleEvents=True" + "&maxResults=8" + "&timeMin=" + getLocalTimeFull();

  JsonDocument filter;
  filter["items"][0]["summary"] = true;
  filter["items"][0]["start"] = true;
  filter["items"][0]["end"] = true;

  JsonDocument doc = httpGETRequest(url, filter, getTokenGoogle())["items"];
  return doc;
}

JsonDocument getForecast()
{
  JsonDocument filter;
  filter["calendarDayTemperatureMax"] = true;
  filter["calendarDayTemperatureMin"] = true;
  filter["dayOfWeek"] = true;
  filter["daypart"][0]["wxPhraseLong"] = true;

  return httpGETRequest(API_WEATHER_FORECAST_URL, filter);
}

JsonDocument getWeather()
{
  // create filter to only get the wanted values
  JsonDocument filter;
  filter["observations"][0]["humidity"] = true;
  filter["observations"][0]["metric"]["temp"] = true;
  filter["observations"][0]["metric"]["windSpeed"] = true;

  return httpGETRequest(API_WEATHER_URL, filter)["observations"][0];
}

int getFitXAuslastung(int studio)
{
  HTTPClient client;
  // Serial.println("String instead of json");
  String url = String("https://mein.fitx.de/nox/public/v1/studios/") + studio + "/utilization";

  client.begin(url);
  client.addHeader("X-Tenant", "fitx");
  if (client.GET() != 200)
  {
    client.end();
    return 0;
  }
  String res = client.getString();

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

JsonDocument getNamaz(const char *ilceKodu)
{
  static JsonDocument filter;
  if (filter.isNull())
  {
    filter[0]["MiladiTarihKisaIso8601"] = true;
    filter[0]["Imsak"] = true;
    filter[0]["Gunes"] = true;
    filter[0]["Ikindi"] = true;
    filter[0]["Ogle"] = true;
    filter[0]["Aksam"] = true;
    filter[0]["Yatsi"] = true;
  }

  String url = String("https://ezanvakti.emushaf.net/vakitler/") + ilceKodu;
  return httpGETRequest(url, filter);
}