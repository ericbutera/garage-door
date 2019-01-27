#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "x";
const char* password = "x";

const char* www_username = "x";
const char* www_password = "x";

// IPAddress ip(192, 168, 1, 119);    
// IPAddress gateway(192,168,1,1);
// IPAddress subnet(255,255,255,0);

int d1 = 5;       //relay connected to D1(note on nodemcu)/pin 5
int GARAGE_SENSOR_PIN = 4; // d2 = gpio 4


ESP8266WebServer server(80);

const int led = 13;

void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/html", "<h1>garage!</h1>");
  digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
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
  digitalWrite(led, 0);
}

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);

  pinMode(d1, OUTPUT);
  digitalWrite(d1, LOW);

  pinMode(GARAGE_SENSOR_PIN, INPUT_PULLUP);
  
  Serial.begin(115200);

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

  server.on("/statusdoor", [](){
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

  server.on("/push", []() {
    // push button 1x
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    digitalWrite(LED_BUILTIN, LOW); // on
    digitalWrite(d1, HIGH);
    delay(300);
    digitalWrite(d1, LOW);
    digitalWrite(LED_BUILTIN, HIGH); // off
    
    server.sendHeader("Location", String("/status?pushed"), true);
    server.send(304, "text/plain", "");
  });

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
    digitalWrite(d1, HIGH);
    delay(miliseconds);
    digitalWrite(d1, LOW);
    digitalWrite(LED_BUILTIN, HIGH); // off
    
    server.send(200, "text/plain", "Pushed for " + strMiliSeconds + " ms");
  });

  server.on("/toggle", []() {
    // push 2x with 1.5 sec pause
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    
    digitalWrite(LED_BUILTIN, LOW); // on
    
    // on 500ms off
    digitalWrite(d1, HIGH);
    delay(300);
    digitalWrite(d1, LOW);
    
    delay(1500);
    
    // on 500ms off
    digitalWrite(d1, HIGH);
    delay(300);
    digitalWrite(d1, LOW);
    
    digitalWrite(LED_BUILTIN, HIGH); // off

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
