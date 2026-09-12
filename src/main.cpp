#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>

#define DHT_PINO 14
#define DHT_TYPE DHT22
DHT dht(DHT_PINO, DHT_TYPE);

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* api_url = "seu endpoint da dashboard";

const int PIR_PIN = 4;
const int TRIG_PIN = 27;
const int ECHO_PIN = 18;
const int SOUND_PIN = 34;
const int LED_PIN = 32;

unsigned long tempoUltimoSystem = 0;
unsigned long tempoUltimoSom = 0;
unsigned long tempoUltimoUltra = 0;
unsigned long tempoUltimoDHT = 0;

void enviarEstado(String device_id, String sensor, bool estado, float value) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    if (http.begin(client, api_url)) {
      http.addHeader("Content-Type", "application/json");

      String estadoStr = estado ? "true" : "false";
      String json = "{\"device_id\":\"" + device_id +
                    "\",\"sensor\":\"" + sensor +
                    "\",\"estado\":" + estadoStr +
                    ",\"value\":" + String(value, 2) +
                    ",\"timestamp\":\"2026-09-11T20:00:00.000Z\"}";

      int codigo = http.POST(json);

      if (codigo == 200 || codigo == 201) {
        Serial.printf("[HTTP %d] %s -> Enviado com sucesso!\n", codigo, sensor.c_str());
      } else {
        Serial.printf("[HTTP ERRO %d] Falha ao enviar %s\n", codigo, sensor.c_str());
      }
      http.end();
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=========================================");
  Serial.println("     INICIANDO SIMULADOR ESP32 WOKWI    ");
  Serial.println("=========================================");

  pinMode(PIR_PIN, INPUT);
  pinMode(DHT_PINO, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(SOUND_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  
  dht.begin();
  WiFi.begin(ssid, password);
  
  Serial.print("Conectando ao WiFi");
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Conectado com sucesso!");
  } else {
    Serial.println("\n[WIFI] Operando offline...");
  }
}

void loop() {
  unsigned long tempoAtual = millis();

  // 1. Métricas do Sistema (A cada 10s)
  if (tempoAtual - tempoUltimoSystem >= 10000) {
    tempoUltimoSystem = tempoAtual;
    enviarEstado("ESP32_WiFi_01", "WiFi", true, (float)WiFi.RSSI());
    enviarEstado("ESP32_UPTIME_01", "UPTIME", true, (float)(millis() / 1000));
    enviarEstado("ESP32_memori_01", "memori", true, (float)ESP.getFreeHeap());
  }

  // 2. Sensor PIR
  int estadoPIR = digitalRead(PIR_PIN);
  static int ultimoEstadoPIR = LOW;
  if (estadoPIR != ultimoEstadoPIR) {
    bool detectou = (estadoPIR == HIGH);
    digitalWrite(LED_PIN, detectou ? HIGH : LOW);
    enviarEstado("ESP32_PIR_01", "PIR", detectou, detectou ? 1.0 : 0.0);
    ultimoEstadoPIR = estadoPIR;
  }

  // 3. Sensor de Som (A cada 3s)
  if (tempoAtual - tempoUltimoSom >= 3000) {
    tempoUltimoSom = tempoAtual;
    enviarEstado("ESP32_SOUND_01", "SOUND", true, (float)analogRead(SOUND_PIN));
  }

  // 4. Ultrassônico (A cada 5s)
  if (tempoAtual - tempoUltimoUltra >= 5000) {
    tempoUltimoUltra = tempoAtual;
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duracao = pulseIn(ECHO_PIN, HIGH);
    float distancia = duracao * 0.034 / 2.0;
    enviarEstado("ESP32_ULTRASONIC_01", "ULTRASONIC", true, distancia);
  }

  // 5. Sensor DHT22 (A cada 10s)
  if (tempoAtual - tempoUltimoDHT >= 10000) {
    tempoUltimoDHT = tempoAtual;
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t)) enviarEstado("ESP32_TEMP_01", "DHT22", true, t);
    if (!isnan(h)) enviarEstado("ESP32_HUMID_01", "HUMIDITY", true, h);
  }

  delay(20);
}