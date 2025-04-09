#pragma once

#include <LinkedList.h>
#include <WiFi.h>

LinkedList<String> stringToArray(String input, int length, int size);
LinkedList<String> stringToArray(String input, int length);
void connectToWifi(const char *ssid, const char *password);
bool hasWifi();
bool reconnectWifi();
bool writeStringToEEPROM(const String &strToWrite);
String readStringFromEEPROM();
String getTime(String planned, String est);
String countTwoByteChars(String &input);
tm getTmFromString(String time);
bool hasChanged(String &saved_hash, String input);
String getStringFromStopEvent(JsonVariant stopEvent);
String getHash(const String &input);
String getStringFromCalEvent(JsonVariant event);
String forecastString(String day, int min, int max);
String getLocalTimeFull();
void printLocalTime();
String *getLocalTime();
