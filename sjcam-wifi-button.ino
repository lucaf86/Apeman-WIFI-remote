#include "ESP8266WiFi.h"
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>


/*****************************************
 * OTA update 
 */

const char* host = "esp8266-webupdate";
const char* ssid_prg = "CASA";
const char* password_prg = "politecnico";
ESP8266WebServer server(80);
const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

/******************************************/

/*********************************
  DEVICE SETTINGS
*********************************/
const char* ssid = "SPORT DV58b3fc923f7c"; // device Wifi name (SSID)
const char* pass = "12345678"; //device WiFi password

const String PHOTO_MODE = "/?custom=1&cmd=3001&par=0";
const String VIDEO_MODE = "/?custom=1&cmd=3001&par=1";

const String CAPTURE_PHOTO = "/?custom=1&cmd=1001";
const String START_RECORDING = "/?custom=1&cmd=2001&par=1";
const String STOP_RECORDING = "/?custom=1&cmd=2001&par=0";

char deviceIP[14];
const int httpPort = 80;

const int buttonPin = 14;

int deviceState = 0; // 0 = disconected, 1 = connected
int deviceMode = 0; // 0 photo, 1 video
int deviceCaptureState = 0; // capturing

// Button variables
#define debounce 20 // ms debounce period to prevent flickering when pressing or releasing the button
#define holdTime 2000 // ms hold period: how long to wait for press+hold event

int buttonValue = HIGH;
int currentButtonValue = HIGH;

long btnDnTime; // time the button was pressed down
long btnUpTime; // time the button was released
boolean ignoreUp = false; // whether to ignore the button release because the click+hold was triggered

int status = WL_IDLE_STATUS;     // the Wifi radio's status

WiFiClient client;

void handleOTAUpdate();

void setup() {
  int prgMode;
  
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  delay(100);

  prgMode = digitalRead(buttonPin);

  if(0 == prgMode)
  {
      handleOTAUpdate();  
  }
  
  // attempt to connect using WPA2 encryption:
  Serial.print("Attempting to connect to WPA network ");
  Serial.print(ssid);
  Serial.println("...");

  status = WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  digitalWrite(LED_BUILTIN, LOW);
  // if you are connected, print out info about the connection:
  Serial.println("");
  Serial.println("Connected to network");
  IPAddress deviceAdrr = WiFi.gatewayIP();
  sprintf(deviceIP, "%d.%d.%d.%d", deviceAdrr[0], deviceAdrr[1], deviceAdrr[2], deviceAdrr[3]);

  Serial.println(deviceIP);
  photoMode();

}

void loop() {
  delay(100);
  currentButtonValue = digitalRead(buttonPin);

  // Test for button pressed and store the down time
  if (currentButtonValue == LOW && buttonValue == HIGH && (millis() - btnUpTime) > long(debounce)) {
    btnDnTime = millis();
  }

  // Test for button release and store the up time
  if (currentButtonValue == HIGH && buttonValue == LOW && (millis() - btnDnTime) > long(debounce))
  {
    if (ignoreUp == false) {
      buttonDownFn();
    } else {
      ignoreUp = false;
    }

    btnUpTime = millis();
  }

  // Test for button held down for longer than the hold time
  if (currentButtonValue == LOW && (millis() - btnDnTime) > long(holdTime)) {
    toggleCameraMode();
    ignoreUp = true;
    btnDnTime = millis();
  }

  buttonValue = currentButtonValue;

}

void buttonDownFn() {
  Serial.println("down");
  capture();
}

void buttonReleasedFn() {
  Serial.println("released");
}

void toggleCameraMode() {
  if (deviceMode == 0) {
    videoMode();
  } else {
    photoMode();
  }
}

void capture() {
  Serial.println("capture");
  capturePhoto();
  if (deviceMode == 0) {

  } else if(deviceMode == 1 && deviceCaptureState == 0) {
    recordVideo();
//  } else if(deviceMode == 1 &&  deviceCaptureState == 0) {
  } else {

    stopRecording();
  } 
}

void capturePhoto(){
  Serial.println("capture");
  requestUrl(CAPTURE_PHOTO);
}

void recordVideo(){
  Serial.println("start recording");
  requestUrl(START_RECORDING);
  deviceCaptureState = 1;
}

void stopRecording(){
  Serial.println("stop recording");
  requestUrl(STOP_RECORDING);
  deviceCaptureState = 0;
}

void photoMode() {
  Serial.println("switching to photo mode");
  deviceMode = 0;
  requestUrl(PHOTO_MODE);
}

void videoMode() {
  Serial.println("switching to video mode");
  deviceMode = 1;
  requestUrl(VIDEO_MODE);
}

void requestUrl(String url) {
  Serial.println(url);
  if (!client.connect(deviceIP, httpPort)) {
    Serial.println("connection failed");
    digitalWrite(LED_BUILTIN, HIGH);
    return;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + deviceIP + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(10);
}

void initOTAUpdate()
{
  Serial.println("Entering OTA update mode...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid_prg, password_prg);
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    MDNS.begin(host);
    server.on("/", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", serverIndex);
    });
    server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      yield();
    });
    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.println("Connected to network");
    IPAddress deviceAdrr = WiFi.localIP();
    sprintf(deviceIP, "%d.%d.%d.%d", deviceAdrr[0], deviceAdrr[1], deviceAdrr[2], deviceAdrr[3]);
    Serial.println(deviceIP);
    Serial.printf("Ready! Open http://%s.local in your browser\n", host);
  } else {
    Serial.println("WiFi Failed");
  }
}

void handleOTAUpdate()
{
    initOTAUpdate();
    while(1)
    {
        server.handleClient();
        delay(1);
    }
}
