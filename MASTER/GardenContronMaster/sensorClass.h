// sensorTmpClass.h

#ifndef _SENSORCLASS_h
#define _SENSORCLASS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Blynk.h"
	#include "BlynkSimpleEsp32.h"
	#include "arduino.h"
	#include "SPIFFS.h"
	#include <ArduinoJson.h>
	#include "FS.h"

#else
	#include "WProgram.h"
#endif

class sensorTmpClass
{
 public:
	 String name;
	 float temperature;
	 float s_val;
	 uint8_t vPin1 = V22; //to sending temp
	 uint8_t vPin2 = V23; //to sending senond value


	 void blynkWrite() {

		 if (this->temperature != NULL) {
			 Blynk.virtualWrite(vPin1, temperature);
		 }
		 else {
		 }

		 if (this->s_val != NULL) {
			 Blynk.virtualWrite(this->vPin2, this->s_val);
		 }
		 
	 }

	 bool setValues(String tmp_name = "undefinedName") {
		 this->name = tmp_name;
		 Serial.println("JESTEM w SETVALUES");
		 if (SPIFFS.begin()) {
			 if (SPIFFS.exists("/config_sensors.json")) {
				 File config_file = SPIFFS.open("/config_sensors.json");
				 if (config_file) {

					 StaticJsonDocument<32> filter;
					 filter[name] = true;

					 DynamicJsonDocument conf(128);
					 deserializeJson(conf, config_file, DeserializationOption::Filter(filter));

					 this->vPin1 = conf[name]["vPin1"];
					 this->vPin2 = conf[name]["vPin2"];
					 


					 config_file.close();
				 }

			 }
			 else {
				 Serial.println("Failed to load json config");
				 Serial.println("Trying to set default...");

				 File config_file = SPIFFS.open("/config_sensors.json", FILE_WRITE);
				 if (config_file) {


					 DynamicJsonDocument conf(128);
					 conf["temp1"]["vPin1"] = V21;
					 conf["temp1"]["vPin2"] = V24;
					 conf["temp2"]["vPin1"] = V22;
					 conf["temp2"]["vPin2"] = V23;
					 
					 

					 serializeJson(conf, config_file);
					 config_file.close();

					 this->setValues(name);
				 }

			 }
		 }
	 }

};

extern sensorTmpClass sensor;

#endif

