// board : ESP32 V3.3.8

#include <Arduino.h>
#include "Config.h"
#include "SensorData.h"
#include "espnow_comm.h"
#include <WiFi.h>
#include "WifiHelper.h"
#include "MqttGateway.h"

// ── Objets globaux ────────────────────────────────────────────────────────────
WiFiClient  wifiClient;
MqttGateway mqtt(wifiClient, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, CONFIG_MQTT_CLIENT);
uint32_t lastMesure = 0;

// ─────────────────────────────────────────────────────────────────────────────
// Callback réception MQTT
// ─────────────────────────────────────────────────────────────────────────────
void onMqttMessage(const String& topic, const String& payload) {
    Serial.printf("[APP] Message reçu — topic: %s | payload: %s\n",
                  topic.c_str(), payload.c_str());
 
    // Exemple de dispatch par topic
    if (topic == "hydro/cmd/pump") {
      bool on = (payload == "ON" || payload == "1");
      Serial.printf("[APP] Commande pompe : %s\n", on ? "MARCHE" : "ARRÊT");
      // digitalWrite(PIN_PUMP, on ? HIGH : LOW);
    }
    else if (topic == "hydro/cmd/light") {
      // …
    } else {
      Serial.printf("[APP] topic not found");
    }
}

// ---setup------------------------------------------------
void setup() {
  // setup serial
  Serial.begin(CONFIG_BAUDRATE);
  delay(1000);

  // Setup start
  Serial.println("\nsetup: start");

  // Wifi connection to routeur
  connectToWiFi(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);

  // MQTT connection
  mqtt.setCredentials(CONFIG_MQTT_USER, CONFIG_MQTT_PASS);
  mqtt.setLastWill(CONFIG_TOPIC_STATUS, "offline", /*retain=*/true, /*qos=*/1);
  mqtt.setMessageCallback(onMqttMessage);
  if (!mqtt.connect(5)) {
      Serial.println("[APP] Impossible de se connecter au broker. Redémarrage…");
      ESP.restart();
  }
  mqtt.subscribe(CONFIG_TOPIC_CMD, 1);
  mqtt.publish(CONFIG_TOPIC_STATUS, "online", /*retain=*/true);

  // ESP-NOW (Pas de pair à enregistrer côté récepteur → nullptr)
  if (!espnowComm.begin(nullptr,true)) {
      Serial.println("FATAL : Initialisation ESP-NOW échouée. Redémarrage...");
      delay(3000);
      ESP.restart();
  }

  // Setup done
  Serial.println("setup: done");
}

// ---loop-------------------------------------------------
void loop() {
  // Reconnexion Wi-Fi si nécessaire (non bloquant)
  handleWiFiReconnect();

  // Maintien connexion mqtt + réception messages
  mqtt.loop();

  // Initialisation de la mesure du temps
  uint32_t now = millis();
  // Si l'intervale de temps s'est écoulé
  if (now - lastMesure >= CONFIG_MESURE_INTERVAL){
    // Actualisation de la mesure du temps
    lastMesure = now;
  }

  // Réception message ESP-NOW
  if (espnowComm.hasNewMessage()) {
      // Get last message
      String   msg = espnowComm.getLastMessage();
      // Désérialisation → SensorData_t
      SensorData_t parsed;
      bool deserialisationIsOk = stringToSensorData(msg, parsed);
      // si la deserialisation est OK
      if (deserialisationIsOk) {
          // Print mesures
          Serial.printf("[DATA] Temp=%.2f°C  EC=%.2fmS/cm  Niveau=%.1fcm  Flux=%s\n",
              parsed.waterTemp,
              parsed.waterEc,
              parsed.waterLevel,
              parsed.waterPresent ? "OUI" : "NON");
      } else {
          Serial.println("ERREUR : parsing SensorData_t échoué !");
      }

      // send MQTT
      mqtt.publish(CONFIG_TOPIC_waterTemp, String(parsed.waterTemp, 2));
      mqtt.publish(CONFIG_TOPIC_waterEc, String(parsed.waterEc, 2));
      mqtt.publish(CONFIG_TOPIC_waterLevel, String(parsed.waterLevel, 2));
      mqtt.publish(CONFIG_TOPIC_waterPresent, String(parsed.waterPresent ? "1" : "0"));

      Serial.println("========================");
  }
}