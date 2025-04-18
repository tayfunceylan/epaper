
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <ArduinoJson.h>
#include "util/util.h"
#include "draw.h"

// somehow there should be an easier way to do this
#define GxEPD2_BW_IS_GxEPD2_BW true
#define GxEPD2_3C_IS_GxEPD2_3C true
#define GxEPD2_7C_IS_GxEPD2_7C true
#define GxEPD2_1248_IS_GxEPD2_1248 true
#define IS_GxEPD(c, x) (c##x)
#define IS_GxEPD2_BW(x) IS_GxEPD(GxEPD2_BW_IS_, x)
#define IS_GxEPD2_3C(x) IS_GxEPD(GxEPD2_3C_IS_, x)
#define IS_GxEPD2_7C(x) IS_GxEPD(GxEPD2_7C_IS_, x)
#define IS_GxEPD2_1248(x) IS_GxEPD(GxEPD2_1248_IS_, x)

#if defined(ESP32)
#define MAX_DISPLAY_BUFFER_SIZE 65536ul // e.g.
#if IS_GxEPD2_BW(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))
#elif IS_GxEPD2_3C(GxEPD2_DISPLAY_CLASS)
#elif IS_GxEPD2_7C(GxEPD2_DISPLAY_CLASS)
#endif
Display display(GxEPD2_DRIVER_CLASS(/*CS=*/15, /*DC=*/27, /*RST=*/26, /*BUSY=*/25));
#endif

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

const uint8_t *f = u8g2_font_7x14_mf;
const uint8_t *b1 = u8g2_font_7x14B_mf;
const uint8_t *b2 = u8g2_font_9x15B_mf;

void initDisplay()
{
    display.init(115200);
    SPI.end(); // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
    // SPI: void begin(int8_t sck=-1, int8_t miso=-1, int8_t mosi=-1, int8_t ss=-1);
    SPI.begin(13, 12, 14, 15); // map and init SPI pins SCK(13), MISO(12), MOSI(14), SS(15)
    u8g2Fonts.begin(display);  // connect u8g2 procedures to Adafruit GFX
}

void drawRows(int x, int y, int abstand, LinkedList<String> &list)
{
    for (int i = 0; i < list.size(); i++)
    {
        u8g2Fonts.setCursor(x, y + i * abstand);
        u8g2Fonts.print(list[i]);
    }
}

void drawWidget(const char *title, const uint8_t *font1, int x, int y, LinkedList<String> &list, const uint8_t *font2)
{
    u8g2Fonts.setCursor(x, y);
    u8g2Fonts.setFont(font1); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
    u8g2Fonts.print(title);
    u8g2Fonts.setFont(font2); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
    drawRows(x, y + 20, 15, list);
}

void clearScreen()
{
    display.setFullWindow();
    display.setRotation(0);
    u8g2Fonts.setFontDirection(0);             // left to right (this is default)
    u8g2Fonts.setForegroundColor(GxEPD_BLACK); // apply Adafruit GFX color
    u8g2Fonts.setBackgroundColor(GxEPD_WHITE); // apply Adafruit GFX color
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
    } while (display.nextPage());
    display.powerOff();
}

void drawClock(int x, int y, String *arr)
{
    // Uhrzeit
    u8g2Fonts.setCursor(x, y);
    u8g2Fonts.setFont(u8g2_font_profont29_mf); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
    u8g2Fonts.print(arr[0]);
    // Datum
    u8g2Fonts.setCursor(x, y + 15);
    u8g2Fonts.setFont(f); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
    u8g2Fonts.print(arr[1]);
}

void drawClock(int x)
{
    display.firstPage();
    do
    {
        display.fillScreen(GxEPD_WHITE);
        display.setPartialWindow(x, 0, 200, 50);
        drawClock(x, 30, getLocalTime());
    } while (display.nextPage());
}

void drawTafel(const char *title, int x, int y, JsonVariant array)
{
    LinkedList<String> list;
    for (JsonVariant stopEvent : array.as<JsonArray>())
    {
        list.add(getStringFromStopEvent(stopEvent));
    }
    drawWidget(title, b2, x, y, list, f);
}

void drawNamaz(const char *title, int x, int y, JsonVariant array)
{
    LinkedList<String> list;
    String TodayIso8601 = getLocalTimeIso8601();
    for (JsonVariant namazEvent : array.as<JsonArray>())
    {
        if (namazEvent["MiladiTarihKisaIso8601"].as<String>() != TodayIso8601)
            continue;

        const char *vakitIsim[] = {"Imsak", "Gunes", "Ogle", "Ikindi", "Aksam", "Yatsi"};
        char buffer[30];

        for (const char *vakit : vakitIsim)
        {
            snprintf(buffer, 30, "%-8s %s", vakit, namazEvent[vakit].as<const char *>());
            list.add(String(buffer));
        }
    }
    drawWidget(title, b2, x, y, list, f);
}

void drawCalendar(const char *title, int x, int y, JsonVariant array)
{
    LinkedList<String> list;
    for (JsonVariant calEvent : array.as<JsonArray>())
    {
        list.add(getStringFromCalEvent(calEvent));
    }
    drawWidget(title, b1, x, y, list, f);
}

void drawForecast(const char *title, int x, int y, JsonVariant obj)
{
    LinkedList<String> list;
    for (int i = 0; i < 6; i++)
    {
        String day = obj["dayOfWeek"][i].as<String>().substring(0, 2);
        int min = obj["calendarDayTemperatureMin"][i].as<int>();
        int max = obj["calendarDayTemperatureMax"][i].as<int>();
        list.add(forecastString(day, min, max));
    }
    drawWidget(title, b1, x, y, list, f);
}

void drawCurrentTemp(const char *title, int x, int y, JsonVariant obj)
{ // 400 80
    u8g2Fonts.setCursor(x, y);
    u8g2Fonts.setFont(u8g2_font_profont29_mf); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
    u8g2Fonts.print(obj["metric"]["temp"].as<String>() + "°C");

    u8g2Fonts.setFont(f); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
    u8g2Fonts.setCursor(x, y + 15);
    String humidity = obj["humidity"].as<String>() + "%";
    u8g2Fonts.print(humidity);

    u8g2Fonts.setCursor(x + 30, y + 15);
    String windspeed = obj["metric"]["windSpeed"].as<String>() + "kmh";
    u8g2Fonts.print(windspeed);
}

void drawFitX(const char *title, int x, int y, JsonVariant obj)
{
    LinkedList<String> list;
    for (int i = 0; i < 2; i++)
    {
        String item = obj[i].as<String>();
        list.add(item);
    }
    drawWidget("FitX", b1, x, y, list, f);
}

// void drawFont(const char name[])
// {
//     // display.setRotation(0);
//     display.fillScreen(GxEPD_WHITE);
//     u8g2Fonts.setCursor(0, 0);
//     u8g2Fonts.println();
//     u8g2Fonts.println(name);
//     u8g2Fonts.println(" !\"#$%&'()*+,-./");
//     u8g2Fonts.println("0123456789:;<=>?");
//     u8g2Fonts.println("@ABCDEFGHIJKLMNO");
//     u8g2Fonts.println("PQRSTUVWXYZ[\\]^_");
//     u8g2Fonts.println("`abcdefghijklmno");
//     u8g2Fonts.println("pqrstuvwxyz{|}~ ");
//     u8g2Fonts.println("Umlaut ÄÖÜäéöü");
// }

// void showFont(const char name[], const uint8_t *font)
// {
//     display.setFullWindow();
//     display.setRotation(0);
//     u8g2Fonts.setFontMode(1);                  // use u8g2 transparent mode (this is default)
//     u8g2Fonts.setFontDirection(0);             // left to right (this is default)
//     u8g2Fonts.setForegroundColor(GxEPD_BLACK); // apply Adafruit GFX color
//     u8g2Fonts.setBackgroundColor(GxEPD_WHITE); // apply Adafruit GFX color
//     u8g2Fonts.setFont(font);                   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
//     display.firstPage();
//     do
//     {
//         drawFont(name);
//     } while (display.nextPage());
// }