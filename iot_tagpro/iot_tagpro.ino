#include <ESP8266WiFi.h>
#include <LiquidCrystal.h>

char *ssid = "";
char *password = "";

// LEDS
int builtinLED = LED_BUILTIN;
int relaySignalPIN = D8;
// VALUES
int builtinLedVAL = LOW;
int relaySignalVAL = LOW;

// SERVER
WiFiServer server(80);

// LCD screen
const int rs = D5, en = D0;
LiquidCrystal lcd(rs, en, D4, D3, D2, D1);

void setup()
{
    Serial.begin(115200);
    // LCD SCREEN
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print("Init...");
    
    // BUILTIN LED
    pinMode(builtinLED, OUTPUT);
    digitalWrite(builtinLED, builtinLedVAL);

    // RELAY
    pinMode(relaySignalPIN, OUTPUT);
    digitalWrite(relaySignalPIN, relaySignalVAL);

    // Connect to WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    lcd.setCursor(0, 1);
    lcd.print("Wifi");
    lcd.setCursor(5, 1);
    lcd.print(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        blinkLED(builtinLED, builtinLedVAL, 100);
        delay(400);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    // Start the server
    server.begin();
    Serial.println("Server started");
    Serial.print("Use this URL : http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");

    lcd.clear();
    lcd.print(WiFi.localIP());
}

void loop()
{
    // Check if a client has connected
    WiFiClient client = server.available();
    if (!client)
    {
        return;
    }

    // Wait until the client sends some data
    Serial.println("New client");
    blinkLED(builtinLED, builtinLedVAL, 100);
    while (!client.available())
    {
        delay(1);
    }

    // Read the first line of the request
    String request = client.readStringUntil('\r');
    Serial.println(request);
    handleRoute(client, request);
    client.flush();

    Serial.println("Client disconnected");
    Serial.println("");
}

void handleRoute(WiFiClient client, String request)
{
    String method = request.substring(0, request.indexOf(' '));
    String url = request.substring(request.indexOf(' ') + 1, request.lastIndexOf(' '));
    String route = url.substring(0, url.indexOf('?'));
    String paramStr = url.substring(url.indexOf('?') + 1);

    String params[5][2]; // 5 params max
    int paramIdx = 0;
    while (true)
    {
        Serial.println("paramStr: " + paramStr);
        params[paramIdx][0] = paramStr.substring(0, paramStr.indexOf('='));
        params[paramIdx][1] = paramStr.substring(paramStr.indexOf('=') + 1, paramStr.indexOf('&'));
        int i = paramStr.indexOf('&');
        if (i == -1)
        {
            break;
        }
        paramStr.remove(0, i + 1);
        paramIdx++;
    }

    if (route == "/tagpro")
    {
        routeTagpro(client, params);
        delay(4000);
        lcdHomeScreen(lcd, client);
    }
    else if (route == "/")
    {
        routeHome(client);
    }

}

void routeTagpro(WiFiClient client, String params[][2])
{
    int duration = getParam(params, "duration", "2500").toInt();
    String lcdHead = getParam(params, "lcdHead", "");
    String lcdBody = getParam(params, "lcdBody", "");

    clientHeaders(client, "application/json");
    client.println("{ \"status\": \"success\" }");

    if (lcdHead != "" || lcdBody != "")
    {
        lcd.clear();
        lcd.print(lcdHead);
        lcd.setCursor(0, 1);
        lcd.print(lcdBody);
    }

    blinkLED(relaySignalPIN, relaySignalVAL, duration);

    if (lcdHead != "" || lcdBody != "")
    {
        delay(5000 - duration);
        lcdHomeScreen(lcd, client);
    }
}

void routeHome(WiFiClient client)
{
    clientHeaders(client, "text/html");
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("Server up and running");
    client.println("</html>");
}

void clientHeaders(WiFiClient client, String contentType)
{
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: " + contentType);
    client.println(""); //  do not forget this one
}

void blinkLED(int pin, int val, int dur)
{
    int tempVal = val == HIGH ? LOW : HIGH;
    digitalWrite(pin, tempVal);
    delay(dur);
    digitalWrite(pin, val);
}

String getParam(String params[][2], char *name, char *def)
{
    // Returns *name from params if found, else defaults to def
    for (int i = 0; i < sizeof(params) - 1; i++)
    {
        if (params[i][0] == name)
        {
            return params[i][1];
            break;
        }
    }
    return def;
}

void clearLcdLine(LiquidCrystal lcd, int nb)
{
    lcd.setCursor(0, nb);
    lcd.print("                ");
}

void lcdHomeScreen(LiquidCrystal lcd, WiFiClient client)
{
    lcd.clear();
    lcd.print(WiFi.localIP());
}
