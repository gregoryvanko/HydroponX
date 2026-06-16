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
  Serial.println("\n[Setup] start");

  // Start sensor
  SensorTemperature1.begin();
  SensorWaterPresent1.begin();
  SensorSonar1.begin();
  SensorTDS1.begin();

  // Initialisation du bus CAN
  if (!can.begin(CONFIG_PIN_CAN_RX, CONFIG_PIN_CAN_TX, CONFIG_CAN_BAUDRATE)) {
      Serial.println("[Setup] ERREUR: Impossible d'initialiser le CAN!");
      while (1) {
          delay(100);
      }
  }

  // Setup done
  Serial.println("[Setup] done");
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

    // Send Can
    can.sendFloat(CONFIG_CAN_ID_waterTemp, waterTemp);
    delay(10);
    can.sendFloat(CONFIG_CAN_ID_waterEc, waterEc);
    delay(10);
    can.sendFloat(CONFIG_CAN_ID_waterLevel, waterLevel);
    delay(10);
    can.sendBool(CONFIG_CAN_ID_waterPresent, waterPresent);
    delay(10);

    // Print mesures
    Serial.printf("Send CAN => Temp=%.2f°C  EC=%.2fmS/cm  Niveau=%.1fcm  Flux=%s\n",
                  waterTemp,
                  waterEc,
                  waterLevel,
                  waterPresent ? "OUI" : "NON");


  }
}