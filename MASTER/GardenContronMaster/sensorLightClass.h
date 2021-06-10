// sensorLightClass.h

#ifndef _SENSORLIGHTCLASS_h
#define _SENSORLIGHTCLASS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#else
	#include "WProgram.h"
#endif

class sensorLightClass
{
 public:
	 String name;
	 int light = 0;
	 int valueToNight = 1800;
	 uint8_t pin = NULL;

	 void setName(String x = "undefinedName") {
		 this->name = x;
		 this->readConf();
	 }

	 bool isNight(){
		 if (this->pin != NULL) {
			 this->light = analogRead(this->pin);
		 }
		 if (this->light >= this->valueToNight){
			 return true;
		 }
		 else {
			 return false;
		 }

	 }

	 String get() {
		 return (String(this->name) + " value: " + String(this->light) + " valueN: " + String(this->valueToNight));
	 }

	 void saveConf() {
		 if (SPIFFS.begin()) {
			 String fileName = ("/config_" + this->name);
			 if (SPIFFS.exists(fileName)) {
				 SPIFFS.remove(fileName);
			 }

			 File config_file = SPIFFS.open(fileName, FILE_WRITE);
			 if (config_file) {


				 DynamicJsonDocument conf(64);

				 conf["valueToNight"] = this->valueToNight;
				 conf["pin"] = this->pin;
				 serializeJson(conf, config_file);
				 config_file.close();

			 }
			 else {
				 Serial.println("Failed to save json");
			 }
		 }
	 }

	 void readConf() {
		 if (SPIFFS.begin()) {
			 String fileName = ("/config_" + this->name);
			 if (SPIFFS.exists(fileName)) {

				 File config_file = SPIFFS.open(fileName);
				 if (config_file) {

					 DynamicJsonDocument conf(64);
					 deserializeJson(conf, config_file);
					 config_file.close();
					 this->valueToNight = conf["valueToNight"];
					 this->pin = conf["pin"];
				 }
				 else {
					 Serial.println("Failed to load json config_light[x]. Set default - 1800");
				 }
			 }
		 }
	 }
	 
};

extern sensorLightClass sensorLight;

#endif

