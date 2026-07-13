// Librerías necesarias
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

#define nodo 0

// ─── Pines de Hardware ──────────────────────────────────────
#define PIN_HUMEDAD 34 // GPIO34 → datos del sensor de humedad capacitivo
#define PIN_VALVULA 2  // GPIO2 → control de la válvula (relevo o transistor)

// Conexión Broker MQTT
const char *mqtt_server = "192.168.0.175"; // Reemplaza con la IP del server MQTT local

// Objetos globales
WiFiClient espClient; // Cliente MQTT
PubSubClient client(espClient);

//
bool estadoValvula = false; // Estado de la válvula (apagada por defecto)

// Intervalo de envío de datos del sensor (milisegundos)
const unsigned long INTERVALO_SENSOR = 2000;
unsigned long ultimaLectura = 0; // Temporizador

// ─── Prototipos de funciones ───────────────────────────────
String construirJSON(float, bool);
void enviarAlServidorMQTT();
void wifiManagerSetup();
void callback(char *topic, byte *payload, unsigned int length);
void connectToMQTT();

void setup()
{
  Serial.begin(115200);

  // ── Pines de Hardware ─────────────────────────────────────
  // pinMode(PIN_HUMEDAD, INPUT);
  pinMode(PIN_VALVULA, OUTPUT);
  digitalWrite(PIN_VALVULA, LOW); // Apagar la válvula por defecto

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

void connectToMQTT()
{
  client.setServer(mqtt_server, 1883);
  char id_cli[50];
  sprintf(id_cli, "ESP32Client-nodo%d", nodo);
  client.connect(id_cli);

  char subtopic[50];
  sprintf(subtopic, "nodo%d/valvula", nodo);
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

// ---------- Lectura de humedad de suelo local (sensor capacitivo) ----------
// Reemplazar por la calibración real de tu sensor (seco vs. sumergido)
float leerHumedadSueloLocal()
{
  int lecturaADC = analogRead(PIN_HUMEDAD); // ajustar pin según tu conexión
  // Mapeo simple: ajustar ADC_SECO y ADC_SATURADO según calibración
  const int ADC_SECO = 4095;
  const int ADC_SATURADO = 1000;
  float humedad = (float)(ADC_SECO - lecturaADC) / (ADC_SECO - ADC_SATURADO);
  printf("Lectura ADC: %d, Humedad calculada: %.2f\n", lecturaADC, humedad);
  return constrain(humedad, 0.0, 1.0);
}

/**
 * Construye un JSON con todos los datos del sistema.
 * Ejemplo: {"temp":24.5,"hum":60.0,"bomba":true}
 */
String construirJSON(float hum, bool bomba)
{
  JsonDocument doc;
  doc["hum"] = isnan(hum) ? -1 : hum;
  doc["bomba"] = bomba;
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

  char topic[50];
  sprintf(topic, "nodo%d/datos", nodo);
  client.publish(topic, json.c_str());

  // Serial.println("Enviado a MQTT: " + json);
}

void wifiManagerSetup()
{
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
  sprintf(subtopic, "nodo%d/valvula", nodo);
  if (String(topic) == subtopic)
  {
    if (mensaje == "ON")
    {
      estadoValvula = true;
      digitalWrite(PIN_VALVULA, HIGH); // Encender algo
    }
    else if (mensaje == "OFF")
    {
      estadoValvula = false;
      digitalWrite(PIN_VALVULA, LOW); // Apagar algo
    }
  }
}