#ifndef MQTT_LIB_H
#define MQTT_LIB_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "utilities.h"
#include "headers.h"

// Conexión Broker MQTT
extern const char *mqtt_server;
extern PubSubClient client;
extern bool estadoValvula; // Estado de la válvula (apagada por defecto)

void connectToMQTT();
String construirJSON(float, bool);
void enviarAlServidorMQTT();
void callback(char *topic, byte *payload, unsigned int length);

#endif
