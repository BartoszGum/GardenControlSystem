// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
    Name:       GardenConrtonSensor1.ino
    Created:	10.06.2021 10:57:35
    Author:     Bartosz Gumula
*/

// Define User Types below here or use a .h file
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <WebSocketsClient.h>
#include <Wire.h>
#include <DHT12.h>
#include <String.h>
#include <Adafruit_BMP280.h>
#include <Hash.h>
//
#define USE_SERIAL Serial
#define H HIGH
#define L LOW
#define Led 2

#define slow 2000;
#define fast 280;
//
IPAddress ip(192, 168, 1, 10);
IPAddress gate(192, 168, 1, 1);
IPAddress sub(255,255,255,0);

char ssid[] = "SensorsWiFi";
char pass[] = "658B9D5B***";

WebSocketsClient webSocket;

DHT12 dht;

//globalVar



unsigned long readingTime;
unsigned long blinkTime;
float temp, hum;
uint8_t ledStatus = LOW;


void blink(uint8_t mode = 0) {

	switch (mode) {

	case 0: { // LED ON - OK;

		digitalWrite(Led, HIGH);
		ledStatus = HIGH;

	}break;

	case 1: { //slow blink - WiWi not connected 

		if (millis() >= blinkTime) {
			ledStatus = !ledStatus;
			digitalWrite(Led, ledStatus);
			blinkTime = millis() + slow;
		}

	}break;

	case 2: { //fast blink - sensor not avaliable

		if (millis() >= blinkTime) {
			ledStatus = !ledStatus;
			digitalWrite(Led, ledStatus);
			blinkTime = millis() + fast;
		}

	}break;

	}

}




// The setup() function runs once each time the micro-controller starts
void setup()
{
	delay(2000);

	//Basic
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	Wire.begin();

	//WiFi
	//WiFi.config(ip, gate, sub);
	delay(100);
	if (WiFi.begin(ssid, pass) == WL_CONNECTED){
		Serial.println("Connected");
	}

	//websockets
	webSocket.begin("192.168.1.1", 81, "/");
	webSocket.onEvent(webSocketEvent);
	webSocket.setReconnectInterval(5000);
	webSocket.enableHeartbeat(15000, 3000, 2);

	pinMode(16, OUTPUT);
	pinMode(Led, OUTPUT);

	//time
	readingTime = millis() + (1000 * 60);
	blinkTime = millis();

}

void loop()
{
	if (WiFi.status() == WL_CONNECTED) {
		
		if (millis() <= readingTime) {
			if (dht.readStatus()){
				blink(0);
				delay(10);
				temp = dht.readTemperature();
				hum = dht.readHumidity();

				int ilosc = 2;

				DynamicJsonDocument dane(256);
				dane["type"] = "sensorData";
				dane["ilosc"] = ilosc;
				dane["val1"]["name"] = "TunelTemp";
				dane["val1"]["val"] = temp;
				dane["val2"]["name"] = "TunelHum";
				dane["val2"]["val"] = hum;
				dane["globalName"] = "TUNEL";

				String x;
				serializeJson(dane, x);
				webSocket.sendTXT(x);
				delay(500);
				Serial.println("SLEEP");
				delay(5000);
				ESP.deepSleep(20e7);

			}
			else {
				Serial.println("DHT wrong status");
				blink(2);
			}
		}
		webSocket.loop();
		

	}
	else {
		Serial.println("Not connected");
		blink(1);
	}


	delay(300);
}


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

	switch (type) {
	case WStype_DISCONNECTED:
		USE_SERIAL.printf("[WSc] Disconnected!\n");
		break;
	case WStype_CONNECTED: {
		USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);

		// send message to server when Connected
		webSocket.sendTXT("Connected");
	}
						   break;
	case WStype_TEXT:
		USE_SERIAL.printf("[WSc] get text: %s\n", payload);

		// send message to server
		// webSocket.sendTXT("message here");
		break;
	case WStype_BIN:
		USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
		hexdump(payload, length);

		// send data to server
		// webSocket.sendBIN(payload, length);
		break;
	case WStype_PING:
		// pong will be send automatically
		USE_SERIAL.printf("[WSc] get ping\n");
		break;
	case WStype_PONG:
		// answer to a ping we send
		USE_SERIAL.printf("[WSc] get pong\n");
		break;
	}

}

