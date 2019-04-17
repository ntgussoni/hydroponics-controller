/*
Class to manage each totem
SR04 Ultrasonic sensor
*/
#include "Arduino.h"

class Relay {
	int _relayPin;
	boolean _invert;

	public:
	Relay(int relayPin, boolean NO = true) {
		_relayPin = relayPin;
		_invert = NO;

		pinMode(_relayPin, OUTPUT);
		off();
	}

	void on() {
		if (_invert) {
			digitalWrite(_relayPin, LOW);
		} else {
			digitalWrite(_relayPin, HIGH);
		}
	}

	void off() {
		if (_invert) {
			digitalWrite(_relayPin, HIGH);
		} else {
			digitalWrite(_relayPin, LOW);
		}
	}
};
