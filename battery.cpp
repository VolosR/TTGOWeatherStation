#include "battery.hpp"

int vref = 1100;
float curv = 0;

void setupBattADC() {
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize((adc_unit_t)ADC_UNIT_1, (adc_atten_t)ADC1_CHANNEL_6, (adc_bits_width_t)ADC_WIDTH_BIT_12, 1100, &adc_chars);
    //Check type of calibration value used to characterize ADC
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.printf("[BATT] ADC eFuse Vref:%u mV\n", adc_chars.vref);
        vref = adc_chars.vref;
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        Serial.printf("[BATT] ADC Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
    } else {
        Serial.printf("[BATT] ADC Default Vref: %u mV\n", vref);
    }
}

void setupBattery() {
    /*
    ADC_EN is the ADC detection enable port
    If the USB port is used for power supply, it is turned on by default.
    If it is powered by battery, it needs to be set to high level
    */
    pinMode(ADC_EN, OUTPUT);
    digitalWrite(ADC_EN, HIGH);
}

float battGetVoltage() {
    digitalWrite(ADC_EN, HIGH);
    delay(10);                         // suggested by @ygator user in issue #2
    uint16_t v = analogRead(ADC_PIN);  
    curv = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
    digitalWrite(ADC_EN, LOW);   // for possible issue: https://github.com/Xinyuan-LilyGO/TTGO-T-Display/issues/6
    // Serial.printf("[BATT] voltage: %f volts\n", curv);
    return curv;
}

uint8_t _calcPercentage(float volts, float max, float min) {
    float percentage = (volts - min) * 100 / (max - min);
    if (percentage > 100) {
        percentage = 100;
    }
    if (percentage < 0) {
        percentage = 0;
    }
    return (uint8_t)percentage;
}

uint8_t battCalcPercentage(float volts) {
    if (battIsCharging()){
      return _calcPercentage(volts,BATTCHARG_MAX_V,BATTCHARG_MIN_V);
    } else {
      return _calcPercentage(volts,BATTERY_MAX_V,BATTERY_MIN_V);
    }
}

void battUpdateChargeStatus() {
    // digitalWrite(LED_PIN, battIsCharging());
}

bool battIsCharging() {
    return curv > BATTERY_MAX_V;
}