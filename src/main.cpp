#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Pin lampu
const int merahPin  = 25;
const int kuningPin = 26;
const int hijauPin  = 27;

// WiFi
const char* ssid = "Enigmanians";
const char* password = "Growwithenigma2024!";

// MQTT broker
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

String currentStatus = "Merah";
bool isRunning = true;

// Fungsi update status ke broker
void setStatus(String status) {
  currentStatus = status;
  client.publish("trafficlight/status", status.c_str());
}

// Callback MQTT untuk kontrol ON/OFF
void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)message[i];

  Serial.print("Pesan diterima [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(msg);

  if (String(topic) == "trafficlight/control") {
    if (msg == "ON") {
      isRunning = true;
    } else if (msg == "OFF") {
      isRunning = false;
      // langsung matikan lampu saat OFF
      digitalWrite(merahPin, LOW);
      digitalWrite(kuningPin, LOW);
      digitalWrite(hijauPin, LOW);
      setStatus("Mati");
    }
  }
}

// Reconnect MQTT jika disconnected
void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");

    // Set Last Will: kalau ESP mati mendadak → broker publish "OFFLINE"
    if (client.connect("ESP32TrafficLight",
                       NULL, NULL,
                       "trafficlight/status", 0, true, "OFFLINE")) {
      Serial.println("Terhubung!");

      // Publish status ONLINE setelah connect
      client.publish("trafficlight/status", "ONLINE", true);

      // Subscribe topik kontrol
      client.subscribe("trafficlight/control");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(" coba lagi 5 detik");
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);

  pinMode(merahPin, OUTPUT);
  pinMode(kuningPin, OUTPUT);
  pinMode(hijauPin, OUTPUT);

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi terhubung!");
  Serial.println(WiFi.localIP());

  // MQTT setup
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void delayWithCheck(int totalMillis) {
  int interval = 100; // 100ms
  int loops = totalMillis / interval;
  for (int i = 0; i < loops; i++) {
    delay(interval);
    client.loop();          // proses MQTT agar OFF bisa diterima
    if (!isRunning) return; // keluar langsung saat OFF
  }
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  if (!isRunning) {
    delay(100);
    return;
  }

  // ===== Fase Merah =====
  digitalWrite(merahPin, HIGH);
  digitalWrite(kuningPin, LOW);
  digitalWrite(hijauPin, LOW);
  setStatus("Merah");
  delayWithCheck(5000);

  if (!isRunning) return;

  // ===== Fase Merah+Kuning =====
  digitalWrite(merahPin, HIGH);
  digitalWrite(kuningPin, HIGH);
  digitalWrite(hijauPin, LOW);
  setStatus("Merah + Kuning");
  delayWithCheck(1500);

  if (!isRunning) return;

  // ===== Fase Kuning =====
  digitalWrite(merahPin, LOW);
  digitalWrite(kuningPin, HIGH);
  digitalWrite(hijauPin, LOW);
  setStatus("Kuning");
  delayWithCheck(500);

  if (!isRunning) return;

  // ===== Fase Hijau =====
  digitalWrite(merahPin, LOW);
  digitalWrite(kuningPin, LOW);
  digitalWrite(hijauPin, HIGH);
  setStatus("Hijau");
  delayWithCheck(5000);

  if (!isRunning) return;

  // ===== Fase Kuning Akhir =====
  digitalWrite(merahPin, LOW);
  digitalWrite(kuningPin, HIGH);
  digitalWrite(hijauPin, LOW);
  setStatus("Kuning");
  delayWithCheck(2000);
}
