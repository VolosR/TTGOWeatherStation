#include <Arduino.h>
#include <esp_adc_cal.h>
#include "hal.hpp"

#define BATTERY_MIN_V 2.9
#define BATTERY_MAX_V 4.3
#define BATTCHARG_MIN_V 4.6
#define BATTCHARG_MAX_V 4.99

void setupBattADC();
void setupBattery();
float battGetVoltage();
uint8_t battCalcPercentage(float volts);
void battUpdateChargeStatus();
bool battIsCharging();

uint8_t _calcPercentage(float volts, float max, float min);
