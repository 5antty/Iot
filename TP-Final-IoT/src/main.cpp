// Librerías necesarias

#include <mqtt_lib.h>
#include <utilities.h>
#include "headers.h"

// Objetos globales
WiFiClient espClient; // Cliente MQTT
PubSubClient client(espClient);

const char *mqtt_server = "192.168.0.205"; // Reemplaza con la IP del server MQTT local
//
bool estadoValvula = false; // Estado de la válvula (apagada por defecto)

// Intervalo de envío de datos del sensor (milisegundos)
const unsigned long INTERVALO_SENSOR = 2000;
unsigned long ultimaLectura = 0; // Temporizador

void setup()
{
  Serial.begin(115200);

  // ── Pines de Hardware ─────────────────────────────────────
  // pinMode(PIN_SENSOR, INPUT);
  pinMode(PIN_ACTUADOR, OUTPUT);
  digitalWrite(PIN_ACTUADOR, LOW); // Apagar la válvula por defecto

  // ── WiFiManager ────────────────────────────────────────────
  // Conectamos a WiFi usando WiFiManager (crea un AP de configuración si no se conecta)
  wifiManagerSetup();

  // ── MQTT ─────────────────────────────────────────────

  connectToMQTT();
}
void loop()
{
  if (!client.connected())
  {
    connectToMQTT();
  }
  client.loop(); // Es el encargado de llamar al callback si llegan datos
  // Envio datos del sensor cada INTERVALO_SENSOR milisegundos
  unsigned long ahora = millis();
  if (ahora - ultimaLectura >= INTERVALO_SENSOR)
  {
    ultimaLectura = ahora;
    enviarAlServidorMQTT();
  }
}