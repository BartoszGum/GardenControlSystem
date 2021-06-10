/*
    Name:       GardenContronCommunicator.ino
    Created:	16.05.2021 17:28:26
    Author:     BARTEKVIVOBOOK\barto
*/



#include "SensorDataComClass.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <WebSocketsServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <WireSlave.h>
#include <DHT12.h>
//#include <BMP280_DEV.h>
#include <String.h>
#include <driver/i2c.h>
#include <esp32-hal-adc.h>
#include <Adafruit_BMP280.h>




//definitions 
#define ARDUINOJSON_ENABLE_PROGMEM 0
#define USE_SERIAL Serial
#define S Serial
#define SEALEVELPRESSURE_HPA (1013.25)
#define ZM1Pin 18
#define ZM2Pin 19
#define SDA 13
#define SCL 27

//Handles
TaskHandle_t WSTask = NULL;
TaskHandle_t I2CMasterTask = NULL;
TaskHandle_t I2CSensorsTask = NULL;
TaskHandle_t AnalogSensorsTask = NULL;
TaskHandle_t CreateJsonTask = NULL;


//WiFi variables
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
char ssid[] = "SensorsWiFi";
char pass[] = "658B9D5B29F";

//Classes


//Global variables

unsigned long lastMess;
TwoWire wireSensors = TwoWire(1);
void requestEvent();
DHT12 dht12(&wireSensors);
Adafruit_BMP280 bmp(&wireSensors);

DynamicJsonDocument ready(256);
String serializedReady;

void setup() {
	delay(4000); //wait for sure
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	delay(500);

	PinMode();

	lastMess = millis();
	

	if ((xTaskCreatePinnedToCore(WSFun, "WSTask", 50000, NULL, 4, &WSTask, 1)) == pdPASS) {
		delay(400);
		//configASSERT(WSTask);
		delay(400);
	}

	if ((xTaskCreatePinnedToCore(I2CMasterFun, "I2CMasterTask", 20000, NULL, 4, &I2CMasterTask, 0)) == pdPASS) {
		delay(400);
		//configASSERT(I2CMasterTask);
		//delay(400);
	}
	if ((xTaskCreatePinnedToCore(I2CSensorsFun, "I2CSensorsTask", 25000, NULL, 3, &I2CSensorsTask, 1)) == pdPASS) {
		delay(400);
		//configASSERT(I2CSensorsTask);
		//delay(400);
	}
	if ((xTaskCreatePinnedToCore(AnalogSensorsFun, "AnalogSensorsTask", 20000, NULL, 2, &AnalogSensorsTask, 1)) == pdPASS) {
		delay(400);
		//configASSERT(AnalogSensorsTask);
		//delay(400);
	}
	if ((xTaskCreatePinnedToCore(CreateJsonFun, "CreateJsonTask", 20000, NULL, 1, &CreateJsonTask, 1)) == pdPASS) {
		delay(400);
		//configASSERT(CreateJsonTask);
		//delay(400);
	}

}

void loop()
{
	vTaskDelay(250);

}

void WiFiInitial(){

	IPAddress local_IP(192, 168, 1, 1);
	IPAddress gateway(192, 168, 1, 1);
	IPAddress subnet(255, 255, 255, 0);

	WiFi.disconnect();
	WiFi.mode(WIFI_AP);
	delay(3000);
	WiFi.softAPConfig(local_IP, gateway, subnet);
	delay(3000);
	WiFi.softAP(ssid, pass);
	delay(3000);
	S.println("Utworzono siec WiFi");
	S.print("SSID: ");
	S.println(WiFi.SSID());
}

void WSFun(void * param){

	WiFiInitial();
	vTaskDelay(1000);
	webSocket.begin();
	webSocket.onEvent(webSocketEvent);
	ready["ADD"] = 0;

	if (MDNS.begin("esp32")) {
		USE_SERIAL.println("MDNS responder started");
	}

	server.on("/", []() {
		// send index.html
		server.send(200, "text/html", getpage());
	});

	server.begin();
	MDNS.addService("http", "tcp", 80);
	MDNS.addService("ws", "tcp", 81);

	for (;;) {
		webSocket.loop();
		server.handleClient();
		vTaskDelay(50);
	}

	vTaskDelete(NULL);

}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

	Serial.println("webSocketEvent is working");

	switch (type) {
	case WStype_DISCONNECTED:{
		USE_SERIAL.printf("[%u] Disconnected!\n", num);

	}break;

	case WStype_CONNECTED:{
		IPAddress ip = webSocket.remoteIP(num);
		USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

	}break;

	case WStype_TEXT:{
		USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);

		String json = ((char*)payload);
		DynamicJsonDocument dane(1024);
		deserializeJson(dane, json);

		if (dane["type"] == "sensorData") {
			
			//add another sensor to json
			

			int ilosc = dane["ilosc"]; //how much different data f.e. temperature, pressure, humidity... are 3 types
			int tmp_i = ready["ADD"];
			tmp_i += ilosc;
			ready["ADD"] = tmp_i; 


			//name is type, f.e. temp, hum...

			for (int i = (tmp_i - ilosc), j = 1; i <= tmp_i; i++, j++) {
				String currentVal = "val" + String(i);
				ready[currentVal]["name"] = dane[currentVal]["name"];
				ready[currentVal]["val"] = dane[currentVal]["val"];
			}

		}



	}break;


	}
}

void I2CMasterFun(void * param) {

	vTaskDelay(1000);
	unsigned long timeToBeginAgain = millis() + (1000 * 60 * 4);
	bool status = (WireSlave.begin(13, 27, 0x04));
	delay(1000);
	if (!status) {
		Serial.println("I2C Slave init failed");
		while (1) delay(100);
	}
	else {
		Serial.println("I2C Slave init OK");
	}

	WireSlave.onRequest(requestEvent);
	
	for (;;) {
		
		if (millis() >= timeToBeginAgain) {
			/*
			S.println(i2c_reset_tx_fifo(I2C_NUM_0));
			vTaskDelay(100);
			S.println(i2c_reset_rx_fifo(I2C_NUM_0));
			vTaskDelay(100);
			uint8_t *tmp;
			i2c_slave_read_buffer(I2C_NUM_0, tmp, 10, 200);
			
			S.println(i2c_driver_delete(I2C_NUM_0));
			delay(1000);
			S.println((WireSlave.begin(13, 27, 0x04)));
			delay(400);
			WireSlave.onRequest(requestEvent);

			
			S.println("---> RESET I2C <---");
			timeToBeginAgain = millis() + (1000 * 60 * 7);
			*/
		}

		if (lastMess > (millis() + (1000*60*10))) {
			S.println("ESP need restart ---> 5 sec");
			//delay(1000 * 5);
			//ESP.restart();
		}
		

		WireSlave.update();
		vTaskDelay(4);
		
	}

	vTaskDelete(NULL);
}


void requestEvent() {
	
	WireSlave.print(serializedReady);
	Serial.print("SENT: ");
	Serial.println(serializedReady);
	lastMess = millis();
}

void I2CSensorsFun(void * param) {

	vTaskDelay(10 * 1000);

	unsigned long timeToBeginAgain = millis() + (1000 * 60 * 1);
	wireSensors.begin(22, 23, 10000);
	//vTaskDelay(1000);
	if (!bmp.begin(BMP280_ADDRESS_ALT)) {
		Serial.println("Error bmp begining");
	}
	//dht12.begin();

	vTaskDelay(2000);

	bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
		Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
		Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
		Adafruit_BMP280::FILTER_X16,      /* Filtering. */
		Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
	vTaskDelay(3000);

	ready["T1"];
	ready["T2"];

	for (;;) {

		if (millis() >= timeToBeginAgain) {

			
		}

		//float tempDHT12 = dht12.readTemperature();
		//vTaskDelay(100);
		//float humDHT12 = dht12.readHumidity();
		//vTaskDelay(500);
		float tempBMP = bmp.readTemperature();
		//vTaskDelay(500);
		float atmBMP = (bmp.readPressure()/100);


		//dht
		/*
		if ((tempDHT12 > (-40)) && (tempDHT12 < 55)) {
			ready["T1"]["tmp"] = tempDHT12;
		}
		if (((humDHT12 > 0)) && (humDHT12 <= 100)) {
			ready["T1"]["hum"] = humDHT12;
		} 
		*/
		//bmp

		
			if ((tempBMP > (-40)) && (tempBMP < 55)) {
				ready["T2"]["tmp"] = tempBMP;
			}
			if ((atmBMP > 920) && (atmBMP < 1150)) {
				ready["T2"]["atm"] = atmBMP;
			}

			Serial.print("temp: ");
			Serial.println(tempBMP);
			Serial.print("atm: ");
			Serial.println(atmBMP);
		

		

		vTaskDelay(1000 * 60 * 1);
	}

	vTaskDelete(NULL);
}

void AnalogSensorsFun(void * param) {

	vTaskDelay(10000);
	ready["L1"];
	ready["L2"];
	adcAttachPin(ZM1Pin);
	adcAttachPin(ZM2Pin);
	analogReadResolution(10);
	analogSetWidth(10);

	for (;;) {
		int l1 = analogRead(ZM1Pin);
		int l2 = analogRead(ZM2Pin);
		ready["L1"]["val"] = l1;
		ready["L2"]["val"] = l2;
		S.print("L1: ");
		S.println(l1);
		S.print("L2: ");
		S.println(l2);


		vTaskDelay(1000 * 50);
	}

	vTaskDelete(NULL);


}

void PinMode(){
	pinMode(ZM1Pin, INPUT);
	pinMode(ZM2Pin, INPUT);
	pinMode(2, OUTPUT);
}

void CreateJsonFun(void * param) {
	
	vTaskDelay(10000);

	for (;;) {
		String tmp = "";
		serializeJson(ready, tmp);

		if (tmp.length() > 0) {
			serializedReady = tmp;
		}
		Serial.print("serializedReady: ");
		Serial.println(serializedReady);

		vTaskDelay(1000 * 60 * 1);
	}
}


String getpage() {
	return "<html><body>HELLO</body></html>";
}