# HydroponX
Ce projet décrit un ensemble de module gérés par des microcontroleur ESP32 permettant de controler automatiquement un système de culture hydroponique.
Tous ces modules communiquent entre eux par le protocole ESP-NOW.

Le premier module recolte les mesures de différents capteurs:
- La temperature de l'eau par un capteur DS18B20
- l'EC de l'eau par un capteur TDS de CQRobot
- La quantité d'eau restant dans le réservoir d'eau par un capteur JNS-SR04T
- La présence d'eau revenant dans le réservoir par un capteur de niveau d'eau de CQRobot

Le second module:
- pilote la pompe à eau du système par un module relais
- collecte les mesures des capteurs et les envoient sur un broker MQTT en passant par une connection Wifi

## Architecture

## Module Capteur : cablage
| Capteur             | Broche ESP32 | Remarques                          |
|---------------------|--------------|------------------------------------|
| DS18B20 Data        | GPIO 4       | Résistance pull-up 4.7 kΩ vers 5V. |
| DS18B20 VCC         | 5V           |                                    |
| DS18B20 GND         | GND          |                                    |
| Sonde EC (signal)   | GPIO 34      |                                    |
| Sonde EC VCC        | 5V           |                                    |
| Sonde EC GND        | GND          |                                    |
| JNS-SR04T Trig      | GPIO 12      |                                    |
| JNS-SR04T Echo      | GPIO 14      | Diviseur 5V→3.3V                   |
| JNS-SR04T VCC       | 5V           |                                    |
| JNS-SR04T GND       | GND          |                                    |
| Niveau eau (signal) | GPIO 27      | Pull-up interne activé             |
| Niveau eau VCC      | 5V           |                                    |
| Niveau eau GND      | GND          |                                    |
> 

## Module gateway MQTT