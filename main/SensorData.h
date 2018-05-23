/*
 * SensorData.h
 *
 *  Created on: 23 mag 2018
 *      Author: Patrizio
 */

#ifndef MAIN_SENSORDATA_H_
#define MAIN_SENSORDATA_H_

#include <sys/time.h>
#include <string>

using namespace std;

class SensorData {
private:
	int channel ;
	int RSSI;
	struct timeval time;
	string source;
	string sequence_ctrl;

public:
	SensorData(int c, int rssi, struct timeval t, string s, string seq_ctrl);
	virtual ~SensorData();
	void printData();
};

#endif /* MAIN_SENSORDATA_H_ */
