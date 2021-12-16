#include "measure.h"

int ThermistorPin;
float adcMax, Vs;

double R1 = 10000.0; // voltage divider for 10kOhm resistor
double Beta = 3950.0;
double To = 298.15;  // Temperature in Kelvin for 25C
double Ro = 10000.0; // Thermistor resistance at 25C

double measure()
{
  ThermistorPin = 36;
  adcMax = 4095.0; // ADC resolution 12-bit (0-4095)
  Vs = 3.3;        // supply voltage

  double Vout, Rt = 0;
  double T, Tc = 0;

  double adc = 0;
  adc = analogRead(ThermistorPin); // Read thermistor value
  adc = ADC_LUT[(int)adc];         // Use lookup table for more precision

  // Math for temperature conversion

  Vout = adc * Vs / adcMax;
  Rt = R1 * Vout / (Vs - Vout);

  T = 1 / (1 / To + log(Rt / Ro) / Beta); // Kelvin
  Tc = T - 273.15;                        // Celsius

  return Tc;
}