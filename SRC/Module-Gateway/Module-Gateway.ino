// board : ESP32 V3.3.8

#include <Arduino.h>
#include "Config.h"

// Structure de l'object contenant les mesures des capteurs
typedef struct {
  float  waterTemp;       // °C
  float  waterEc;         // mS/cm
  float  waterLevel;      // cm
  bool   waterPresent;    // true = flux détecté
  uint32_t timestamp;     // millis() au moment de l'envoi
} SensorData_t;

// Deniere mesure (ms)
uint32_t lastMesure = 0;

// ---setup------------------------------------------------
void setup() {
  // setup serial
  Serial.begin(CONFIG_BAUDRATE);
  delay(1000);

  // Setup start
  Serial.println("\nsetup: start");


  // Setup done
  Serial.println("setup: done");
}

// ---loop-------------------------------------------------
void loop() {
  // Initialisation de la mesure du temps
  uint32_t now = millis();
  // Si l'intervale de temps s'est écoulé
  if (now - lastMesure >= CONFIG_MESURE_INTERVAL){
  }
  
}