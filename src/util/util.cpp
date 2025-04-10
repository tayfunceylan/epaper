#include <LinkedList.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <Timezone.h>
#include <ArduinoJson.h>
#include "SHA1.h"

TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 1, 120};
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 2, 60}; // Central European Standard Time
Timezone GermanTimezone(CEST, CET);

void connectToWifi(const char *ssid, const char *password)
{
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

bool hasWifi()
{
  return WiFi.status() == WL_CONNECTED;
}

bool reconnectWifi()
{
  Serial.println("Reconnecting to WiFi...");
  WiFi.disconnect();
  return WiFi.reconnect();
}

LinkedList<String> stringToArray(String input, int length, int size)
{
  int start = 0;
  int end = 0;
  int i = 0;
  int size_counter = 0;
  LinkedList<String> array;
  // Serial.println(input.length);
  for (char ch : input)
  {
    if (i - start > length)
    {
      array.add(input.substring(start, end));
      if (++size_counter >= size)
        return array;
      start = end + 1;
    }
    if (ch == ' ')
    {
      end = i;
    }
    i++;
  }
  array.add(input.substring(start, i));
  return array;
}

LinkedList<String> stringToArray(String input, int length)
{
  return stringToArray(input, length, 100);
}

bool writeStringToEEPROM(const String &strToWrite)
{
  EEPROM.writeString(0, strToWrite);
  return EEPROM.commit();
}

String readStringFromEEPROM()
{
  return EEPROM.readString(0);
}

// neutral version of mktime
time_t makeTime(tm *tm)
{
  time_t tStampBadLocaltime = mktime(tm);

  struct tm tmUTC;
  struct tm tmLocaltime;
  gmtime_r(&tStampBadLocaltime, &tmUTC);
  localtime_r(&tStampBadLocaltime, &tmLocaltime);
  time_t tstampBadUTC = mktime(&tmUTC);
  time_t tstampLocaltime = mktime(&tmLocaltime);
  time_t tLocalOffset = tstampLocaltime - tstampBadUTC;
  return tStampBadLocaltime + tLocalOffset;
}

tm getTmFromString(String time)
{
  time_t temp;
  struct tm ts = {0};
  if (time.length() == 10)
    time += "T00:00:00Z";
  strptime(time.c_str(), "%Y-%m-%dT%H:%M:%S%Z", &ts);
  temp = GermanTimezone.toLocal(makeTime(&ts));
  return *gmtime(&temp);
  return ts;
}

String calcHash(uint8_t *hash)
{
  String output = "";
  for (int i = 0; i < 20; i++)
  {
    output += "0123456789abcdef"[hash[i] >> 4];
    output += "0123456789abcdef"[hash[i] & 0xf];
  }
  return output;
}

String getHash(const String &input)
{
  Sha1.init();
  Sha1.print(input);
  return calcHash(Sha1.result());
}

String getTime(String planned, String est)
{
  struct tm tm_start = getTmFromString(planned);
  struct tm tm_end = getTmFromString(est);

  char buffer[80];
  strftime(buffer, 80, "%H:%M", &tm_start);

  // calculate delay
  String delay = "";
  if (est != "null")
  {
    time_t start = mktime(&tm_start);
    time_t end = mktime(&tm_end);
    time_t diff = difftime(end, start);
    delay = String("+") + diff / 60;
  }

  return String(buffer) + " " + delay;
}

bool hasChanged(String &saved_hash, String input)
{
  if (input == "null" || input == "[{}]" || input == "[]" || input == "{}") // nichts tun bei leeren obj
    return false;
  if (input.length() < 20)
  {
    Serial.println("small hash, maybe empty?");
    Serial.println(input.length());
    Serial.println(input);
  }
  String new_hash = getHash(input);
  if (saved_hash != new_hash)
  {
    saved_hash = new_hash;
    return true;
  }
  return false;
}

String countTwoByteChars(String &input)
{
  int width = 0;
  for (int i = 0; i < input.length();)
  {
    unsigned char c = input[i];
    int charLen = 0;

    if (c < 0x80)
    {
      // 1-byte (ASCII)
      charLen = 1;
      width += 0;
    }
    else if ((c & 0xE0) == 0xC0)
    {
      // 2-byte sequence
      charLen = 2;
      width += 1;
    }
    else if ((c & 0xF0) == 0xE0)
    {
      // 3-byte sequence
      charLen = 3;
      width += 3; // Assuming full-width
    }
    else if ((c & 0xF8) == 0xF0)
    {
      // 4-byte sequence
      charLen = 4;
      width += 4; // Assuming full-width; adjust if needed
    }
    else
    {
      // Fallback: treat as a single byte
      charLen = 1;
      width += 0;
    }

    i += charLen;
  }

  // Generate the output string with the calculated width
  String output = "";
  for (int i = 0; i < width; i++)
  {
    output += " ";
  }

  return output;
}

String getStringFromStopEvent(JsonVariant stopEvent)
{
  const char *linie = stopEvent["linie"].as<const char *>();
  const char *destination = stopEvent["destination"].as<const char *>();
  String destinationString = String(destination);
  String departureTimePlanned = stopEvent["departureTimePlanned"].as<String>();
  String departureTimeEstimated = stopEvent["departureTimeEstimated"].as<String>();
  String time = getTime(departureTimePlanned, departureTimeEstimated);

  char s[60];
  snprintf(s, 60, "%s %-26s%s %s", linie, destination, countTwoByteChars(destinationString).c_str(), time.c_str());

  return String(s);
}

struct tm getLocalTimeAsTm()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
  }
  return timeinfo;
}

String getLocalTimeFull()
{
  struct tm timeinfo = getLocalTimeAsTm();

  char buffer[80];
  strftime(buffer, 80, "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(buffer);
}

void resetClock(struct tm *obj)
{
  obj->tm_sec = 0;
  obj->tm_min = 0;
  obj->tm_hour = 0;
}

int dayDiff(String event_start, int *weekday)
{
  struct tm tm_start = getTmFromString(event_start);
  struct tm tm_now = getLocalTimeAsTm();

  *weekday = tm_start.tm_wday;

  resetClock(&tm_start);
  resetClock(&tm_now);
  int daysecs = 60 * 60 * 24;

  // calculate delay
  time_t start = mktime(&tm_start);
  time_t now = mktime(&tm_now);
  double diff = difftime(start, now);

  return diff / daysecs;
}

String dayDiffString(String event_start)
{
  int weekday = 0;
  int days = dayDiff(event_start, &weekday);
  if (days == 0)
    return "heute";
  if (days == 1)
    return "morgen";

  char s[20];

  static const char *dayOfTheWeek[] = {"So ", "Mo ", "Di ", "Mi ", "Do ", "Fr ", "Sa "};
  snprintf(s, 20, "in %3d Tagen, %s", days, dayOfTheWeek[weekday]);
  return String(s);
}

String getStringFromCalEvent(JsonVariant event)
{
  String summary = event["summary"].as<String>().substring(0, 35);
  String start = event["start"].as<JsonObject>().begin()->value().as<String>();

  char s[60];
  snprintf(s, 60, "%-35s%s %s", summary.c_str(), countTwoByteChars(summary).c_str(), dayDiffString(start).c_str());

  return String(s);
}

String forecastString(String day, int min, int max)
{
  char s[60];
  snprintf(s, 60, "%2s %3d %3d", day, min, max);
  return String(s);
}

void printLocalTime()
{
  struct tm timeinfo = getLocalTimeAsTm();
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

String *getLocalTime()
{
  static String arr[2];
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    arr[0] = "00:00";
    arr[1] = "Montag, 1. Januar";
    return arr;
  }

  char buffer[80];
  char buffer2[100];

  strftime(buffer, 80, "%H:%M", &timeinfo);
  arr[0] = buffer;

  strftime(buffer, 80, "%d. %B", &timeinfo);
  static const char *dayOfTheWeek[] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
  snprintf(buffer2, sizeof(buffer2), "%s, %s", dayOfTheWeek[timeinfo.tm_wday], buffer); // Use snprintf for safety
  arr[1] = buffer2;

  return arr;
}