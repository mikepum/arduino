#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <TimeLib.h>
#include <NtpClientLib.h>

// Uncomment below line for running Test version on D1 Uno
// String relayName = "Test"; int relayPin = D4; const int port = 1081; String NVRstream = "11"; String postIP = "http://192.168.1.81:1081";

// Uncomment below line for running Front Cam version on D1 Mini
String relayName = "Front Cam"; int relayPin = D1; const int port = 1080; String NVRstream = "01"; String postIP = "http://192.168.1.80:1080";

// Uncomment below line for running Back Cam version on D1 Mini
// String relayName = "Back Cam"; int relayPin = D1; const int port = 1079; String NVRstream = "11"; String postIP = "http://192.168.1.79:1079";

const char* ssid = "xxxxxx";
const char* password = "xxxxx";

ESP8266WebServer server(port);
String page = "";
const char* www_username = "xxxxxx";
const char* www_password = "xxxxxx";

const char * NVRhost = "192.168xxxx"; // ip or dns
const uint16_t NVRport = 554;

int8_t timeZone = 0;
int8_t minutesTimeZone = 0;

String startTime = "";
String lastRTSPCheck = "None since server start";
String lastAutoReboot = "None since server start";
String lastAppReq = "None since server start";
String lastGoogleReq = "None since server start";
String lastWebReq = "None since server start";

unsigned long currentMillis = 0;    // stores the value of millis() in each iteration of loop()
unsigned long previousRTSPcheck = 0;   // will store last time the RTSP feed was checked
const int RTSPcheckinterval = 300000;  // RTSP check interval - 5 mins

void setup(void) {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.print(WiFi.localIP());
  Serial.print(":");
  Serial.println(port);

  NTP.begin ("uk.pool.ntp.org", timeZone, true, minutesTimeZone);
  delay(2000);
  getNTPtime();
  startTime = (NTP.getTimeDateString());

  server.on("/", HTTP_POST, handleRequest);
  server.on("/", []() {
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    handleUptest();
  });
  
  server.begin();
  Serial.println("HTTP server started\n");
}

void loop(void) {
  currentMillis = millis();
  server.handleClient();
  rtspClient();
}

void handleRequest() {
  getNTPtime();
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
  Serial.println(message);
  Serial.print("Response : ");

  if (server.arg("request") == "1" ) {
    digitalWrite(relayPin, HIGH);
    delay(5000);
    digitalWrite(relayPin, LOW);
    Serial.print(relayName);
    Serial.println(" Rebooted\n");
    previousRTSPcheck = currentMillis;
    if (server.arg("source") == "app" ) {
      lastAppReq = (NTP.getTimeDateString());
      // server.send(200, "text/plain", "Done!");  // response to http post request
      server.send(200);
    }
    if (server.arg("source") == "google" ) {
      lastGoogleReq = (NTP.getTimeDateString());
      // server.send(200, "text/plain", "Done!");  // response to http post request
      server.send(200);
    }
    if (server.arg("source") == "web" ) {
      lastWebReq = (NTP.getTimeDateString());
      server.sendHeader("refresh", "0");
      server.send(200);
    }
  }
}

void handleUptest() {
  page = "<html>\n";
  page += "<head>\n";
  page += "<meta http-equiv='refresh' content='5'/>\n";
  page += "<title>Wemos D1 " + relayName + "</title>\n";
  page += "<style>\n";
  page += "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\n";
  page += "</style>\n";
  page += "</head>\n";
  page += "<body>\n";
  page += "<h1><center>Wemos D1 " + relayName + " HTTP Server Running</center></h1>\n";
  page += "<p><center>Server Start Time: " + startTime + "</center></p>\n";
  page += "<p><center>Last NVR RTSP Check: " + lastRTSPCheck + "</center></p>\n";
  page += "<p><center>Last Reboot From Failed RTSP Check: " + lastAutoReboot + "</center></p>\n";
  page += "<p><center>Last Reboot From App User Request: " + lastAppReq + "</center></p>\n";
  page += "<p><center>Last Reboot From Google Assistant Request: " + lastGoogleReq + "</center></p>\n";
  

  page += "<form action=\"" + postIP + "\" method=\"post\">\n";
  page += "<input type=\"hidden\" id=\"request\" name=\"request\" value=\"1\">\n";
  page += "<input type=\"hidden\" id=\"source\" name=\"source\" value=\"web\">\n";
  page += "<p><center>Last Reboot From Web User Request: " + lastWebReq + "   <button>Reboot Now</button></center></p>\n";
  page += "</form>\n";

  page += "</body>\n";
  page += "</html>\n";

  server.send(200, "text/html", page);

}

void getNTPtime() {
  Serial.print (NTP.getTimeDateString ()); Serial.print (" ");
  Serial.println (NTP.isSummerTime () ? "Summer Time. " : "Winter Time. ");
}

void rtspClient() {
  if (currentMillis - previousRTSPcheck >= RTSPcheckinterval) {
    getNTPtime();
    lastRTSPCheck = (NTP.getTimeDateString ());
    Serial.print("Check NVR RTSP feed for ");
    Serial.println(relayName);
    Serial.print("connecting to: ");
    Serial.println(NVRhost);

    // Use WiFiClient class to create TCP connections
    WiFiClient client;

    if (!client.connect(NVRhost, NVRport)) {
      Serial.println("connection failed");
      Serial.println("wait 1 sec...");
      delay(1000);
      return;
    }

    Serial.print("connected to: ");
    Serial.println(NVRhost);
    Serial.print("port: ");
    Serial.println(NVRport);

    // This will send the request to the server
    Serial.print(String("DESCRIBE ") + "rtsp://" + NVRhost + ":" + NVRport + "/" + NVRstream + " RTSP/1.0\r\n" +
                 "Cseq: 2\r\n\r\n") ;
    client.print(String("DESCRIBE ") + "rtsp://" + NVRhost + ":" + NVRport + "/" + NVRstream + " RTSP/1.0\r\n" +
                 "Cseq: 2\r\n\r\n") ;

    //read back one line from server
    String line = client.readStringUntil('\r');
    Serial.println(line);
    Serial.print("Response : ");
    if (line != "RTSP/1.0 200 OK") {
      lastAutoReboot = (NTP.getTimeDateString());
      Serial.println("REBOOT REQUIRED!");
      digitalWrite(relayPin, HIGH);
      delay(5000);
      digitalWrite(relayPin, LOW);
      Serial.print(relayName);
      Serial.println(" Rebooted");
    }
    else {
      Serial.println("All ok!");
    }
    Serial.println("closing connection");
    Serial.println("");
    client.stop();
    previousRTSPcheck = currentMillis;
  }
}
