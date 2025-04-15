#pragma once

#include <WString.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <time.h> // for struct tm

// Returns the local time as a struct tm.
// If obtaining the time fails, a message is printed to Serial.
struct tm getLocalTimeAsTm();

// Returns the local time in ISO 8601 format as a String.
String getLocalTimeFull();

// Performs an HTTP GET request to the specified server with optional authorization.
// Returns the response payload as a String.
String httpGETRequest(const String serverName, String auth);

// Overload: Performs an HTTP GET request without authorization.
String httpGETRequest(const String serverName);

// Retrieves stop events from the given URL and returns them as a JsonDocument.
JsonDocument getStops(String url);

// Fetches news from a predefined URL and returns the articles as a JsonDocument.
JsonDocument getNews();

// Refreshes the token by sending a POST request to the specified server using the given payload.
// Returns the response as a String.
String refreshToken(String server, String payload);

// Retrieves an access token from the given server and payload.
// Returns the access token as a String.
String getToken(String server, String payload);

// Retrieves an access token from Google by refreshing it.
// Returns the access token as a String.
String getTokenGoogle();

// Retrieves calendar events from Google Calendar and returns them as a JsonDocument.
JsonDocument getCal(const char *calendarID);

// Retrieves a 5-day weather forecast from the weather API and returns it as a JsonDocument.
JsonDocument getForecast();

// Retrieves current weather observations from the weather API and returns them as a JsonDocument.
JsonDocument getWeather();

// Retrieves the FitX studio utilization percentage for a given studio ID.
// Returns the utilization percentage as an integer.
int getFitXAuslastung(int studio);

// Retrieves the FitX studio utilization and formats it as a string with the studio name.
// Returns a String in the format "StudioName: XX%".
String getFitXAuslastung(const char *studio, int studioID);

// Convenience functions for specific FitX studios.
String getFitX1();
String getFitX2();
