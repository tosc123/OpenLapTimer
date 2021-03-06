#ifndef GPSPOINT_H_
#define GPSPOINT_H_

#include <math.h>

class GpsPoint {
public:
	GpsPoint();
	GpsPoint(float latitude, float longitude);
	virtual ~GpsPoint();
	bool equalsGpsPoint(GpsPoint* point);
	void updatePointDecimal(float latitude, float longitude);
	void updatePointDegree(float latitude, float longitude);

	float latitude;
	float longitude;
private:
	double convertDegMinToDecDeg(float deg);
};

#endif /* GPSPOINT_H_ */