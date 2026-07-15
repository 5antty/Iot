#include "mqtt_lib.h"

void connectToMQTT()
{
    client.setServer(mqtt_server, 1883);
    char id_cli[50];
    sprintf(id_cli, "ESP32Client-nodo%d", nodo);
    client.connect(id_cli);

    char subtopic[50];
    sprintf(subtopic, "actuadores/nodo%d", nodo);
    Serial.print("Suscribiéndose al topic: ");
    Serial.println(subtopic);

    // Suscribirse al topic de control de la válvula
    if (!(client.subscribe(subtopic)))
    {
        Serial.println("Error al suscribirse al topic MQTT");
    }
    else
    {
        Serial.print("Suscrito al topic: ");
        Serial.println(subtopic);
    }

    client.setCallback(callback);
}

/**
 * Construye un JSON con el formato esperado por el flujo de Node-RED:
 * { "tipo": "...", "value": ..., "dispositivo": "...", "id": ... }
 */
String construirJSON(float hum, bool bomba)
{
    JsonDocument doc;
    doc["tipo"] = TIPO_SENSOR;
    doc["value"] = isnan(hum) ? -1 : hum;
    doc["dispositivo"] = bomba ? "ON" : "OFF";
    doc["id"] = nodo;

    String json;
    serializeJson(doc, json);
    return json;
}

/**
 * Envía el estado actual (sensor dht11) al servidor MQTT.
 */
void enviarAlServidorMQTT()
{
    float hum = leerHumedadSueloLocal();
    String json = construirJSON(hum, estadoValvula);

    // char topic[50];
    // sprintf(topic, "redsensores", nodo);
    client.publish("redsensores", json.c_str());

    // Serial.println("Enviado a MQTT: " + json);
}

void callback(char *topic, byte *payload, unsigned int length)
{
    // 1. Crear un string temporal para manejar los datos recibidos
    String mensaje = "";
    for (int i = 0; i < length; i++)
    {
        mensaje += (char)payload[i];
    }

    // 2. Imprimir los datos en el monitor serie
    Serial.print("Mensaje recibido en el topic: ");
    Serial.println(topic);
    Serial.print("Contenido: ");
    Serial.println(mensaje);

    // 3. Puedes tomar acciones según el topic o el mensaje
    char subtopic[50];
    sprintf(subtopic, "actuadores/nodo%d", nodo);
    if (String(topic) == subtopic)
    {
        if (mensaje == "ON")
        {
            estadoValvula = true;
            digitalWrite(PIN_ACTUADOR, HIGH); // Encender algo
        }
        else if (mensaje == "OFF")
        {
            estadoValvula = false;
            digitalWrite(PIN_ACTUADOR, LOW); // Apagar algo
        }
    }
}