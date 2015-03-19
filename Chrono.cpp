/*
 * Chrono.cpp
 *
 *  Created on: 16/ago/2014
 *      Author: Tongo
 */

#include "Chrono.h"
 
#define DEBUG_LOG_SETUP false
#define DEBUG_LOG false
#define GPS_BAUD_RATE 57600

Chrono::Chrono(ILI9341_due* lcdTft, Adafruit_GPS* gpsSensor, HardwareSerial *gpsSerial) {
	lcdTft->begin();
	#if DEBUG_LOG_SETUP
	Serial.println("LCD init finish");
	#endif
	chronoGui = ChronoGui();
	#if DEBUG_LOG_SETUP
	Serial.println("ChronoGui init finish");
	#endif
	chronoGui.initTft(lcdTft);
	lapTimer = LapTimer();
	#if DEBUG_LOG_SETUP
	Serial.println("LapTimer init finish");
	#endif

	#if DEBUG_LOG_SETUP
	Serial.println("GPS init...");
	#endif
	this->gps = gpsSensor;
	
	#if DEBUG_LOG_SETUP
	Serial.println("GPS assigned");
	#endif

	this->gps->begin(GPS_BAUD_RATE);
	#if DEBUG_LOG_SETUP
	Serial.println("GPS initialized");
	#endif

	gpsSerial->begin(9600);
	//this->gps->sendCommand(PMTK_SET_BAUD_9600);
	this->gps->sendCommand(PMTK_SET_BAUD_57600);
	gpsSerial->flush();
	delay(100);
	gpsSerial->end();
	delay(100);
	gpsSerial->begin(GPS_BAUD_RATE);
	
	#if DEBUG_LOG_SETUP
	Serial.println("GPS init baudRate selected");
	#endif

	//gps->sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
	//GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
	gps->sendCommand(PMTK_SET_NMEA_OUTPUT_ALLDATA);
	// NMEA in Output frequency
	#if DEBUG_LOG_SETUP
	Serial.println("GPS init OutputData selected");
	#endif
	gps->sendCommand(PMTK_SET_NMEA_UPDATE_10HZ);
	// 1Hz GPS Fix
	gps->sendCommand(PMTK_API_SET_FIX_CTL_5HZ);
	#if DEBUG_LOG_SETUP
	Serial.println("GPS init FixRate selected");
	#endif
	gps->sendCommand(PGCMD_NOANTENNA);
	//GPS.sendCommand(PGCMD_ANTENNA);
	#if DEBUG_LOG_SETUP
	Serial.println("GPS init Antenna selected");
	#endif
	//gpsSerial.println(PMTK_Q_RELEASE);

	#if DEBUG_LOG_SETUP
	Serial.println("LapTimer init finish");
	#endif

	gpsFixState = false;
	gpsFixStateLedTimestamp = 0;
	gpsStelliteNumber = 0;
	chronoGui.updateGpsFixState(gps->fix);

	#if DEBUG_LOG_SETUP
	Serial.println("GPS init finish");
	#endif

	chronoGui.updateLapNumber(lapTimer.getLapNumber());
	chronoGui.updateGpsSatelliteNumber(gpsStelliteNumber);

	simulateNewLap = false;
	//lastPoint = new GpsPoint(0.0, 0.0);
	//newPoint = new GpsPoint(0.0, 0.0);

	track = new Track();

	// SD-Card
	this->useSdCard = false;
	this->logFile = NULL;

	// TouchScreen
	this->touchScreenManager = NULL;

	chronoGui.updateGearCounter(0);
}

Chrono::~Chrono() {
	// TODO Auto-generated destructor stub
}

void Chrono::setLogSdCard(bool useSdCard) {
	this->useSdCard = useSdCard;
	if(logFile == NULL) {
		this->logFile = new SdFile();
		Serial.println("Log file initialization...");
		if (logFile->open("syslog.log", O_WRITE | O_CREAT | O_APPEND)) {
			Serial.println("Log file OK");
			logFile->println("*** LOGGING ***");
			logFile->close();
		} else {
			Serial.println("Log file initialization ERROR");
		}
	}
}

void Chrono::setTouchScreen(TouchScreen* touchScreen) {
	this->touchScreenManager = new TouchScreenManager(touchScreen);
}

void Chrono::loopChrono(void) {

	if(this->touchScreenManager != NULL) {
		int operation = this->touchScreenManager->getChronoOperation();
		if(operation > 0) {		
			Serial.print("Operation: ");
			Serial.println(operation);
		}
	}

	#if DEBUG_LOG
	Serial.println("Reading gpsSerial...");
	#endif
	while(char c = gps->read()) {
		#if DEBUG_LOG
		Serial.print(c);
		#endif
	}
	#if DEBUG_LOG
	Serial.println("gpsSerial red");
	#endif

	if (gps->newNMEAreceived()) {
		#if DEBUG_LOG
		Serial.println(gps->lastNMEA());
		#endif
		// NMEA parsing
		gps->parse(gps->lastNMEA());

		if (gps->fix) {
			// log
		#if DEBUG_LOG
			Serial.print("Location: ");
			Serial.print(gps->latitude, 4); Serial.print(gps->lat);
			Serial.print(", ");
			Serial.print(gps->longitude, 4); Serial.println(gps->lon);
			Serial.print("Satellites: "); Serial.println((int)gps->satellites);
		#endif
		}

		#if DEBUG_LOG
		Serial.print("Check new Position ...");
		#endif

		newPoint.updatePointDegree(gps->latitude, gps->longitude);

		if(useSdCard && logFile != NULL) {
			logSdCard(&newPoint);
		}

		if(!lastPoint.equalsGpsPoint(&newPoint) || simulateNewLap) {
			#if DEBUG_LOG
			Serial.print("NEW POSITION");
			#endif
			// Check new Lap
			GpsPoint* intersectionPoint = track->isFinishLinePassed(&lastPoint, &newPoint);
			if(intersectionPoint != NULL || simulateNewLap) {
				simulateNewLap = false;
				lapTimer.newLap();
				chronoGui.updateLapNumber(lapTimer.getLapNumber());

				// TODO manage BEST / LAST delay calculation
				chronoGui.updateLapDelay(lapTimer.getBestLapDelay());

				if(intersectionPoint != NULL) {
					delete intersectionPoint;
				}
			}
		}

		lastPoint.updatePointDecimal(newPoint.latitude, newPoint.longitude);
	}
	#if DEBUG_LOG
	Serial.print("Check GPS Fix ...");
	#endif
	if(gpsFixState != gps->fix) {
		chronoGui.updateGpsFixState(gps->fix);
		gpsFixState = gps->fix;
	}

	if(gpsStelliteNumber != gps->satellites) {
		gpsStelliteNumber = gps->satellites;
		chronoGui.updateGpsSatelliteNumber(gps->satellites);
	}

	#if DEBUG_LOG
		Serial.print("Update GUI");
	#endif
	chronoGui.updateLapTime(lapTimer.getCurrentLapTime());
}

// void Chrono::updateLapTimeGui() {
// 	chronoGui.updateLapTime(lapTimer.getCurrentLapTime());
// }

float knotsToKmH(float speed) {
	return speed * 1.852;
}

void Chrono::logSdCard(GpsPoint* intersectionPoint) {
  
	if (logFile->open("syslog.log", O_WRITE | O_CREAT | O_APPEND)) {

		logFile->print(gps->day);
		logFile->print("-");
		logFile->print(gps->month);
		logFile->print("-");
		logFile->print(gps->year);
		logFile->print(" ");

		logFile->print(gps->hour);
		logFile->print(":");
		logFile->print(gps->minute);
		logFile->print(":");
		logFile->print(gps->seconds);
		logFile->print(".");
		logFile->print(gps->milliseconds);
		logFile->print(";");

		//dataFile.print("NEW LAT: ");
		logFile->print(newPoint.latitude, 4);
		logFile->print(";");
		//dataFile.print("NEW LON: ");
		logFile->print(newPoint.longitude, 4);
		logFile->print(";");

		if(intersectionPoint != NULL) {
			logFile->print("1;");
			logFile->print(intersectionPoint->latitude, 4);
			logFile->print(";");
			logFile->print(intersectionPoint->longitude, 4);
			logFile->print(";");
		} else {
			logFile->print("0;;;");
		}
		logFile->println("");
		logFile->close();
	} else {
		Serial.println("Open file error");
	}
}
