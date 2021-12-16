#include <Arduino.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <esp_system.h>
#include <time.h>

#include "configuration.h"
#include "keys.h"
#include "measure.h"

using namespace std;

#ifndef MAIN_h
#define MAIN_h

void connectWiFi();
void setupMQTT();
void connectMQTT();

void onMQTTMessage(char *topic, byte *payload, unsigned int length);

void setup();
void loop();

#endif