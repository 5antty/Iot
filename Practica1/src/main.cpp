#include <Arduino.h>
#include <WiFiManager.h>
#undef HTTP_GET // Evita conflicto con HTTP_GET de ESPAsyncWebServer
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHTesp.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// ─── Pines de Hardware ──────────────────────────────────────
#define DHT_PIN 17 // GPIO17 → datos del sensor DHT11
#define LED_PIN 2  // GPIO2 → salida digital (LED integrado)

// Intervalo de envío de datos del sensor (milisegundos)
const unsigned long INTERVALO_SENSOR = 2000;
unsigned long ultimaLectura = 0; // Temporizador

// Objetos globales
DHTesp dht;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ─── Estado de la Salida Digital ────────────────────────────
volatile bool ledState = false;

// ─── Prototipos de funciones ───────────────────────────────
void setupRoutes();
void onWsEvent(AsyncWebSocket *server,
               AsyncWebSocketClient *client,
               AwsEventType type,
               void *arg,
               uint8_t *data,
               size_t len);
String construirJSON(float temp, float hum, bool led);
void enviarEstadoATodos();
void enviarEstadoACliente(AsyncWebSocketClient *client);

void setup()
{
  Serial.begin(115200);

  // ── Configuración de Hardware ─────────────────────────────

  // Inicializo el pin del LED como salida y lo apago
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Inicializo el dht11
  dht.setup(DHT_PIN, DHTesp::DHT11);

  // ── LittleFS ─────────────────────────────────────────────

  // Incializo el filesystem donde se guardarán los archivos HTML, CSS y JS
  if (!LittleFS.begin(true))
  {
    Serial.println("Error al montar LittleFS");
    return;
  }

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

  // ── WebSocket ─────────────────────────────────────────────
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // ── Configurar rutas del servidor ──────────────────────────
  setupRoutes();

  // ── Iniciar servidor ────────────────────────────────────────
  server.begin();
  Serial.println("Servidor HTTP iniciado en puerto 80");
}

void loop()
{
  // Limpia clientes WebSocket desconectados (importante hacerlo periódicamente)
  ws.cleanupClients();

  // Enviamos datos del sensor cada INTERVALO_SENSOR milisegundos
  unsigned long ahora = millis();
  if (ahora - ultimaLectura >= INTERVALO_SENSOR)
  {
    ultimaLectura = ahora;

    // Solo enviamos si hay clientes conectados (ahorra recursos)
    if (ws.count() > 0)
    {
      enviarEstadoATodos();
    }
  }
}

void setupRoutes()
{
  // Ruta principal
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });

  // 3. Servir el resto de archivos (CSS, JS, imágenes) automáticamente
  server.serveStatic("/", LittleFS, "/");

  // Si la ruta no existe
  server.onNotFound([](AsyncWebServerRequest *req)
                    { req->send(404, "text/plain", "No encontrado"); });
}

// ─────────────────────────────────────────────
//  FUNCIONES AUXILIARES
// ─────────────────────────────────────────────

/**
 * Construye un JSON con todos los datos del sistema.
 * Ejemplo: {"temp":24.5,"hum":60.0,"led":true}
 */
String construirJSON(float temp, float hum, bool led)
{
  JsonDocument doc;
  doc["temp"] = isnan(temp) ? -1 : temp; // -1 si el sensor falló
  doc["hum"] = isnan(hum) ? -1 : hum;
  doc["led"] = led;

  String json;
  serializeJson(doc, json);
  return json;
}

/**
 * Envía el estado actual (sensor + LED) a TODOS los clientes conectados.
 */
void enviarEstadoATodos()
{
  float temp = dht.getTemperature();
  float hum = dht.getHumidity();
  String json = construirJSON(temp, hum, ledState);
  ws.textAll(json);
  Serial.println("→ Enviado a todos: " + json);
}

/**
 * Envía el estado actual solo a UN cliente específico.
 * Se usa cuando un cliente nuevo se conecta.
 */
void enviarEstadoACliente(AsyncWebSocketClient *client)
{
  float temp = dht.getTemperature();
  float hum = dht.getHumidity();
  String json = construirJSON(temp, hum, ledState);
  client->text(json);
  Serial.printf("→ Enviado al cliente #%u: %s\n", client->id(), json.c_str());
}

// ─────────────────────────────────────────────
//  MANEJADOR DE EVENTOS WEBSOCKET
// ─────────────────────────────────────────────
void onWsEvent(AsyncWebSocket *server,
               AsyncWebSocketClient *client,
               AwsEventType type,
               void *arg,
               uint8_t *data,
               size_t len)
{
  switch (type)
  {

  case WS_EVT_CONNECT:
    // Un navegador abrió la página
    Serial.printf("Cliente #%u conectado desde %s\n",
                  client->id(),
                  client->remoteIP().toString().c_str());
    // Le mandamos el estado actual inmediatamente
    enviarEstadoACliente(client);
    break;

  case WS_EVT_DISCONNECT:
    Serial.printf("Cliente #%u desconectado\n", client->id());
    break;

  case WS_EVT_DATA:
  {
    // El navegador nos mandó un mensaje de texto
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    {
      String mensaje = String((char *)data).substring(0, len);
      Serial.println("← Recibido: " + mensaje);

      // ── Parseamos el JSON del cliente ──
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, mensaje);

      if (!err)
      {
        // El cliente puede enviar: {"cmd":"toggle"} o {"cmd":"set","valor":true}
        String cmd = doc["cmd"].as<String>();

        if (cmd == "toggle")
        {
          ledState = !ledState;
        }
        else if (cmd == "set")
        {
          ledState = doc["valor"].as<bool>();
        }

        // Aplicamos el nuevo estado al pin
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);

        // Notificamos a TODOS los clientes (por si hay varias pestañas abiertas)
        enviarEstadoATodos();
      }
    }
    break;
  }

  case WS_EVT_ERROR:
    Serial.printf("⚠ Error en cliente #%u: %s\n",
                  client->id(), (char *)data);
    break;

  default:
    break;
  }
}