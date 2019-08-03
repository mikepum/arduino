#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <TimeLib.h>

const char *ssid = "xxxxx";
const char *password = "xxxxxx";

ESP8266WebServer server(80);
String page = "";
float celcius = 0.0;
float maxCelcius = 0.0;
String maxCelciusTime = "00:00";
String status = "OFF";
int timerStatus = 0;
int relay = D7;

#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into pin 9 on the Arduino
#define ONE_WIRE_BUS D9 
// #define ONE_WIRE_BUS D2 *****************************************
// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

unsigned long currentMillis = 0;    // stores the value of millis() in each iteration of loop()
unsigned long previousTempCheck = 0;   // will store last time temp was checked
const int tempCheckInterval = 5000;  // interval

unsigned long initTimerCheck = 0;   // will store last time timer was checked
int timerCheck = 0;  // interval

void setup(void)
{
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW);
  // start serial port
  Serial.begin(115200);
  Serial.print("Configuring access point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_POST, handleRequest);
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");

  Serial.println("Dallas Temperature IC Control Library Used");

  // Start up the library
  sensors.begin();
}

void loop(void)
{
  server.handleClient();

  currentMillis = millis();
  if (currentMillis - previousTempCheck >= tempCheckInterval) {

    sensors.requestTemperatures(); // Send the command to get temperatures
    // Serial.print("Temperature is: ");
    if (String(sensors.getTempCByIndex(0)) != "-127.00") {
      celcius = sensors.getTempCByIndex(0);
    }
    if (celcius > maxCelcius) {
      maxCelcius = celcius;
      if (minute() < 10) {
        maxCelciusTime = (String(hour()) + ":0" + String(minute()));
      } else {
        maxCelciusTime = (String(hour()) + ":" + String(minute()));
      }
    }

    if (timerCheck > 0) {
      timerStatus = timerCheck - currentMillis + initTimerCheck;
      if (timerStatus <= 0) {
        timerStatus = 0;
        timerCheck = 0;
        digitalWrite(relay, LOW);
        status = "OFF";
      }
    }
    Serial.println(String(celcius) + "," + String(maxCelcius) + "," + String(maxCelciusTime) + "," + status + "," + String(timerStatus));

    previousTempCheck = currentMillis;
  }
}

void handleRoot() {
  page = "<html>\n";
  page += "<head>\n";
  page += "<meta http-equiv='refresh' content='5'/>\n";
  page += "<title>ESP8266</title>\n";
  page += "<style>\n";
  page += "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\n";
  page += "</style>\n";
  page += "</head>\n";
  page += "<body>\n";
  page += "<h1><center>ESP8266 HTTP Server Running</center></h1>\n";
  page += "<h1><center>Current temp " + String(celcius) + "</center></h1>\n";
  page += "</body>\n";
  page += "</html>\n";

  server.send(200, "text/html", page);

}

void handleRequest() {
  String message = "Request received....\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += server.argName(i) + ": " + server.arg(i) + "\n";
  }
  // Serial.println(message);
  Serial.println(server.arg("request") + " received");
  if (server.arg("request").startsWith("initTime")) {
    String tempString = server.arg("request").substring(8);
    int commaIndex = tempString.indexOf(',');
    int secondCommaIndex = tempString.indexOf(',', commaIndex + 1);
    int thirdCommaIndex = tempString.indexOf(',', secondCommaIndex + 1);
    int fourthCommaIndex = tempString.indexOf(',', thirdCommaIndex + 1);
    int fifthCommaIndex = tempString.indexOf(',', fourthCommaIndex + 1);
    int tempHour = String(tempString.substring(0, commaIndex)).toInt();
    int tempMins = String(tempString.substring(commaIndex + 1, secondCommaIndex)).toInt();
    int tempSecs = String(tempString.substring(secondCommaIndex + 1, thirdCommaIndex)).toInt();
    int tempDay = String(tempString.substring(thirdCommaIndex + 1, fourthCommaIndex)).toInt();
    int tempMonth = String(tempString.substring(fourthCommaIndex + 1, fifthCommaIndex)).toInt();
    int tempYear = String(tempString.substring(fifthCommaIndex + 1)).toInt();
    setTime(tempHour, tempMins, tempSecs, tempDay, tempMonth, tempYear);
    Serial.print("Time set to " );
    Serial.print(hour());
    if (minute() < 10) {
      Serial.print(":0");
    } else {
      Serial.print(":");
    }
    Serial.print(minute());
    if (second() < 10) {
      Serial.print(":0");
    } else {
      Serial.print(":");
    }
    Serial.println(second());
  }
  if (server.arg("request") == "resetMax" ) {
    maxCelcius = 0.0;
  }
  else if (server.arg("request") == "on" ) {
    digitalWrite(relay, HIGH);
    status = "ON";
  }
  else if (server.arg("request") == "off" ) {
    digitalWrite(relay, LOW);
    status = "OFF";
    timerStatus = 0;
    timerCheck = 0;
  }
  else if (server.arg("request") == "timernone" ) {
    timerStatus = 0;
    timerCheck = 0;
  }
  else if (server.arg("request") == "timer2" ) {
    timerCheck = 120000;
    initTimerCheck = currentMillis;
  }
  else if (server.arg("request") == "timer5" ) {
    timerCheck = 300000;
    initTimerCheck = currentMillis;
  }
  else if (server.arg("request") == "timer10" ) {
    timerCheck = 600000;
    initTimerCheck = currentMillis;
  }
  server.send(200, "text/plain", String(celcius) + "," + String(maxCelcius) + "," + String(maxCelciusTime) + "," + status + "," + String(timerStatus));
}
