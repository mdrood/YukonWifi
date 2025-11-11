#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include "nvs_flash.h"

const char* ssid = "ESP32-AP";
const char* password = "12345678";

bool wifiLock = false;
bool wifiUnlock = false;
bool wifiRunning = false;
int LOCK = 5;
int UNLOCK = 18;
bool onbardPinOn = false;

const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);

String inputValue = "-80";  // initial value
IPAddress local_IP(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);

void saveInputValue();
void loadInputValue();

String displayValue = "Not set yet";  // This is read-only

// HTML template (with placeholder %VALUE%)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Control</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 50px; }
    input[type='text'] { padding: 10px; font-size: 18px; width: 200px; text-align: center; }
    input[type='submit'], button { padding: 10px 20px; font-size: 18px; margin: 10px; }
  </style>
</head>
<body>
<h2>Yukon Control</h2>
<form action="/setInput" method="GET">
  <input type="text" name="value" value="%VALUE%">
  <input type="submit" value="Update">
</form>

<p>Signal Strength: %DISPLAYVALUE%</p>  <!-- Read-only display -->

<br>
<button onclick="fetch('/switch1')">Lock</button>
<button onclick="fetch('/switch2')">Unlock</button>
</body>
</html>
)rawliteral";


// Event handlers (persistent)
WiFiEventId_t clientConnectedHandler;
WiFiEventId_t clientDisconnectedHandler;


void onClientConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("connected!!!!!!!!!!");
}

void onClientDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("disconnected00000000000000000000");
}



// Function to generate HTML with current value
String processor(const String& var) {
  if (var == "VALUE") return inputValue;
  if (var == "DISPLAYVALUE") return displayValue;
  return String();
}

void handleRoot() {
  String html = FPSTR(index_html);
  html.replace("%VALUE%", inputValue);
  html.replace("%DISPLAYVALUE%", displayValue);  // <--- add this line
  server.send(200, "text/html", html);
}


void handleSetInput() {
  if (server.hasArg("value")) {
    inputValue = server.arg("value");
    Serial.print("Input field updated: ");
    Serial.println(inputValue);
    saveInputValue();  // <--- Save to LittleFS
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleSwitch1() {
  Serial.println("Switch 1 pressed!");
  server.send(200, "text/plain", "Switch 1 triggered");
  digitalWrite(LOCK,HIGH);
  delay(500);
  digitalWrite(LOCK, LOW);
  Serial.println("LOOOOOOOOOOOOOOOOOOOOcked");
}

void handleSwitch2() {
  Serial.println("Switch 2 pressed!");
  server.send(200, "text/plain", "Switch 2 triggered");
    digitalWrite(UNLOCK,HIGH);
  delay(500);
  digitalWrite(UNLOCK, LOW);
  Serial.println("UUUUUUUUUUUUUUUUUUnlock");
}

void setup() {
    WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(500);

  // AP mode
  WiFi.mode(WIFI_AP);

  // Start AP on channel 1
  if (WiFi.softAP(ssid, password, 1)) {
    Serial.println("AP started successfully");
    Serial.print("SSID: "); Serial.println(ssid);
      Serial.print("Password: "); Serial.println(password);
    Serial.print("IP: "); Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start AP");
  }  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  delay(500);  // Let the AP settle
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("Access Point IP: ");
  Serial.println(myIP);

  // Start DNS (redirect all domains to ESP32)
  dnsServer.start(DNS_PORT, "*", myIP);

  // Web routes
  server.on("/", handleRoot);
  server.on("/setInput", handleSetInput);
  server.on("/switch1", handleSwitch1);
  server.on("/switch2", handleSwitch2);
  server.begin();

  Serial.println("Web server started!");
  clientConnectedHandler = WiFi.onEvent(onClientConnected, ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  clientDisconnectedHandler = WiFi.onEvent(onClientDisconnected, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
  if(!LittleFS.begin(true)){
    Serial.println("LittleFS mount failed");
    return;
  }
  loadInputValue();
}

void loop(){
  dnsServer.processNextRequest();
  server.handleClient();
    if(onbardPinOn){
    digitalWrite(2,LOW);
    onbardPinOn = false;
    Serial.print("");
  }else{
    digitalWrite(2,HIGH);
    onbardPinOn = true;
     Serial.print("");
 }
}

void loadInputValue() {
  if(LittleFS.exists("/input.txt")){
    File file = LittleFS.open("/input.txt", "r");
    if(file){
      inputValue = file.readString();
      file.close();
      Serial.print("Loaded inputValue: ");
      Serial.println(inputValue);
    }
  }
}

void saveInputValue() {
  File file = LittleFS.open("/input.txt", "w");
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  file.print(inputValue);
  file.close();
  Serial.println("Input saved to LittleFS");
}
