// board : ESP32 (Arduino core)
//
// Passerelle CAN → MQTT :
//  - ESPConnect gère le Wi-Fi (portail captif, reconnexion) et le client MQTT
//    (configuré depuis la page http://<ip>/status, statut online/offline + LWT) ;
//  - les mesures reçues sur le bus CAN sont publiées sur MQTT ;
//  - les commandes MQTT pilotent les relais ;
//  - le firmware peut être mis à jour par le réseau (OTA, env esp32dev_ota).

#include <Arduino.h>
#include <ESPConnect.h>
#include "Config.h"
#include "CANManager.h"
#include "RelayController.h"

// ── Objets globaux ────────────────────────────────────────────────────────────
CANManager      can;
RelayController relay1(CONFIG_PIN_RELAY1, true);
RelayController relay2(CONFIG_PIN_RELAY2, true);

// ── Réception MQTT : commandes des relais ─────────────────────────────────────
static bool parseOnOff(const String& payload) {
  return payload == "ON" || payload == "1";
}

void onMqttMessage(const String& topic, const String& payload) {
  if (topic == CONFIG_TOPIC_CMD_RELAY1) {
    bool on = parseOnOff(payload);
    Serial.printf("Commande Relay1 : %s\n", on ? "MARCHE" : "ARRÊT");
    on ? relay1.turnOn() : relay1.turnOff();
  } else if (topic == CONFIG_TOPIC_CMD_RELAY2) {
    bool on = parseOnOff(payload);
    Serial.printf("Commande Relay2 : %s\n", on ? "MARCHE" : "ARRÊT");
    on ? relay2.turnOn() : relay2.turnOff();
  } else {
    Serial.printf("Topic inconnu : %s\n", topic.c_str());
  }
}

// ── Traitement des messages CAN reçus ─────────────────────────────────────────
void traiterCanMsg(const CANManager::CANMessage& rxMessage) {
  switch (rxMessage.id) {
    case CONFIG_CAN_ID_waterPresent: {
      bool value = CANManager::decodeBool(rxMessage);
      espConnect.publish(CONFIG_TOPIC_waterPresent, value ? "1" : "0");
      Serial.printf("Flux=%s\n", value ? "true" : "false");
      break;
    }

    case CONFIG_CAN_ID_waterTemp: {
      float value = CANManager::decodeFloat(rxMessage);
      espConnect.publish(CONFIG_TOPIC_waterTemp, value);
      Serial.printf("Temp=%.2f°C\n", value);
      break;
    }

    case CONFIG_CAN_ID_waterEc: {
      float value = CANManager::decodeFloat(rxMessage);
      espConnect.publish(CONFIG_TOPIC_waterEc, value);
      Serial.printf("EC=%.2fµS/cm\n", value);
      break;
    }

    case CONFIG_CAN_ID_waterLevel: {
      float value = CANManager::decodeFloat(rxMessage);
      espConnect.publish(CONFIG_TOPIC_waterLevel, value);
      Serial.printf("Niveau=%.1fcm\n", value);
      break;
    }

    default:
      Serial.printf("ID CAN inconnu : 0x%03lX\n", (unsigned long)rxMessage.id);
      break;
  }
}

// ── Surveillance de la réception CAN ──────────────────────────────────────────
// Publie une erreur (retenue) sur CONFIG_TOPIC_error quand le capteur se tait
// plus de CONFIG_CAN_TIMEOUT_MS, puis "OK" quand les messages reviennent.
// Si MQTT est déconnecté au moment du changement d'état, la publication est
// retentée à chaque tour de loop jusqu'à ce qu'elle réussisse.
static uint32_t lastCanRxMs     = 0;
static bool     canTimedOut     = false;
static bool     canStatePending = false;

void surveillerCan(bool messageRecu) {
  uint32_t now = millis();

  if (messageRecu) {
    lastCanRxMs = now;
    if (canTimedOut) {
      canTimedOut     = false;
      canStatePending = true;
      Serial.println("[CAN] Réception rétablie");
    }
  } else if (!canTimedOut && now - lastCanRxMs > CONFIG_CAN_TIMEOUT_MS) {
    canTimedOut     = true;
    canStatePending = true;
    Serial.printf("[CAN] Aucun message depuis %lu s\n",
                  (unsigned long)((now - lastCanRxMs) / 1000));
  }

  if (canStatePending && espConnect.isMqttConnected()) {
    const char* payload = canTimedOut ? "CAN_TIMEOUT: aucun message du capteur" : "OK";
    if (espConnect.publish(CONFIG_TOPIC_error, payload, true)) {
      canStatePending = false;
    }
  }
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(CONFIG_BAUDRATE);
  delay(1000);
  Serial.println("\n[Setup] start");

  // Relais (sortie à l'état sûr au plus tôt)
  relay1.begin();
  relay2.begin();

  // Bus CAN
  if (!can.begin(CONFIG_PIN_CAN_RX, CONFIG_PIN_CAN_TX, CONFIG_CAN_BAUDRATE)) {
    Serial.println("[Setup] ERREUR : impossible d'initialiser le CAN !");
    while (true) delay(100);
  }

  // Wi-Fi + MQTT + OTA (souscriptions renouvelées à chaque reconnexion)
  espConnect.onMqttMessage(onMqttMessage);
  espConnect.subscribe(CONFIG_TOPIC_CMD, 1);
  espConnect.enableOTA(CONFIG_OTA_PASSWORD, CONFIG_OTA_PORT);
  espConnect.begin(CONFIG_DEVICE_NAME, CONFIG_AP_NAME);

  lastCanRxMs = millis();  // le délai de surveillance CAN démarre ici
  Serial.println("[Setup] done");
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
  // Wi-Fi, MQTT, bouton de réinitialisation et callbacks
  espConnect.loop();

  CANManager::CANMessage rxMessage;
  bool recu = can.receive(rxMessage);
  if (recu) {
    traiterCanMsg(rxMessage);
  }
  surveillerCan(recu);
}
