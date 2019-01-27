#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

/**
 * TODO: next steps
 * 
 * - research static ip assignment
 * - create functions for manipulating TOGGLE_PIN
 * - create function for authentication
 * - research way to use configurable ssid/pw
 *   - wifi: some devices allow you to connect to them first
 */

// wifi ssid/password
const char* ssid = "x";
const char* password = "x";

// username/password for htauthentication
const char* www_username = "x";
const char* www_password = "x";

// @TODO static ip
// for now use router to assign same
// IPAddress ip(192, 168, 1, 119);    
// IPAddress gateway(192,168,1,1);
// IPAddress subnet(255,255,255,0);

// Pin that connects the door open switch, used to physically open/close the door
//relay connected to D1(note on nodemcu)/pin 5
int TOGGLE_PIN = 5; 

// Sensor pin that reads the state of the "magnetic alarm" sensor 
// giving door open/closed state
// d2 = gpio 4
int GARAGE_SENSOR_PIN = 4; 

const int LED = 13;

ESP8266WebServer server(80);

void handleRoot() {
  digitalWrite(LED, 1);
  server.send(200, "text/html", "<h1>garage!</h1>");
  digitalWrite(LED, 0);
}

void handleNotFound() {
  digitalWrite(LED, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(LED, 0);
}

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);

  pinMode(TOGGLE_PIN, OUTPUT);
  digitalWrite(TOGGLE_PIN, LOW);

  pinMode(GARAGE_SENSOR_PIN, INPUT_PULLUP);
  
  Serial.begin(115200);

  // @TODO static ip
  // WiFi.hostname("garage");
  // WiFi.config(ip, gateway, subnet);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("garage")) {
    Serial.println("MDNS responder started");
  }

  ArduinoOTA.begin();

  server.on("/", handleRoot);

  // returns json response indicating door position
  server.on("/statusdoor", [](){
    // @TODO create function for authentication
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    
    int proximity = digitalRead(GARAGE_SENSOR_PIN);
    if (proximity == LOW) {
      server.send(200, "application/json", "{door:'open'}");
    } else {
      server.send(200, "application/json", "{door:'closed'}");
    }
  });

  server.on("/status", [](){
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    
    String out = "";
    out += "IP: ";
    out += WiFi.localIP();
    out += "\n";
    out += "Status: ";
    out += WiFi.status();
    out += "\n";
    
    out += "Door: ";
    int proximity = digitalRead(GARAGE_SENSOR_PIN);
    if (proximity == LOW) {
       out += "open ";
    } else {
      out += "closed ";
    }
    out += "\n";
    
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate", true);
    server.sendHeader("Pragma", "no-cache", true);
    server.sendHeader("Expires", "0", true);
    server.send(200, "text/plain", out);
  });

  // simulates a physical push of the garage door button one time for 300 ms
  server.on("/push", []() {
    // push button 1x
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    digitalWrite(LED_BUILTIN, LOW); // on
    digitalWrite(TOGGLE_PIN, HIGH);
    delay(300);
    digitalWrite(TOGGLE_PIN, LOW);
    digitalWrite(LED_BUILTIN, HIGH); // off
    
    server.sendHeader("Location", String("/status?pushed"), true);
    server.send(304, "text/plain", "");
  });

  // simulates a physical push of the garage door button one time for 
  // a custom amount of time
  server.on("/pushN", []() {
    // push button for N seconds
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }

    int miliseconds = 2000;
    
    String strMiliSeconds = server.arg("miliseconds");
    if (strMiliSeconds != "") {
      miliseconds = strMiliSeconds.toInt();
    }
    
    digitalWrite(LED_BUILTIN, LOW); // on
    digitalWrite(TOGGLE_PIN, HIGH);
    delay(miliseconds);
    digitalWrite(TOGGLE_PIN, LOW);
    digitalWrite(LED_BUILTIN, HIGH); // off
    
    server.send(200, "text/plain", "Pushed for " + strMiliSeconds + " ms");
  });

  // toggles physical push of garage door button, waits 1.5 seconds, pushes again.
  // for my model of garage door this simulates the garage door remote 
  server.on("/toggle", []() {
    // push 2x with 1.5 sec pause
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    
    digitalWrite(LED_BUILTIN, LOW); // on
    
    // on, wait 300ms, off
    digitalWrite(TOGGLE_PIN, HIGH);
    delay(300);
    digitalWrite(TOGGLE_PIN, LOW);
    
    delay(1500);
    
    // on, wait 300ms, off
    digitalWrite(TOGGLE_PIN, HIGH);
    delay(300);
    digitalWrite(TOGGLE_PIN, LOW);
    
    digitalWrite(LED_BUILTIN, HIGH); // off

    // i was having issues with my phone toggling the door upon locking/unlocking the device. this
    // redirect fixed the issue.
    server.sendHeader("Location", String("/status?double"), true);
    server.send(304, "text/plain", "");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  if (WiFi.status() != WL_CONNECTED)
  {
      ESP.restart();
      delay(10000);
  }
  server.handleClient();
}
