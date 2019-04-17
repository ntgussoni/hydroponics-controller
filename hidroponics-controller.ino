#define WLAN_SSID       "Nico y Lu"
#define WLAN_PASS       "queteimporta"

#define MQTT_SERVER      "192.168.1.11"
#define MQTT_PORT  		1883                   // use 8883 for SSL
#define MQTT_USERNAME    ""
#define MQTT_KEY         ""

#define TIMEZONE  		-3
#define DST				0

#define START_PUMP_INTERVAL  50 * 60
#define PUMP_DURATION  10 * 60

#include <ESP8266WiFi.h>
#include <time.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ArduinoJson.h>


// #include <brzo_i2c.h>
// #include "SSD1306Brzo.h"
// #include "OLEDDisplayUi.h"

#include "Libraries/Relay/Relay.cpp"

bool _putToSleep = false;
bool _connectToMQTT = true;
int _state = 0;
int _stateLight = 0;

time_t _previousActionTime = 0;
time_t _previousActionTimeLight = 0;

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, MQTT_USERNAME, MQTT_KEY);
Adafruit_MQTT_Publish hydroponics = Adafruit_MQTT_Publish(&mqtt, "hydroponics");
Adafruit_MQTT_Subscribe onOff = Adafruit_MQTT_Subscribe(&mqtt, "hydroponics/onoff");


Relay Pump(D5);
// Relay Air(D6);
Relay Light(D6);

// SSD1306Brzo display(0x3C, D1, D2);
// OLEDDisplayUi   ui( &display );

// void onOffCallback(const char * command) {
// 	Serial.println("COMMAND WAS ");
// 	Serial.print(command);
// 	if (strcmp(command, "connected") == 0) {
// 		_putToSleep = false;
// 	} else if(strcmp(command, "disconnected") == 0) {
// 		_putToSleep = true;
// 		Pump.off();
// 		Light.off();
// 		_state = 0;
// 		_stateLight = 0;
// 		_previousActionTime = 0;
// 	}
// }

void setup() {
	Serial.begin(115200);
	WIFI_connect();
	TIME_connect();
	MQTT_connect();

  	// onOff.setCallback(onOffCallback);
	// mqtt.subscribe(&onOff);
	publishStatus(hydroponics, "connected");
}

void loop() {
	MQTT_connect();
	time_t now = time(nullptr);
	mqtt.processPackets(1000);

	// if (_putToSleep) {
	// 	delay(1000); // For the time
	// 	return;
	// }

	// For the pump
	if (_state == 0 && ((now - _previousActionTime) > START_PUMP_INTERVAL)) {
		_previousActionTime = now;
		_state = 1;
		Pump.on();
		publishStatus(hydroponics, "pump_start");
	} else if(_state == 1 && ((now - _previousActionTime) > PUMP_DURATION)){
		_previousActionTime = now;
		_state = 0;
		Pump.off();
		publishStatus(hydroponics, "pump_stop");
	}


	bool dateIsValid = isBetweenDates(now, 7,00, 19, 00);

	// For the light

	Serial.print("stateLight");
	Serial.println(_stateLight);

	if (_stateLight == 0 && dateIsValid) {
		_stateLight = 1;
		Light.on();
		publishStatus(hydroponics, "light_on");
	} else if (_stateLight == 1 && !dateIsValid) {
		_stateLight = 0;
		Light.off();
		publishStatus(hydroponics, "light_off");
	}


	delay(1000); // For the time
}

bool isBetweenDates(time_t now, byte startHour, byte startMinute, byte endHour, byte endMinute){
	struct tm *now_tr = localtime(&now);
	boolean isBetween = (((now_tr->tm_hour == startHour && now_tr->tm_min >= startMinute) || (now_tr->tm_hour > startHour))
   						 && ((now_tr->tm_hour == endHour && now_tr->tm_min  < endMinute) || (now_tr->tm_hour < endHour)));

	Serial.println(now_tr->tm_hour);
	Serial.println(now_tr->tm_min);

	Serial.println(startHour);
	Serial.println(startMinute);

	Serial.println(endHour);
	Serial.println(endMinute);

	Serial.println(isBetween);
	return isBetween;
}

void WIFI_connect() {
	// Connect to WiFi access point.
	Serial.println(); Serial.println();
	Serial.print("Connecting to ");
	Serial.println(WLAN_SSID);

	WiFi.begin(WLAN_SSID, WLAN_PASS);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println();

	Serial.println("WiFi connected");
	Serial.println("IP address: "); Serial.println(WiFi.localIP());
}

void TIME_connect() {
	configTime(TIMEZONE * 3600, DST, "pool.ntp.org", "time.nist.gov");

	Serial.println("\nWaiting for time");
	while (!time(nullptr) || time(nullptr) <= 1500000000 ) {
		Serial.print(".");
		delay(1000);
	}
	Serial.println(" Time Acquired");
	time_t now = time(nullptr);
	Serial.print(ctime(&now));
}

void MQTT_connect() {
	  // Ensure the connection to the MQTT server is alive (this will make the first
	// connection and automatically reconnect when disconnected).  See the MQTT_connect
	// function definition further below.

	// Function to connect and reconnect as necessary to the MQTT server.
	// Should be called in the loop function and it will take care if connecting.
	int8_t ret;
	// Stop if already connected.
	if (mqtt.connected() || !_connectToMQTT) {
		return;
	}

	Serial.print("Connecting to MQTT... ");

	uint8_t retries = 3;
	while ((ret = mqtt.connect()) != 0 && (retries > 0)) { // connect will return 0 for connected
		Serial.println(mqtt.connectErrorString(ret));
		Serial.println("Retrying MQTT connection in 5 seconds...");
		mqtt.disconnect();
		retries--;
		if (retries == 0) {
			_connectToMQTT= false;
			Serial.println("Could not connect to MQTT! Will stop trying");
		}
		delay(5000);  // wait 5 seconds
	}
	Serial.println("MQTT Connected!");

}


bool publishStatus(Adafruit_MQTT_Publish channel, char* state_description) {

	if (!mqtt.connected()) {
		Serial.println("MQTT Server down, will not send message");
	}
	const size_t capacity = JSON_OBJECT_SIZE(3);
	DynamicJsonBuffer jsonBuffer(capacity);

	JsonObject& root = jsonBuffer.createObject();
	root["state"] = _state;
	root["state_description"] = state_description;
	root["time"] = _previousActionTime;

	char JSONmessageBuffer[100];
	root.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

	Serial.println("Sending message to MQTT topic..");
	Serial.println(JSONmessageBuffer);

	if (channel.publish(JSONmessageBuffer) == true) {
		Serial.println("Success sending message");
	} else {
		Serial.println("Error sending message");
	}
  	Serial.println("-------------");
}

