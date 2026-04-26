# HydroponX
Ce projet décrit un ensemble de module gérés par des microcontroleur ESP32 permettant de controler automatiquement un système de culture hydroponique.
Tous ces modules communiquent entre eux par le protocole ESP-NOW.

Le premier module recolte les mesures de différents capteurs:
- La temperature de l'eau
- l'EC de l'eau
- La quantité d'eau restant dans le réservoir d'eau
- La présence d'eau revenant dans le réservoir

Le second module:
- pilote la pompe à eau du système
- collecte les mesures des capteurs et les envoient sur un broker MQTT en passant par une connection Wifi

## Module Capteur

## Module gateway MQTT