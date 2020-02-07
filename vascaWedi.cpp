/*
 * TODO.
 * Check the timers for safety open/close of the valve on the overflow.
 * Approximatively we cover 1.6 liters every millimeter of height.Le't assume 16 liters make 1cm of water.
 * We should so consider a meaningful raising a value in the neighbor of 15mm raise, hence 26 liters entered in the bathtub
 * Assuming a loading flow of x l/s we can calculate the stabilization time in:
 * 	26 / loadFlow
 * as an example, if the load flow is 11 l/min (e.g. 0,183 l/s) we should reach the 15mm raise in roughly 140s
 *
 * during tests we might use a pipe having a 90mm diameter and a loadflow of 12 l/m (e.g. 0,2 l/m).
 * The pipe surface is roughly 25000mmq, each millimeter represents roughly 25ml of water. We get to the 1.5mm raise in roughly 10s
 *
 * To be noticed that we should always increase the stabilization time when we empty to allow a longer time between the 2 events
 * we multiply it by a 1.5 factor (e.g. 15mm in raise 22.5mm decrease)
 */


#include "vascaWedi.h"
#define TMR_VALVE_OPEN				15000
#define TMR_VALVE_CLOSE				10000
//#define TMR_STABILIZE_ALERT			40000	// use for real situation
//#define TMR_STABILIZE_OVERFLOW		140000	// use for real situation
#define TMR_STABILIZE_ALERT			3500		// use for test with pipe 90mm diam
#define TMR_STABILIZE_OVERFLOW		10000		// use for test with pipe 90mm diam

#define TMR_STAB_DECREASE_FACTOR	1.5

#define	TEST_RUNS	false
#define SENSOR_CLOSE false // false for test, set to true in production

typedef struct {
	int pin;
	int currentVal;
	int previousVal;
	boolean front;
} inPin;

// Input
inPin button;
inPin waterHigh;
inPin waterOverflow;

// Output
const int pinLedSensorHigh = 5;
const int pinLedSensorOverflow = 6;
const int pinLedValve = 7;
const int pinDrainRele = 9;

// variables will change:
int inputStatePrevious;

int buttonState = 0;         // variable for reading the pushbutton status
boolean waterSensAlert = 0;         // variable for reading the water High
boolean waterSensAlarm = 0;         // variable for reading the water high-high
int drainReleState = 0;
int ledWaterAlert = 0;
int ledWaterOverflow = 0;
int ledValveClose = 0;
int ledStatus = LOW;

long ledStatusBlinkTime;
long drainReopenTimer = 0;
long timerStablizeAlert = 0;
long timerStablizeOverflow = 0;
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
	waterOverflow.pin = 12;
	waterHigh.front = false;
	waterOverflow.previousVal = LOW;

	// initialize the output pins
	pinMode(pinDrainRele, OUTPUT);
	pinMode(pinLedValve, OUTPUT);
	pinMode(pinLedSensorHigh, OUTPUT);
	pinMode(pinLedSensorOverflow, OUTPUT);
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, ledStatus);

	// initialize the input pins:
	pinMode(button.pin, INPUT);
	pinMode(waterHigh.pin, INPUT);
	pinMode(waterOverflow.pin, INPUT);

	start = ledStatusBlinkTime = millis();
}

int digitalReadOutputPin(uint8_t pin)
{
 uint8_t bit = digitalPinToBitMask(pin);
 uint8_t port = digitalPinToPort(pin);
 if (port == NOT_A_PIN)
   return LOW;

 return (*portOutputRegister(port) & bit) ? HIGH : LOW;
}

void blink()
{
	if (now - ledStatusBlinkTime > 1000)
	{
		if (ledStatus == HIGH)
			ledStatus = LOW;
		else
			ledStatus = HIGH;
		ledStatusBlinkTime = now;
	}

}


void loop()
{
	now = millis();

	// read the state of the input pin and set front:
	readInput(&button);
	readInput(&waterHigh);
	readInput(&waterOverflow);

	// The request to open/close the valve follows the status of the switch

	drainReleState = button.currentVal;
	if (button.front)
	{
		Serial.println("Switch changed status");
		timerValveMove = now;
	}
	else if ((timerValveMove != 0) && (now - timerValveMove> TMR_VALVE_CLOSE))
	{
		ledValveClose = button.currentVal ;
		timerValveMove = 0;
	}

	// Check the WaterAlert sensor and set led accordingly
	ledWaterAlert = digitalReadOutputPin(pinLedSensorHigh);
	if (waterHigh.front)
	{
		Serial.println("Water sensor changed status");
		timerStablizeAlert = now;
	}
	else if ((timerStablizeAlert != 0) && (now - timerStablizeAlert > TMR_STABILIZE_ALERT))
	{
		timerStablizeAlert = 0;
		if (waterHigh.currentVal == SENSOR_CLOSE)
			ledWaterAlert = HIGH;
		else
			ledWaterAlert = LOW; // the led goes off
	}

	int stabileOVerflowTime = (waterOverflow.currentVal == SENSOR_CLOSE ?
									TMR_STABILIZE_OVERFLOW :
									TMR_STABILIZE_OVERFLOW * TMR_STAB_DECREASE_FACTOR);

	ledWaterOverflow = digitalReadOutputPin(pinLedSensorOverflow);
	if (waterOverflow.front)
	{
		Serial.println("Overflow sensor changed status");
		timerStablizeOverflow = now;
	}
	else if ((timerStablizeOverflow  != 0) &&
			 (now - timerStablizeOverflow > stabileOVerflowTime))
	{
		timerStablizeOverflow = 0;
		if (waterOverflow.currentVal == SENSOR_CLOSE)
		{
			// Same logic as the High level, forcing the valve opening too
			drainReleState = LOW;
			ledWaterOverflow = HIGH;
		}
		else
		{
			ledWaterOverflow = LOW;
		}
	}

	// check if the pushbutton is pressed. If it is, the buttonState is HIGH:
	digitalWrite(pinDrainRele, drainReleState);
	digitalWrite(pinLedSensorHigh, ledWaterAlert);
	digitalWrite(pinLedSensorOverflow, ledWaterOverflow);
	digitalWrite(pinLedValve, ledValveClose);
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
