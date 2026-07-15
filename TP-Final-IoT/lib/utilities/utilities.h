#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "headers.h"

// ─── Prototipos de funciones ───────────────────────────────
void wifiManagerSetup();
float leerHumedadSueloLocal();

#endif