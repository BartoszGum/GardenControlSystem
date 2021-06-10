/*
 Name:		GardenContronMaster.ino
 Created:	14.04.2021 20:42:57
 Author:	Bartosz Gumula vel. Askabius
*/



#include "sensorLightClass.h"
#include "sensorClass.h"
#include "timeClass.h"
#include "time.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <stdlib.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <FS.h>
#include <BlynkSimpleEsp32.h>
#include <Blynk.h>
#include <Wire.h>
#include <String.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <WireSlaveRequest.h>
#include "Lapm.h"
#include "esp_system.h"



//definitions

#define USE_SERIAL Serial
#define BLYNK_PRINT Serial
#define MAX_SLAVE_RESPONSE_LENGTH 64
#define SDA 16
#define SCL 4
#define C_READ 0
#define C_WRITE 1

//handles

TaskHandle_t blynkTask = NULL;
TaskHandle_t checkButtonsTask = NULL;
TaskHandle_t turnOffByTimeTask = NULL;
TaskHandle_t timeTask = NULL;
TaskHandle_t motionReadTask = NULL;
TaskHandle_t I2CTask = NULL;
TaskHandle_t updateLocalSensorsTask = NULL;
TaskHandle_t changeLCDTask = NULL;

//queues

//classes

LapmClass osOgrod;
LapmClass osSciezka;
LapmClass osPodjazd;
LapmClass osAltana;
LapmClass osOglSciezka;
LapmClass osPrzyciski;

timeClass localTime;

sensorTmpClass temp1; 
sensorTmpClass temp2;
sensorLightClass light1;
sensorLightClass light2;

//global variables
TwoWire x = TwoWire(1);
DynamicJsonDocument sensorsData(256);
DynamicJsonDocument LCDData(128);

bool restartInfo = true;
bool isI2COn = true;
bool isBlynkManualSycnOn = true;


//Blynk variables
BlynkTimer timer;
WidgetTerminal t(V0);
WidgetRTC rtc;
WidgetLCD lcd(V1);

//WiFi variables
WiFiMulti wifiMulti;
char auth[] = "mdtl6aqxtZSfzBu0JmGvQ1p46AoH8HBB";
char ssid[] = "B593-5478";
char pass[] = "658B9D5B29F";



void basicConfiguration(bool mode = C_READ) {
	switch (mode) {
	case C_READ: {

		if (SPIFFS.begin()) {
			if (SPIFFS.exists("/basic_config.json")) {
				File config_file = SPIFFS.open("/basic_config.json");
				if (config_file) {

					DynamicJsonDocument conf(128);
					deserializeJson(conf, config_file);
					config_file.close();

					isI2COn = conf["isI2COn"];
					isBlynkManualSycnOn = conf["isBlynkManualSycnOn"];
				}
				else {
					Serial.println("Failed to load json basic config");
				}
			}

		}
		else {
			Serial.println("Blad otwierania systemu plikow");
		}


	}break;
	case C_WRITE: {

		if (SPIFFS.begin()) {
			if (SPIFFS.exists("/basic_config.json")) {
				SPIFFS.remove("/basic_config.json");
			}
			File config_file = SPIFFS.open("/basic_config.json");
			if (config_file) {

				DynamicJsonDocument conf(128);


				conf["isI2COn"] = isI2COn;
				conf["isBlynkManualSycnOn"] = isBlynkManualSycnOn;

				serializeJson(conf, config_file);
				config_file.close();
			}
			else {
				Serial.println("Failed to load json basic config");
			}

		}

	}break;
	}

}


void setup() {

	delay(350);
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	SPIFFS.begin(true);
	delay(20);

	//remove conf afer changing 
	//SPIFFS.remove("/config_lamps.json");
	SPIFFS.remove("/config_sensors.json");
	
	localTime.readConf();
	basicConfiguration();

	pinModeFn();
	osOgrod.setValues("osOgrod");
	osSciezka.setValues("osSciezka");
	osAltana.setValues("osAltana");
	osPodjazd.setValues("osPodjazd");
	osOglSciezka.setValues("osOglSciezka");
	osPrzyciski.setValues("osPrzyciski");
	/*
	osOgrod.write(LOW);
	osSciezka.write(LOW);
	osAltana.write(LOW);
	osPodjazd.write(LOW);
	osOglSciezka.write(LOW);
	osPrzyciski.write(LOW);
	*/
	temp1.setValues("temp1");
	temp1.setValues("temp2");
	light1.setName("light1");
	light1.pin = 34;
	light2.setName("light2");
	light2.pin = 35;
	
	//Core 1:

	if ((xTaskCreatePinnedToCore(blynkFun, "blynkTask", 100000, NULL, 4, &blynkTask, 1)) == pdPASS){
		delay(400);
	//	configASSERT(blynkTask); delay(200);
	}
	
	if ((xTaskCreatePinnedToCore(checkButtonsFun, "checkButtonsTask", 15000, NULL, 3, &checkButtonsTask, 1)) == pdPASS){
		delay(400);
	//	configASSERT(checkButtonsTask); delay(200);
	}
	
	if ((xTaskCreatePinnedToCore(turnOffByTimeFun, "turnOffByTimeTask", 15000, NULL, 1, &turnOffByTimeTask, 1)) == pdPASS) {
		delay(400);
	//	configASSERT(turnOffByTimeTask); delay(200);
	}

	if ((xTaskCreatePinnedToCore(motionReadFun, "motionReadTask", 8000, NULL, 4, &motionReadTask, 1)) == pdPASS) {
		delay(400);
	//	configASSERT(motionReadTask); delay(200);
	}

	if ((xTaskCreatePinnedToCore(updateLocalSensorsFun, "updateLocalSensorsTask", 8000, NULL, 0, &updateLocalSensorsTask, 1)) == pdPASS) {
		delay(400);
	//	configASSERT(updateLocalSensorsTask); delay(200);
	}
	if ((xTaskCreatePinnedToCore(changeLCDFun, "changeLCDTask", 4000, NULL, 1, &changeLCDTask, 1)) == pdPASS) {
		delay(400);
		//	configASSERT(updateLocalSensorsTask); delay(200);
	}
	

	

	//Core 0:

	if ((xTaskCreatePinnedToCore(timeFun, "timeTask", 15000, NULL, 2, &timeTask, 0)) == pdPASS) {
		delay(400);
	//	configASSERT(timeTask); delay(200);
	}
	
	if ((xTaskCreatePinnedToCore(I2CFun, "I2CTask", 15000, NULL, 3, &I2CTask, 0)) == pdPASS) {
		delay(400);
	//	configASSERT(I2CTask); delay(200);
	}
	



	

}

void loop() {

	// not important things :

	vTaskDelay(50);
	
}


bool WiFiInitial()
{
	WiFi.setSleep(false);
	wifiMulti.addAP(ssid, pass);
	wifiMulti.addAP("B593-5478v2", pass);
	wifiMulti.addAP("B593-5478_5GHz", pass);
	delay(50);
	Serial.println("Connecting Wifi...");

	if (wifiMulti.run() == WL_CONNECTED) {
		Serial.println("");
		Serial.println("WiFi connected");
		Serial.println("IP address: ");
		Serial.println(WiFi.localIP());
		return true;
	}
	else {
		return false;
	}
}

bool WiFiRestart() {
	WiFi.disconnect();
	delay(300);
	if (WiFiInitial()) {
		return true;
	}
	else {
		return false;
	}
}



void pinModeFn() {
	pinMode(23, OUTPUT);
	pinMode(22, OUTPUT);
	pinMode(21, OUTPUT);
	pinMode(19, OUTPUT);
	pinMode(18, OUTPUT);
	pinMode(17, OUTPUT);

	pinMode(34, INPUT);
	pinMode(35, INPUT);
	pinMode(32, INPUT);
	pinMode(33, INPUT);
	pinMode(25, INPUT);
	pinMode(27, INPUT);
	pinMode(26, INPUT);
	pinMode(13, INPUT);
	delay(5);
}

void blynkFun(void * param) {

	unsigned long timeToSync = millis() + 10000; 
	unsigned long WTDWiFi = millis();

	Blynk.config(auth);
	delay(500);

	if (WiFiInitial()) {
		Blynk.connect();
		delay(3000);
	}
	else {
		Serial.println("Problem z polaczeniem WiFi");
	}

	for (;;){
		
		if (Blynk.connected()) {
			Blynk.run();
			WTDWiFi = millis();
		}
		else{
			if (wifiMulti.run() == WL_CONNECTED) {
				delay(10);
				if (Blynk.connected()) {
					Blynk.run();
					WTDWiFi = millis();
				}
				else {
					Serial.println("Wywoluje Blynk.connect();");
					Blynk.connect();
					vTaskDelay(4000);
				}
			}
			else if (WiFi.status() == WL_NO_SSID_AVAIL){
				Serial.println("Nie ma sieci o tym SSID");
				WTDWiFi = millis();
				WiFiRestart();
				vTaskDelay(4000);
			}
		}


		if ((timeToSync <= millis()) && (isBlynkManualSycnOn == true)){
				//btnSync
				Serial.println("Synchronizacja BLYNK z ESP");
				Blynk.virtualWrite(osAltana.vPin, osAltana.status);
				Blynk.virtualWrite(osSciezka.vPin, osSciezka.status);
				Blynk.virtualWrite(osOgrod.vPin, osOgrod.status);
				Blynk.virtualWrite(osPodjazd.vPin, osPodjazd.status);
				Blynk.virtualWrite(osOglSciezka.vPin, osOglSciezka.status);
				//sensorsSync
				temp1.blynkWrite();
				temp2.blynkWrite();
			

			timeToSync = millis() + (1000*120);
		}
		if (WTDWiFi + (1000 * 60 * 2) < millis()){
			Serial.println("Blad WiFi.");
			delay(200);
			if (WiFiRestart()) {
				delay(1000);
				Blynk.config(auth);
				delay(400);
				Blynk.connect();
				delay(4000);

				if (Blynk.connected()) {
					Blynk.run();
					delay(5);
					Serial.println("Proba naprawy powiodla sie.");
					WTDWiFi = millis();
				}
				else {
					Serial.println("Restart because of Blynk");
					ESP.restart();
				}
			}
			else {
				Serial.println("Restart because of WiFi");
				ESP.restart();
			}
			
		}

		vTaskDelay(150);
	}

	blynkTask = NULL;
	vTaskDelete(NULL);

}

void checkButtonsFun(void * param) { //check buttons and IR sensors
	vTaskDelay(4000);

	for (;;) {
		osAltana.btnRead();
		osOglSciezka.btnRead();
		osOgrod.btnRead();
		osPodjazd.btnRead();
		osSciezka.btnRead();
		vTaskDelay(170);
	}

	checkButtonsTask = NULL;
	vTaskDelete(NULL);
}

void turnOffByTimeFun(void * param){ //checking if it's time to turn off and changing value of this time in blynk app

	vTaskDelay(15000);

	for (;;) {
		
		osPodjazd.offByTime();
		osAltana.offByTime();
		osOglSciezka.offByTime();
		osOgrod.offByTime();
		osSciezka.offByTime();

		vTaskDelay(600);
	}

	turnOffByTimeTask = NULL;
	vTaskDelete(NULL);

}


void timeFun(void * param) {
	vTaskDelay(8000);

	for (;;){

		if (Blynk.connected()) {
			localTime.year = year();
			localTime.month = month();
			localTime.day = day();
			localTime.hour = hour();
			localTime.minute = minute();
			Serial.println(localTime.get());
		}

		vTaskDelay(999 * 60); //one minute repeat
	}

	timeTask = NULL;
	vTaskDelete(NULL);
}


void motionReadFun(void * param) {
	vTaskDelay(20000);
	bool isWorking = true;
	unsigned long checkParamsNextTime = (millis() + (1000 * 5)); //for 5 second (first fime for sure)

	for (;;)
	{
		if (millis() >= checkParamsNextTime) {

			localTime.setMotionStatus();

			if ((localTime.isMotionEnable == true) && ((light1.isNight() == true) || (light2.isNight() == true))) {
				isWorking = true;
				osPrzyciski.write(HIGH);
			}
			else {
				isWorking = false;
				osPrzyciski.write(LOW);
			}

			Serial.print("Sprawdzono parametry do MOTION, isWorking = ");
			Serial.println(isWorking);

			checkParamsNextTime = (millis() + (1000 * 60 * 3));
		}
		
		if (isWorking == true) {

			osPodjazd.motionRead();
			osSciezka.motionRead();
			osOglSciezka.motionRead();
			osOgrod.motionRead();
			osAltana.motionRead();

			vTaskDelay(170);
		}
		else {
			vTaskDelay(1000 * 60); //suspend for one minute
		}
	}

	motionReadTask = NULL;
	vTaskDelete(NULL);
}


void I2CFun(void * param) {
	
	vTaskDelay(5000);
	String message = "";
	Serial.println("Wywoluje x.begin();");
	bool status = (x.begin(16, 4));
	vTaskDelay(2000);
	scanI2C();

	for (;;){

		if (isI2COn) {

			if (!status) {
				Serial.println("Wywoluje x.begin();");
				status = (x.begin(16, 4));
			}

			WireSlaveRequest slaveReq(x, 0x04, MAX_SLAVE_RESPONSE_LENGTH);


			bool success = slaveReq.request();
			String temp = "";

			if (success) {
				while (1 < slaveReq.available()) {
					char c = slaveReq.read();
					temp += c;

				}
				Serial.println(temp);
				saveData(temp);
				deserializeJson(sensorsData, temp);

			}
			else {
				Serial.println(slaveReq.lastStatusToString());
				Serial.println("Wywoluje x.begin();");
				status = (x.begin(16, 4));
				vTaskDelay(2000);
				//maybe also begin();
			}
		}
		vTaskDelay(1000*60*3);
	}

	I2CTask = NULL;
	vTaskDelete(NULL);
}


void scanI2C() {

	int nDevices;
	byte error, address;

	for (address = 1; address < 127; address++)
	{
		// The i2c_scanner uses the return value of
		// the Write.endTransmisstion to see if
		// a device did acknowledge to the address.
		x.beginTransmission(address);
		error = x.endTransmission();

		if (error == 0)
		{
			Serial.print("I2C device found at address 0x");
			if (address < 16)
				Serial.print("0");
			Serial.print(address, HEX);
			Serial.println("  !");

			nDevices++;
		}
		else if (error == 4)
		{
			Serial.print("Unknow error at address 0x");
			if (address < 16)
				Serial.print("0");
			Serial.println(address, HEX);
		}
	}
}


bool saveData(String tmp) {
	
	if (SPIFFS.begin()) {

		if (SPIFFS.exists("/data_Sensors_new.json")) {
			if (SPIFFS.exists("/data_Sensors_old.json")){
				SPIFFS.remove("/data_Sensors_old.json");
			}

			SPIFFS.rename("/data_Sensors_new.json", "/data_Sensors_old.json"); //rename as old
			saveData(tmp);
		}
		else{
			File config_file = SPIFFS.open("/data_Sensors_new.json", FILE_WRITE);
			config_file.print(tmp);
			config_file.close();
		}

		return true;
	}
	else{
		Serial.println("Blad otwierania systemu plikow");
		return false;
	}
}


void updateLocalSensorsFun(void * param) {

	vTaskDelay(1000 * 50);

	for (;;) {

		if (isI2COn) {

			String s = "";
			serializeJson(sensorsData, s);
			Serial.print("UPD: ");
			Serial.println(s);


			if (!sensorsData["T1"].isNull()) { //exist that sensorsData? Idk it's correct
				temp1.temperature = sensorsData["T1"]["tmp"];
				temp1.s_val = sensorsData["T1"]["hum"];
				Serial.println("Istnieje T1, pobrano dane");
			}
			if (!sensorsData["T2"].isNull()) {
				temp2.temperature = sensorsData["T2"]["tmp"];
				temp2.s_val = sensorsData["T2"]["atm"];
				Serial.println("Istnieje T2, pobrano dane");
			}

			if (!sensorsData["ADD"].isNull()) {
				int howMany = sensorsData["ADD"];
				if (howMany >= 1) {
					LCDData["howMany"] = howMany;
					for (int i = 1; i <= howMany; i++) {
						String currentVal = "val" + String(i); //val1, val2...
						LCDData[howMany]["name"] = sensorsData[currentVal]["name"];
						LCDData[howMany]["val"] = sensorsData[currentVal]["val"];
					}
				}
			}

		}
		vTaskDelay(1000 * 60 * 6);
	}

	vTaskDelete(NULL);
}


void changeLCDFun(void * param) {
	lcd.clear();
	String displayVal1 = "X";
	String displayVal2 = "Y";
	lcd.print(0, 0, displayVal1);
	lcd.print(0, 1, displayVal2);
	for (;;) {
		if (!sensorsData["ADD"].isNull()) {
			int howMany = sensorsData["ADD"];
			if (howMany >= 1) {

				for (int i = 1; i <= howMany; i++) {
					String currentVal = "val" + String(i); //val1, val2...
					displayVal2 = displayVal1;
					String tmpX = sensorsData[currentVal]["name"];
					String tmpY = sensorsData[currentVal]["val"];
					displayVal1 += tmpX + ": " + tmpY;

					lcd.print(0, 0, displayVal1);
					lcd.print(0, 1, displayVal2);
					vTaskDelay(1000 * 10);
				}
			}

			if (howMany <= 2) {
				vTaskDelay(1000 * 60);
			}
			else {
				vTaskDelay(1000 * 20);
			}
		}
		
	}
	vTaskDelete(NULL);

}



//Virtual Pins

BLYNK_CONNECTED() {
	osAltana.blynkSync();
	osPodjazd.blynkSync();
	osSciezka.blynkSync();
	osOgrod.blynkSync();
	osOglSciezka.blynkSync();
	vTaskDelay(5);
	rtc.begin();
	vTaskDelay(20);
	if (restartInfo){
		t.print("Nastapil restart: ");
		t.println(localTime.get());
		t.flush();
		restartInfo = false;
	}
	
}

BLYNK_WRITE(V11) //virtualPin11 //ogrod
{
	osOgrod.write(param.asInt());
	osOgrod.getInfo();
}
BLYNK_WRITE(V12) 
{
	osSciezka.write(param.asInt());
	osSciezka.getInfo();
}
BLYNK_WRITE(V13) 
{
	osOglSciezka.write(param.asInt());
	osOglSciezka.getInfo();
}
BLYNK_WRITE(V14) 
{
	osAltana.write(param.asInt());
	osAltana.getInfo();

}
BLYNK_WRITE(V15)
{

}
BLYNK_WRITE(V16)
{
	osPodjazd.write(param.asInt());
	osPodjazd.getInfo();
}

BLYNK_WRITE(V33)
{
	bool state = param.asInt();
	
	osAltana.write(state);
	osOglSciezka.write(state);
	osOgrod.write(state);
	osSciezka.write(state);
	osPodjazd.write(state);
}




BLYNK_WRITE(V0)
{

	String mess = param.asStr();
	bool saveConf = false;

	if(mess == "help" || mess == "HELP"){
		HELP_TERMINAL();

	}
	else if (mess == "info" || mess == "INFO") {
		info();
	}
	else if (mess.substring(0, 3) == "set"){

		if (mess.substring(4, 11) == "offTime"){

			localTime.offHour = (mess.substring(12,14)).toInt(); // 23 f.e.
			localTime.onHour = (mess.substring(15,17)).toInt();  // 6 f.e.
			localTime.saveConf();
			t.println("OK, saved to SPIFFS");
			t.print("offHour: ");
			t.println(localTime.offHour);
			t.print("onHour: ");
			t.println(localTime.onHour);
			t.flush();

		}
		else if (mess.substring(4, 12) == "lightVal"){
			if (mess.substring(12, 13) == "1") {
				light1.valueToNight = (mess.substring(14, 18).toInt());
				light1.saveConf();
				t.print("lightVal1: ");
				t.println(light1.valueToNight);
				t.flush();
				
			}
			else if (mess.substring(12, 13) == "2") {
				light2.valueToNight = (mess.substring(14, 18).toInt());
				light2.saveConf();
				t.print("lightVal2: ");
				t.println(light2.valueToNight);
				t.flush();
				
			}
			else {
				t.println("[E] Niepoprawny numer");
				t.flush();
			}
		}
		else if (mess.substring(4, 13) == "lightTime") {
			int tmp_val = (mess.substring(14, 17).toInt());
			t.println("OK, wymagany restart");
			t.flush();
			
			// do dokonczenia - podmiana w pliku konfiguracyjnym i restart ESP
			}
		
		else if (mess.substring(4, 7) == "i2c") {
			int mode = (mess.substring(8, 9)).toInt();
			isI2COn = mode;
			t.print("isI2COn: ");
			t.println(isI2COn);
			t.flush();
			saveConf = true;
		}
		else if (mess.substring(4, 7) == "syn") {
			int mode = (mess.substring(8, 9)).toInt();
			isBlynkManualSycnOn = mode;
			t.print("isBlynkManualSyncOn: ");
			t.println(isBlynkManualSycnOn);
			t.flush();
			saveConf = true;
		}
		else {
			t.println();
			t.println("Niepoprawna instrikcja set");
			t.println(mess.substring(5, 12));
			t.flush();
			HELP_TERMINAL();
		}
	}
	else if (mess.substring(0, 4) == "rssi") {
		t.print("RSSI: ");
		t.println(WiFi.RSSI());
		t.flush();
	}
	else {
		t.println(mess);
		t.flush();
	}
	 
	//save new basic conf to SPIFFS if was changed
	if (saveConf) {
		basicConfiguration(C_WRITE);
	}
	
}


void HELP_TERMINAL() {

	t.println("--> set <-- ");
	t.print("+ offTime #int(2) #int(1) ");
	t.print("//offTime 22 6 \n");
	t.flush();
	t.print("+ lightTime1 #int(2-3) ");
	t.print("//lightTime 30 \n");
	t.print("+ lightTime2 #int(2-3) ");
	t.print("//lightTime 110 \n");
	t.flush();
	t.print("+ lightVal1 #int(4) //lightTime 1900 \n");
	t.print("+ lightVal2 #int(4) //lightTime 2250 \n");
	t.flush();
	t.println("+ i2c #int(1) //i2c 0");
	t.println("+ syn #int(1) //syn 1");
	t.flush();
	t.println("--> info <--");
	t.flush();
}

void info() {
	t.println("----> Informacje <----");
	t.flush();
	t.print("Czas lokalny: ");
	t.println(localTime.get());
	t.flush();
	t.print("offHour: ");
	t.println(localTime.offHour);
	t.print("onHour: ");
	t.println(localTime.onHour);
	t.flush();
	t.println(light1.get());
	t.println(light2.get());
	t.flush();
	t.print("RSSI: ");
	t.println(WiFi.RSSI());
	t.flush();
	
}


void obwInfo() {
	t.println(osAltana.getInfo());
	t.println(osSciezka.getInfo());
	t.println(osPodjazd.getInfo());
	t.println(osOgrod.getInfo());
	t.println(osOglSciezka.getInfo());
	t.flush();
}
