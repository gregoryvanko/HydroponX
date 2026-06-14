// board : ESP32 V3.3.8

#include <Arduino.h>
#include "Config.h"
#include <WiFi.h>
#include "WifiHelper.h"
#include "MqttGateway.h"
#include "CANManager.h"

// Crée une instance du gestionnaire CAN
CANManager can;

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

// ---------------------------------------------------------------------------
// Traitement des commandes reçues
// ---------------------------------------------------------------------------

// void traiter_commande(const can_trame_t &trame)
// {
//     switch (trame.id) {

//         case CONFIG_CAN_ID_waterPresent: {
//             bool etat = can_decoder_bool(&trame);
//             Serial.printf("[CMD] waterPresent → %s\n", etat ? "ON" : "OFF");
//             // TODO : piloter la sortie GPIO de la pompe nutritive
//             break;
//         }

//         case CONFIG_CAN_ID_waterTemp: {
//             float waterTemp = can_decoder_float(&trame);
//             Serial.printf("[CMD] Temp → %.2f\n", waterTemp);
//             // TODO : mettre à jour la consigne du régulateur de pH
//             break;
//         }

//         default:
//             Serial.printf("[CMD] ID inconnu : 0x%03lX\n", (unsigned long)trame.id);
//             break;
//     }
// }


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

  // Initialisation du bus CAN
  if (!can.begin(16, 17, 1000000)) {
      Serial.println("ERREUR: Impossible d'initialiser le CAN!");
      while (1) {
          delay(100);
      }
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

  CANManager::CANMessage rxMessage;
    
  if (can.receive(rxMessage)) {
      Serial.printf("[RX] ID=0x%03X, DLC=%d, ", rxMessage.id, rxMessage.dlc);

      // Décode et affiche selon le type
      if (rxMessage.type == CANManager::DATA_BOOL) {
          bool value = CANManager::decodeBool(rxMessage);
          Serial.printf("| Bool décodé: %s\n", value ? "true" : "false");
      } 
      else if (rxMessage.type == CANManager::DATA_FLOAT) {
          float value = CANManager::decodeFloat(rxMessage);
          Serial.printf("| Float décodé: %.4f\n", value);
      }
      else {
          Serial.println("| Type inconnu");
      }
  }

  delay(10); // Petit délai pour éviter de surcharger le processeur

  // send MQTT
  //mqtt.publish(CONFIG_TOPIC_waterTemp, String(parsed.waterTemp, 2));
  //mqtt.publish(CONFIG_TOPIC_waterEc, String(parsed.waterEc, 2));
  //mqtt.publish(CONFIG_TOPIC_waterLevel, String(parsed.waterLevel, 2));
  //mqtt.publish(CONFIG_TOPIC_waterPresent, String(parsed.waterPresent ? "1" : "0"));

  //Serial.println("========================");
}