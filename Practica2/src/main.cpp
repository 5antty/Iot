// Librerías necesarias
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <DHTesp.h>
#include <ArduinoJson.h>

// ─── Pines de Hardware ──────────────────────────────────────
#define DHT_PIN 17 // GPIO17 → datos del sensor DHT11

// Conexión Broker MQTT
const char *mqtt_server = "192.168.X.X"; // Reemplaza con la IP del server MQTT local

// Objetos globales
DHTesp dht;
WiFiClient espClient; // Cliente MQTT
PubSubClient client(espClient);

// Intervalo de envío de datos del sensor (milisegundos)
const unsigned long INTERVALO_SENSOR = 2000;
unsigned long ultimaLectura = 0; // Temporizador

// ─── Prototipos de funciones ───────────────────────────────
String construirJSON(float temp, float hum);
void enviarAlServidorMQTT();

void setup()
{
  Serial.begin(115200);

  // Inicializo el dht11
  dht.setup(DHT_PIN, DHTesp::DHT11);

  // ── WiFiManager ────────────────────────────────────────────

  // Conectamos a WiFi usando WiFiManager (crea un AP de configuración si no se conecta)
  WiFi.mode(WIFI_STA); // Modo estación (cliente)
  WiFiManager wifiManager;
  // Hago esto para que no haya conflicto entre el portal de configuración de WiFiManager y el servidor web que vamos a crear
  wifiManager.setSaveConfigCallback([]()
                                    {
    Serial.println("Nueva red guardada, reiniciando...");
    delay(1000);
    ESP.restart(); });

  bool res = wifiManager.autoConnect("ESP32-Config", "admin51423"); // Crea un AP con este nombre y contraseña si no se conecta
  if (!res)
  {
    Serial.println("Error: No se pudo conectar al WiFi");
    return;
  }

  // ── MQTT ─────────────────────────────────────────────
  client.setServer(mqtt_server, 1883);
  client.connect("ESP32Client");
}
void loop()
{
  // Envio datos del sensor cada INTERVALO_SENSOR milisegundos
  unsigned long ahora = millis();
  if (ahora - ultimaLectura >= INTERVALO_SENSOR)
  {
    ultimaLectura = ahora;
    enviarAlServidorMQTT();
  }
}

/**
 * Construye un JSON con todos los datos del sistema.
 * Ejemplo: {"temp":24.5,"hum":60.0}
 */
String construirJSON(float temp, float hum)
{
  JsonDocument doc;
  doc["temp"] = isnan(temp) ? -1 : temp; // -1 si el sensor falló
  doc["hum"] = isnan(hum) ? -1 : hum;

  String json;
  serializeJson(doc, json);
  return json;
}

/**
 * Envía el estado actual (sensor dht11) al servidor MQTT.
 */
void enviarAlServidorMQTT()
{
  float temp = dht.getTemperature();
  float hum = dht.getHumidity();
  String json = construirJSON(temp, hum);
  client.publish("sensores/dht11", json.c_str());
  Serial.println("Enviado a MQTT: " + json);
}