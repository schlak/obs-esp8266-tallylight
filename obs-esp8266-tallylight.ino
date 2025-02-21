#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <WebSocketsClient.h>

/*
#include <Hash.h>
#include <ESPCrypto.h>
#include <base64.h>
*/

#include <ArduinoJson.h>

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;

#define LED_RED 3
#define LED_GREEN 2
#define LED_BLUE 0

#define WIFI_SSID "YOUR-SSID-HERE"
#define WIFI_PASS "YOUR-WIFI-PASSWORD"
#define OBS_WEBSOCKET_IP "192.168.X.X" //Add OBS IP
#define OBS_WEBSOCKET_PORT 4444
#define OBS_WEBSOCKET_PASS "CURRENTLY NOT WORKING"
#define OBS_SOURCE_NAME "SOURCE_NAME"  //"Cam 1"

#define LED_ON 0
#define LED_OFF 1

boolean authRequired = false;
boolean checkAuth = true;
boolean connected = false;
long messageIdCounter = 2;
boolean redLedStatus = LED_OFF;
boolean greenLedStatus = LED_OFF;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    case WStype_DISCONNECTED:
      connected = false;
      redLedStatus = LED_OFF;
      greenLedStatus = LED_OFF;
      digitalWrite(LED_GREEN, LED_OFF);
      digitalWrite(LED_RED, LED_OFF);
      digitalWrite(LED_BLUE, LED_ON);
      delay(100);
      digitalWrite(LED_BLUE, LED_OFF);
      delay(100);
      digitalWrite(LED_BLUE, LED_ON);
      delay(100);
      digitalWrite(LED_BLUE, LED_OFF);
      delay(100);
      digitalWrite(LED_BLUE, LED_ON);
      delay(100);
      digitalWrite(LED_BLUE, LED_OFF);
      delay(100);
      digitalWrite(LED_BLUE, LED_ON);
      break;

    case WStype_CONNECTED:
      connected = true;
      digitalWrite(LED_GREEN, LED_OFF);
      digitalWrite(LED_RED, LED_OFF);
      digitalWrite(LED_BLUE, LED_OFF);
      break;

    case WStype_TEXT: {
        DynamicJsonDocument resDoc(4069);
        // Deserialize the JSON document
        DeserializationError error = deserializeJson(resDoc, payload);

        // Test if parsing succeeds.
        if (error) {
          return;
        }

        String messageId = resDoc["message-id"];
        if(messageId == "1"){
          checkAuth = false;
          authRequired = resDoc["authRequired"];

          /*
        USE_SERIAL.printf("DEBUG 1");
        SHA256 hasher;
        byte shaResult[SHA256_SIZE];
        
        USE_SERIAL.printf("DEBUG 2");
          
        String salt = resDoc["salt"];
        String challenge = resDoc["challenge"];
        
        USE_SERIAL.printf("DEBUG 3");
        
        String secretString = OBS_WEBSOCKET_PASS + salt;
        int strLength = secretString.length();
        char* secretCharArr;
        secretString.toCharArray(secretCharArr, strLength);

        USE_SERIAL.printf("DEBUG 4");
        
        hasher.doUpdate(secretCharArr);
        hasher.doFinal(shaResult);
        
        USE_SERIAL.printf("DEBUG 5");
        
        String result = (char*) shaResult;
        String secret = base64::encode(result + challenge);
        strLength = secret.length();
        char* secretArr;
        secret.toCharArray(secretArr, strLength);
        hasher.doUpdate(secretArr);
        hasher.doFinal(shaResult);
        result = (char*) shaResult;
        String authResponse = base64::encode(result);

        StaticJsonDocument<512> responseDoc;
        responseDoc["request-type"] = "Authenticate";
        responseDoc["message-id"] = messageIdCounter++;
        responseDoc["auth"] = authResponse;
        String responseBody;        
        webSocket.sendTXT(responseBody); */
        } else {
          String updateType = resDoc["update-type"];
          if (updateType == "SwitchScenes") {
            JsonArray array = resDoc["sources"].as<JsonArray>();
            boolean sceneFound = false;
            for (JsonVariant v : array) {
              JsonObject sourceObject = v.as<JsonObject>();
              String sourceName = sourceObject["name"];
              Serial.println(sourceName);
              if (sourceName.indexOf(OBS_SOURCE_NAME) >= 0) {
                sceneFound = true;
                break;
              }
            }

            if (sceneFound) {
              redLedStatus = LED_ON;
              return;
            }
            redLedStatus = LED_OFF;
          }

          if (updateType == "PreviewSceneChanged") {
            JsonArray array = resDoc["sources"].as<JsonArray>();
            boolean sceneFound = false;
            for (JsonVariant v : array) {
              JsonObject sourceObject = v.as<JsonObject>();
              String sourceName = sourceObject["name"];
              Serial.println(sourceName);
              if (sourceName.indexOf(OBS_SOURCE_NAME) >= 0) {
                sceneFound = true;
                break;
              }
            }

            if (sceneFound) {
              greenLedStatus = LED_ON;
              return;
            }
            greenLedStatus = LED_OFF;
          }} 
          break;
        }
          case WStype_ERROR:
            break;

          case WStype_PING:
            // pong will be send automatically
            break;

          case WStype_PONG:
            // answer to a ping we send
            break;
        }
      }

      void setup() {
        Serial.begin(9600);
        pinMode(LED_RED, OUTPUT);
        pinMode(LED_GREEN, OUTPUT);
        pinMode(LED_BLUE, OUTPUT);
        digitalWrite(LED_RED, LED_OFF);
        digitalWrite(LED_GREEN, LED_OFF);
        digitalWrite(LED_BLUE, LED_OFF);

        //Wait for boot
        delay(4000);

        WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);

        //WiFi.disconnect();
        while (WiFiMulti.run() != WL_CONNECTED) {
          digitalWrite(LED_RED, LED_OFF);
          digitalWrite(LED_GREEN, LED_OFF);
          digitalWrite(LED_BLUE, LED_ON);
          Serial.print(".");
          delay(250);
          digitalWrite(LED_BLUE, LED_OFF);
          Serial.print(".");
          delay(250);
        }
        digitalWrite(LED_BLUE, LED_OFF);

        // server address, port and URL
        webSocket.begin(OBS_WEBSOCKET_IP, OBS_WEBSOCKET_PORT, "/");

        // event handler
        webSocket.onEvent(webSocketEvent);

        // try every 5000ms again if connection has failed
        webSocket.setReconnectInterval(5000);

        // start heartbeat (optional)
        // ping server every 15000 ms
        // expect pong from server within 3000 ms
        // consider connection disconnected if pong is not received 2 times
        webSocket.enableHeartbeat(15000, 3000, 2);

        delay(5000);
      }

      void loop() {
        webSocket.loop();

        if (connected) {
          if (!authRequired) {
            if (checkAuth) {
              webSocket.sendTXT("{\"request-type\":\"GetAuthRequired\",\"message-id\":\"1\"}");
            }
            if (redLedStatus == LED_ON) {
              digitalWrite(LED_RED, redLedStatus);
              digitalWrite(LED_GREEN, LED_OFF);
            } else {
              digitalWrite(LED_RED, redLedStatus);
              digitalWrite(LED_GREEN, greenLedStatus);
            }
          } else {
            for (int i = 0; i < 15; i++) {
              digitalWrite(LED_GREEN, LED_OFF);
              digitalWrite(LED_RED, LED_ON);
              delay(500);
              digitalWrite(LED_GREEN, LED_ON);
              digitalWrite(LED_RED, LED_OFF);
              delay(500);
            }
            authRequired = false;
            checkAuth = true;
          }
        }
      }