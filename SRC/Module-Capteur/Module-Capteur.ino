// board : ESP32 V3.3.8

#include <Arduino.h>
#include "Config.h"
#include "SensorData.h"
#include "SensorTemperature.h"
#include "SensorWaterPresent.h"
#include "SensorSonar.h"
#include "SensorTDS.h"
#include "espnow_comm.h"


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

  // ESP-NOW
  if (!espnowComm.begin(RECEIVER_MAC, false)) {
      Serial.println("FATAL : Initialisation ESP-NOW échouée. Redémarrage...");
      delay(3000);
      ESP.restart();
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

    // Init Sensor Data
    SensorData_t data = {
        .waterTemp    = 0.0f,
        .waterEc      = 0.0f,
        .waterLevel   = 0.0f,
        .waterPresent = false,
        .timestamp    = millis()
    };
    
    // Mesure des capteurs
    data.timestamp = now;
    data.waterTemp = SensorTemperature1.read();
    data.waterPresent = SensorWaterPresent1.read();
    data.waterLevel = SensorSonar1.measureMedianCm(9);
    data.waterEc = SensorTDS1.readEC(data.waterTemp);

    // Print mesures
    Serial.printf("[DATA] Temp=%.2f°C  EC=%.2fmS/cm  Niveau=%.1fcm  Flux=%s\n",
                  data.waterTemp,
                  data.waterEc,
                  data.waterLevel,
                  data.waterPresent ? "OUI" : "NON");
    
    // Sérialisation data
    String dataSerialized = sensorDataToString(data);

    // Send ESP-NOW
    espnowComm.sendMessage(dataSerialized);
  }
}