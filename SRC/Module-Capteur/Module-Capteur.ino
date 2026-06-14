// board : ESP32 V3.3.8

#include <Arduino.h>
#include "Config.h"
#include "SensorTemperature.h"
#include "SensorWaterPresent.h"
#include "SensorSonar.h"
#include "SensorTDS.h"
#include "CANManager.h"

// Crée une instance du gestionnaire CAN
CANManager can;

// Definition des sensors
SensorTemperature SensorTemperature1(CONFIG_PIN_DS18B20);
SensorWaterPresent SensorWaterPresent1(CONFIG_PIN_WaterPres, false);
SensorSonar SensorSonar1(CONFIG_PIN_TRIG, CONFIG_PIN_ECHO);
SensorTDS SensorTDS1(CONFIG_PIN_TDS);

// Deniere mesure (ms)
uint32_t lastMesure = 0;

// ---setup------------------------------------------------
void setup() {
  // setup serial
  Serial.begin(CONFIG_BAUDRATE);
  delay(1000);

  // Setup start
  Serial.println("\nsetup: start");

  // Start sensor
  SensorTemperature1.begin();
  SensorWaterPresent1.begin();
  SensorSonar1.begin();
  SensorTDS1.begin();

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
  // Initialisation de la mesure du temps
  uint32_t now = millis();

  if (now - lastMesure >= CONFIG_MESURE_INTERVAL){
    // Actualisation de la mesure du temps
    lastMesure = now;
    
    // Mesure des capteurs
    float waterTemp = SensorTemperature1.read();
    bool waterPresent = SensorWaterPresent1.read();
    float waterLevel = SensorSonar1.measureMedianCm(9);
    float waterEc = SensorTDS1.readEC(waterTemp);

    // Print mesures
    Serial.printf("[DATA] Temp=%.2f°C  EC=%.2fmS/cm  Niveau=%.1fcm  Flux=%s\n",
                  waterTemp,
                  waterEc,
                  waterLevel,
                  waterPresent ? "OUI" : "NON");

    if (can.sendFloat(0x101, waterLevel)) {
        Serial.printf("Float envoyé: ID=0x101, Value=%.2f\n", waterLevel);
    } else {
        Serial.printf("Erreur lors de l'envoi du Float sur ID=0x101!\n");
    }
    delay(10); // Petit délai entre les envois

  }
}