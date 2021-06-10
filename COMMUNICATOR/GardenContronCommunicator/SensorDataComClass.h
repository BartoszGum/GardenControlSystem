// SensorDataComClass.h

#ifndef _SENSORDATACOMCLASS_h
#define _SENSORDATACOMCLASS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class SensorDataComClass
{
 protected:


 public:
	void init();
};

extern SensorDataComClass SensorDataCom;

#endif

