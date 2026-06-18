// board : ESP32 V3.3.8

#include <Arduino.h>
#include "Config.h"
#include <WiFi.h>
#include "WifiHelper.h"
#include "MqttGateway.h"
#include "CANManager.h"
#include "RelayController.h"

// ── Objets globaux ────────────────────────────────────────────────────────────
CANManager can;
WiFiClient  wifiClient;
MqttGateway mqtt(wifiClient, CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT, CONFIG_MQTT_CLIENT);
RelayController relay1(CONFIG_PIN_RELAY1, true);
RelayController relay2(CONFIG_PIN_RELAY2, true);

// ─────────────────────────────────────────────────────────────────────────────
// Callback réception MQTT
// ─────────────────────────────────────────────────────────────────────────────
void onMqttMessage(const String& topic, const String& payload) {
    Serial.printf("Message MQTT reçu — topic: %s | payload: %s\n",
                  topic.c_str(), payload.c_str());
 
    if (topic == "hydro1/cmd/out1") {
      bool on = (payload == "ON" || payload == "1");
      Serial.printf("Commande Relay1 : %s\n", on ? "MARCHE" : "ARRÊT");
      on ? relay1.turnOn() : relay1.turnOff();
    } else if (topic == "hydro1/cmd/out2"){
      bool on = (payload == "ON" || payload == "1");
      Serial.printf("Commande Relay2 : %s\n", on ? "MARCHE" : "ARRÊT");
      on ? relay2.turnOn() : relay2.turnOff();
    }
    else {
      Serial.printf("Topic not found");
    }
}

// ---------------------------------------------------------------------------
// Traitement des message CAN reçues
// ---------------------------------------------------------------------------
void traiterCanMsg(const CANManager::CANMessage &rxMessage){
  switch (rxMessage.id) {

    case CONFIG_CAN_ID_waterPresent: {
      bool value = CANManager::decodeBool(rxMessage);
      mqtt.publish(CONFIG_TOPIC_waterPresent, String(value ? "1" : "0"));
      Serial.printf("Flux=%s\n", value ? "true" : "false");
      break;
    }

    case CONFIG_CAN_ID_waterTemp: {
      float value = CANManager::decodeFloat(rxMessage);
      mqtt.publish(CONFIG_TOPIC_waterTemp, String(value, 2));
      Serial.printf("Temp=%.2f°C\n", value);
      break;
    }

    case CONFIG_CAN_ID_waterEc: {
      float value = CANManager::decodeFloat(rxMessage);
      mqtt.publish(CONFIG_TOPIC_waterEc, String(value, 2));
      Serial.printf("EC=%.2fmS/cm\n", value);
      break;
    }

    case CONFIG_CAN_ID_waterLevel: {
      float value = CANManager::decodeFloat(rxMessage);
      mqtt.publish(CONFIG_TOPIC_waterLevel, String(value, 2));
      Serial.printf("Niveau=%.1fcm\n", value);
      break;
    }

    default:
      Serial.printf("ID CAN inconnu : 0x%03lX\n", (unsigned long)rxMessage.id);
      break;
  }
}

// ---setup------------------------------------------------
void setup() {
  // setup serial
  Serial.begin(CONFIG_BAUDRATE);
  delay(1000);

  // Setup start
  Serial.println("\n[Setup] start");

  // Wifi connection to routeur
  connectToWiFi(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);

  // MQTT connection
  mqtt.setCredentials(CONFIG_MQTT_USER, CONFIG_MQTT_PASS);
  mqtt.setLastWill(CONFIG_TOPIC_STATUS, "offline", /*retain=*/true, /*qos=*/1);
  mqtt.setMessageCallback(onMqttMessage);
  if (!mqtt.connect(5)) {
      Serial.println("[Setup] Impossible de se connecter au broker. Redémarrage…");
      ESP.restart();
  }
  mqtt.subscribe(CONFIG_TOPIC_CMD, 1);
  mqtt.publish(CONFIG_TOPIC_STATUS, "online", /*retain=*/true);

  // Initialisation du bus CAN
  if (!can.begin(CONFIG_PIN_CAN_RX, CONFIG_PIN_CAN_TX, CONFIG_CAN_BAUDRATE)) {
      Serial.println("[Setup] ERREUR: Impossible d'initialiser le CAN!");
      while (1) {
          delay(100);
      }
  }

  // Setup Relay
  relay1.begin();
  relay2.begin();

  // Setup done
  Serial.println("[Setup] done");
}

// ---loop-------------------------------------------------
void loop() {
  // Reconnexion Wi-Fi si nécessaire (non bloquant)
  handleWiFiReconnect();

  // Maintien connexion mqtt + réception messages
  mqtt.loop();

  // Analyse des messages CAN
  CANManager::CANMessage rxMessage;
  if (can.receive(rxMessage)) {
    traiterCanMsg(rxMessage);
  }
}