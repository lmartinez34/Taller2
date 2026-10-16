#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h> // Incluir la librería del DHT

// ======================================================
// CONFIGURACIÓN WIFI
// ======================================================

const char* WIFI_SSID = "ARTEFACTOS";
const char* WIFI_PASSWORD = "s^3sQ#9teskVgnb";

// ======================================================
// CONFIGURACIÓN MQTT
// ======================================================

const char* MQTT_SERVER = "10.110.0.2";
const int MQTT_PORT = 1883;

// Topics utilizados por el dashboard React.
const char* TOPIC_TEMPERATURA = "clima/esp32/temperatura";
const char* TOPIC_HUMEDAD = "clima/esp32/humedad";
const char* TOPIC_LUZ = "clima/esp32/luz";

// ======================================================
// CONFIGURACIÓN DE SENSORES
// ======================================================

// Definición de pines según especificaciones del laboratorio
#define DHTPIN 4       // DHT11 conectado al GPIO 4
#define DHTTYPE DHT11  // Definimos el tipo de sensor DHT
DHT dht(DHTPIN, DHTTYPE);

const int LDR_PIN = 36; // LDR conectada al divisor de voltaje en GPIO 32

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Enviar una lectura cada 2 segundos.
const unsigned long INTERVALO_ENVIO = 2000;
unsigned long ultimoEnvio = 0;

void conectarWiFi() {
  Serial.print("Conectando a WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");
  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando a Mosquitto...");

    // Crear un Client ID diferente para este ESP32.
    String clientId = "ESP32-";
    clientId += String(
      (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF),
      HEX
    );

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println(" conectado");
    } else {
      Serial.print(" error, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Reintentando en 2 segundos...");
      delay(2000);
    }
  }
}

// ======================================================
// LEER Y PUBLICAR SENSORES
// ======================================================

void publicarSensores() {
  // 1. Obtener la temperatura y 2. Humedad
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  // Validar si la lectura del DHT falló
  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error al leer del sensor DHT11!");
    return;
  }

  // 3. Obtener la lectura del sensor de iluminación (LDR)
  int valorLuz = analogRead(LDR_PIN);

  // 4 y 5. Preparar y publicar los valores mediante MQTT
  char bufferTemp[10];
  char bufferHumedad[10];
  char bufferLuz[10];

  // Convertir los valores numéricos a cadenas de texto (char array)
  dtostrf(temperatura, 4, 2, bufferTemp);
  dtostrf(humedad, 4, 2, bufferHumedad);
  sprintf(bufferLuz, "%d", valorLuz);

  mqttClient.publish(TOPIC_TEMPERATURA, bufferTemp);
  mqttClient.publish(TOPIC_HUMEDAD, bufferHumedad);
  mqttClient.publish(TOPIC_LUZ, bufferLuz);

  // 6. Mostrar las mediciones en el Monitor Serial
  Serial.println("--- Datos publicados ---");
  Serial.print("Temperatura: "); Serial.print(temperatura); Serial.println(" °C");
  Serial.print("Humedad:     "); Serial.print(humedad);     Serial.println(" %");
  Serial.print("Luz (LDR):   "); Serial.println(valorLuz);
}

void setup() {
  Serial.begin(115200);

  // Inicializar los sensores y configurar pines
  dht.begin();
  pinMode(LDR_PIN, INPUT);

  conectarWiFi();

  mqttClient.setServer(
    MQTT_SERVER,
    MQTT_PORT
  );
}

void loop() {
  // Si se pierde WiFi, volver a conectarse.
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // Si se pierde MQTT, volver a conectarse.
  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();

  // Publicar sensores cada 2 segundos (Corregido el operador de resta '-')
  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {
    ultimoEnvio = millis();
    publicarSensores();
  }
}