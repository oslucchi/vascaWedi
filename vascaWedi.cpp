#include "vascaWedi.h"
#define TMR_VALVE_OPEN	15000
#define TMR_VALVE_CLOSE	10000
#define	TEST_RUNS	false
#define SENSOR_CLOSE true // false for test, set to true in production

typedef struct {
	int pin;
	int currentVal;
	int previousVal;
	boolean front;
} inPin;

// Input
inPin button;
inPin waterHigh;
inPin waterHighHigh;

boolean buttonPinFront = false;
const int waterSensHigh = 10;
boolean waterSensHighFront = false;
const int waterSensHighHigh = 12;
boolean waterSensHighHighFront = false;

// Output
const int led1Pin = 5;
const int led2Pin = 6;
const int led3Pin = 7;
const int drainRelePin = 9;

// variables will change:
int inputStatePrevious;

int buttonState = 0;         // variable for reading the pushbutton status
boolean waterSensAlert = 0;         // variable for reading the water High
boolean waterSensAlarm = 0;         // variable for reading the water high-high
int drainReleState = 0;
int ledWaterAlert = 0;
int ledWaterAlarm = 0;
int ledValveClose = 0;
int ledStatus = LOW;
long ledStatusBlinkTime;
long drainReopenTimer = 0;
long timerStablize = 0;
long timerValveMove = 0;


long countRuns = 0;
long start = 0;
long now = 0;


void readInput(inPin* pin)
{
	pin->currentVal = digitalRead(pin->pin);
	pin->front = false;
	if (pin->currentVal != pin->previousVal)
	{
		pin->front = true;
	}
	pin->previousVal = pin->currentVal;
}


void setup() {
	Serial.begin(115200);

	button.pin = 8;
	button.front = false;
	button.previousVal = LOW;
	waterHigh.pin = 10;
	waterHigh.front = false;
	waterHigh.previousVal = LOW;
	waterHighHigh.pin = 12;
	waterHigh.front = false;
	waterHighHigh.previousVal = LOW;

	// initialize the output pins
	pinMode(drainRelePin, OUTPUT);
	pinMode(led1Pin, OUTPUT);
	pinMode(led2Pin, OUTPUT);
	pinMode(led3Pin, OUTPUT);
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, ledStatus);

	// initialize the input pins:
	pinMode(button.pin, INPUT);
	pinMode(waterHigh.pin, INPUT);
	pinMode(waterHighHigh.pin, INPUT);

	start = ledStatusBlinkTime = millis();
}

void loop()
{
	now = millis();
	if (now - ledStatusBlinkTime > 1000)
	{
		if (ledStatus == HIGH)
			ledStatus = LOW;
		else
			ledStatus = HIGH;
		ledStatusBlinkTime = now;
	}

	// read the state of the input pin and set front:

	readInput(&button);
	readInput(&waterHigh);
	readInput(&waterHighHigh);

	if (button.currentVal == HIGH)
	{
		drainReleState = HIGH;

		// Valve required to close. Check the status or the overflow sensors
		if (waterHigh.currentVal == SENSOR_CLOSE)
		{
			if (waterHigh.front)
			{
				Serial.println("First water sensor ON");
			}
			// the water reached the first sensor. Report it via led. Do not change the drainValve status, still openend
			ledWaterAlert = HIGH;
		}
		else
		{
			if (waterHigh.front)
			{
				Serial.println("First water sensor OFF");
				ledWaterAlert = HIGH; // the led goes off
			}
		}

		if (waterHighHigh.currentVal == SENSOR_CLOSE)
		{
			// The overflow level is reached. Wait for the stabilization time to open the valve
			if (waterHighHigh.front)
			{
				timerStablize = now;
			}
			else if (now - timerStablize > 3000)
			{
				// the level has been high for the stabilization time, open the valve
				if (drainReleState == HIGH)
				{
					ledWaterAlarm = HIGH;
					drainReleState = LOW;
					drainReopenTimer = 0;
					Serial.println("Second water sensor ON. Opening the valve ");
				}
			}
		}
		else
		{
			timerStablize = 0;
			if (waterHighHigh.front)
			{
				drainReopenTimer = now;
				Serial.print("Second water sensor OFF ");
			}
			if ((drainReopenTimer != 0) && (now - drainReopenTimer > 10000))
			{
				Serial.print("Timer expired, closing the valve ");
				drainReleState = HIGH;
				drainReopenTimer = 0;
			}
		}

		if (button.front)
		{
			Serial.println("Switch ON, closing the valve");
			timerValveMove = now;
		}
		if ((timerValveMove != 0) && (now - timerValveMove> TMR_VALVE_CLOSE))
		{
			ledValveClose = HIGH;
			timerValveMove = 0;
		}
	}
	else
	{
		if (button.front)
		{
			Serial.println("Switch OFF, opening the valve");
			timerValveMove = now;
		}
		drainReleState = LOW;
		if (now - timerValveMove > TMR_VALVE_OPEN)
		{
			ledValveClose = LOW;
		}
	}

	// check if the pushbutton is pressed. If it is, the buttonState is HIGH:
	digitalWrite(drainRelePin, drainReleState);
	digitalWrite(led1Pin, ledWaterAlert);
	digitalWrite(led2Pin, ledWaterAlarm);
	digitalWrite(led3Pin, ledValveClose);
	digitalWrite(LED_BUILTIN, ledStatus);
	if (TEST_RUNS)
	{
		if (now - start > 30000)
		{
			Serial.print("# of runs: ");
			Serial.print(countRuns);
			Serial.print(" per second ");
			Serial.println(countRuns / 30000);
			start = now;
			countRuns = 0;
		}
		else
			countRuns++;
	}
}
