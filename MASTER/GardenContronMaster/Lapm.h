// Lapm.h

#ifndef _LAPM_h
#define _LAPM_h
#define ORANGE "#eb8900"
#define GREEN "#23c48e"

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
	#include "SPIFFS.h"
	#include "ArduinoJson.h"
	#include "FS.h"
	#include "Blynk.h"
#else
	#include "WProgram.h"
#endif

class LapmClass
{
 protected:
	

 public:
	String name;
	uint8_t pin;
	uint8_t btnPin;
	uint8_t motionPin;
	uint8_t lastState = LOW;
	uint8_t status;
	int vPin;
	unsigned long timeToOff;
	unsigned int lighteningTime; //in second
	bool onByMotion = false; // is on by motion
	bool possibleOnByMotion = false; //can motion turn on it?
	

	uint8_t write(uint8_t stan){
		digitalWrite(this->pin, stan);
		this->status = stan;
		if (this->vPin != NULL) {
			Blynk.virtualWrite(vPin, status);
		}
		this->onByMotion = false;
		return stan;
	}

	uint8_t read(){
		return this->status;
	}

	uint8_t change(){
		digitalWrite(this->pin, !(this->status));
		this->status = !(this->status);
		Blynk.virtualWrite(vPin, status);
		return this->status;
	}

	bool setValues(String tmp_name){
		this->name = tmp_name;
		Serial.println("JESTEM w SETVALUES");
		if(SPIFFS.begin()){
			if(SPIFFS.exists("/config_lamps.json")){
				File config_file = SPIFFS.open("/config_lamps.json");
				if(config_file){

					StaticJsonDocument<64> filter;
					filter[name] = true;

					DynamicJsonDocument conf(512);
					deserializeJson(conf, config_file, DeserializationOption::Filter(filter));

					this->pin = conf[name]["pin"];
					this->btnPin = conf[name]["btnPin"];
					this->motionPin = conf[name]["motionPin"];
					this->vPin = conf[name]["vPin"];
					this->lighteningTime = conf[name]["lighteningTime"];
					this->possibleOnByMotion = conf[name]["possibleOnByMotion"];
					

					config_file.close();
				}
				
			}
			else {
				Serial.println("Failed to load json config");
				Serial.println("Trying to set default...");

				File config_file = SPIFFS.open("/config_lamps.json", FILE_WRITE);
				if (config_file) {


					DynamicJsonDocument conf(1024);
					conf["osSciezka"]["pin"] = 18;
					conf["osAltana"]["pin"] = 22;
					conf["osOgrod"]["pin"] = 23;
					conf["osPodjazd"]["pin"] = 21;
					conf["osOglSciezka"]["pin"] = 19;
					conf["osPrzyciski"]["pin"] = 17;

					conf["osSciezka"]["btnPin"] = 26;
					conf["osAltana"]["btnPin"] = 25;
					conf["osOgrod"]["btnPin"] = 33;
					conf["osPodjazd"]["btnPin"] = NULL;
					conf["osOglSciezka"]["btnPin"] = NULL;

					conf["osSciezka"]["motionPin"] = 13;
					conf["osAltana"]["motionPin"] = 13;
					conf["osOgrod"]["motionPin"] = 13;
					conf["osPodjazd"]["motionPin"] = 13;
					conf["osOglSciezka"]["motionPin"] = 13;

					conf["osSciezka"]["vPin"] = V12;
					conf["osAltana"]["vPin"] = V14;
					conf["osOgrod"]["vPin"] = V11;
					conf["osPodjazd"]["vPin"] = V16;
					conf["osOglSciezka"]["vPin"] = V13;
					conf["osPrzyciski"]["vPin"] = NULL;

					conf["osSciezka"]["lighteningTime"] = 25;
					conf["osAltana"]["lighteningTime"] = 25;
					conf["osOgrod"]["lighteningTime"] = 25;
					conf["osPodjazd"]["lighteningTime"] = 25;
					conf["osOglSciezka"]["lighteningTime"] = 25;

					conf["osSciezka"]["possibleOnByMotion"] = false;
					conf["osAltana"]["possibleOnByMotion"] = false;
					conf["osOgrod"]["possibleOnByMotion"] = false;
					conf["osPodjazd"]["possibleOnByMotion"] = true;
					conf["osOglSciezka"]["possibleOnByMotion"] = true;
					conf["osPrzyciski"]["possibleOnByMotion"] = false;
					
					

					serializeJson(conf, config_file);
					config_file.close();

					this->setValues(name);
				}

			}
		}
	}

	String getName() {
		return this->name;
	}

	String getInfo() {
		char buffer[4];
		String tmpPin = itoa(this->pin, buffer, 10);
		String tmpStatus = itoa(this->status, buffer, 10);
		String end = (this->name + "- Pin: " + tmpPin + " Status: " + tmpStatus);
		Serial.println(end);
		return end;
	}

	bool btnRead(){
		if (this->btnPin != NULL) {
			if (digitalRead(this->btnPin) == HIGH && this->lastState != HIGH) {
				delay(2);
				if (digitalRead(this->btnPin) == HIGH) {
					this->change();
					this->lastState = HIGH;
					this->getInfo();
					return true;
				}
			}
			else if (digitalRead(this->btnPin) == LOW && this->lastState == HIGH) {
				this->lastState = LOW;
				return false;
			}
		}
		
	}

	bool motionRead() {
		if ((this->motionPin != NULL) && (this->possibleOnByMotion == true)){

			if (digitalRead(this->motionPin) == HIGH && this->status != HIGH) {
				this->write(HIGH);
				this->offByTime(((this->lighteningTime)*1000));
			}
		}
	}


	unsigned int offByTime(unsigned long t = NULL) {

		if (t != NULL){
			
			this->timeToOff = millis() + t;
			onByMotion = true;
		}
		else if(this->onByMotion == true){


			if (this->timeToOff <= millis()){

				Blynk.setProperty(vPin, "color", GREEN);
				Blynk.setProperty(vPin, "onLabel", status);
				
				this->write(LOW);
				onByMotion = false;

				return 0;
			}
			else {
				Blynk.setProperty(vPin, "color", ORANGE);
				Blynk.setProperty(vPin, "onLabel", ((timeToOff - millis()) /1000 ));
				return ((this->timeToOff - millis()) / 1000); //return quanity to turn off in second
			}
		}
		else {
			Blynk.setProperty(vPin, "color", GREEN);
			Blynk.setProperty(vPin, "onLabel", "ON");
			return 0; //doesnt on by motion sensor, just manually
		}

	}

	void blynkSync(bool mode = false) {
		if (mode == false) { //check before
			if (this->status == LOW) {
				Blynk.syncVirtual(this->vPin);
			}
			else {
				Blynk.virtualWrite(this->vPin, this->status);
			}
		}
	}

};

extern LapmClass Lapm;



#endif
