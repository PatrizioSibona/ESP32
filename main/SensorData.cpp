/*
 * SensorData.cpp
 *
 *  Created on: 23 mag 2018
 *      Author: Patrizio
 */

#include "SensorData.h"
#include <string>
#include <iostream>


using namespace std;

SensorData::SensorData(int c, int rssi, struct timeval t, string s, string seq_ctrl):
			channel(c), RSSI(rssi), time(t), source(s), sequence_ctrl(seq_ctrl){

}

SensorData::~SensorData() {

}

void SensorData::printData(){
	cout << "PACKET TYPE=PROBE CHAN=" << channel << " RSSI=" << RSSI
		 << " ADDR=" << source << " SEQ=" << sequence_ctrl
		 << " Time_sec=" << time.tv_sec << " Time_usec=" << time.tv_usec << "\n";
	return;
}

