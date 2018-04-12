#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <U8x8lib.h>
#include <HX711_ADC.h>
#include "soc/rtc.h"

const char* mqttServer = "your_mqtt_server";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client2(espClient);

//HX711 constructor (dout pin, sck pin)
HX711_ADC LoadCell(23, 22);
long t;
long Weight = 0.00;
long oldWeight = 0.00;
// Client variables 
char linebuf[80];
int charcount=0;
boolean mqttSendt = true;

// the OLED used
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);
WiFiServer server ( 80 );

const char* ssid = "ssid_id";
const char* password = "ssid_pass";

void setup() {
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 1, "radiona.org");
  u8x8.drawString(0, 5, "Booting ...");
    //  //Serial.print(": ");
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
     u8x8.drawString(0, 7, "Failed");    
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("Scale OTA ESP32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  u8x8.clearDisplay();
  u8x8.drawString(0, 1, "radiona.org");
  u8x8.drawString(0, 4, WiFi.localIP().toString().c_str());
  client2.setServer(mqttServer, mqttPort);

  while (!client2.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client2.connect("ESP32Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client2.state());
      Serial.println(" ");
    }
}

   
  u8x8.clearDisplay();
  u8x8.drawString(0, 1, "radiona.org");
  LoadCell.begin();
  long stabilisingtime = 2000; // tare preciscion can be improved by adding a few seconds of stabilising time
  LoadCell.start(stabilisingtime);
  LoadCell.setCalFactor(210.0); // user set calibration factor (float)
  Serial.println("Startup + tare is complete");
  u8x8.drawString(0, 4, "Ready");
  server.begin();
}

void loop() {
  ArduinoOTA.handle();
  readScale();
  serveClient();
}

void serveClient(){

  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    charcount=0;
    Serial.println("New client");
    memset(linebuf,0,sizeof(linebuf));
    charcount=0;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        readScale();
        //read char by char HTTP request
        linebuf[charcount]=c;
        if (charcount<sizeof(linebuf)-1) charcount++;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          client.println("<!DOCTYPE HTML><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><meta charset=\"utf-8\"></head>");
          client.println("<body style=\"padding:1rem;font-family:Arial,sans-serif;\">");
          client.println("<meta http-equiv=\"refresh\" content=\"5\"></head>");
          client.println("<h1><img style=\"max-width:600px;width:100%;\" src=\"data:image/png;base64,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\"></h1>");
          client.println("<h1 style=\"font-size:2rem;\">ESP32 LoRa Scale</h1>");
          client.println("<hr>");
          client.println("<div style=\"font-size:2rem;\">Your weight: <span id=\"w\" style=\"color:#930000\">");
          client.println(readScale2());
          client.println("</span>kg</div>");
          client.println("<div style=\"font-size:1rem;position:fixed;bottom:0;\">by Goran Mahovlic</div>");      
          client.println("</body>");
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          if(strstr(linebuf,"GET /value") > 0){
            // return value
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Cache-Control: no-cache, no-store, must-revalidate");
            client.println("Pragma: no-cache");
            client.println("Expires: 0");
            client.println("Connection: close");
            client.println();
            client.println(readScale2());
            //client.println("bla");
            break;
          }
          memset(linebuf,0,sizeof(linebuf));
          charcount=0;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);

    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
  
  }

long readScale2(){
  LoadCell.update();
  Weight = -(LoadCell.getData());
  if(Weight>5){
    char buf[20];
    sprintf(buf, "%04.02f", Weight);
    client2.publish("scale", buf);
    }
  return Weight/100.00;
}
  
void readScale(){
  LoadCell.update();
  //get smoothed value from data set + current calibration factor
  if (millis() > t + 2000) {
    float i = LoadCell.getData();
   // Serial.print("Load_cell output val: ");
    u8x8.clearDisplay();
   // u8x8.drawString(0, 1, "radiona.org");
    u8x8.setCursor(0, 0);
    u8x8.printf("%l.2 kg", -(i/100));
    Serial.print(-(i/100));
    Weight = -(i/100);
    Serial.println("kg");
    t = millis();

   if(int(Weight)==0){
    mqttSendt = true;
    }
   
   if(Weight>5 && mqttSendt){
    if ((int(Weight)-int(oldWeight)) == 0){
          mqttConnect();
          char buf[20];
          sprintf(buf, "%04.02f", Weight);
          client2.publish("scale", buf);
          Serial.println("Send ok!");
          mqttSendt = false;
      }
    }
    oldWeight = Weight;
  }
}

void mqttConnect(){
    while (!client2.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client2.connect("ESP32Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client2.state());
      Serial.println(" ");
    }
}
  }
