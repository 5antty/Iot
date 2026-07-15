#include "utilities.h"

// ---------- Lectura de humedad de suelo local (sensor capacitivo) ----------
// Reemplazar por la calibración real de tu sensor (seco vs. sumergido)
float leerHumedadSueloLocal()
{
    int lecturaADC = analogRead(PIN_SENSOR); // ajustar pin según tu conexión
    // Mapeo simple: ajustar ADC_SECO y ADC_SATURADO según calibración
    const int ADC_SECO = 4095;
    const int ADC_SATURADO = 1000;
    float humedad = (float)(ADC_SECO - lecturaADC) / (ADC_SECO - ADC_SATURADO);
    printf("Lectura ADC: %d, Humedad calculada: %.2f\n", lecturaADC, humedad);
    return constrain(humedad, 0.0, 1.0);
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
