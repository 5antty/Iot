// Librerías necesarias
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
// Conexión Broker MQTT
const char *mqtt_server = "192.168.X.X";
// Cliente MQTT
WiFiClient espClient;
PubSubClient client(espClient);
void setup()
{
  Serial.begin(115200);

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
  float temp = 24.5; // Simulación
  char msg[50];
  sprintf(msg, "{\"temp\": %.2f}", temp);
  client.publish("sensor/ambiente", msg);
  delay(5000);
}