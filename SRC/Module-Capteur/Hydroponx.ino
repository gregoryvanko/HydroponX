// board : ESP32 V3.3.8

#include <Arduino.h>
#include "Config.h"
#include "SensorTemperature.h"
#include "SensorWaterPresent.h"
#include "SensorSonar.h"
#include "SensorTDS.h"

// Structure de l'object contenant les mesures des capteurs
typedef struct {
  float  waterTemp;       // °C
  float  waterEc;         // mS/cm
  float  waterLevel;      // cm
  bool   waterPresent;    // true = flux détecté
  uint32_t timestamp;     // millis() au moment de l'envoi
} SensorData_t;


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
    SensorData_t data;
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
  }
  
}