#pragma once

#include <Arduino.h>

// ── Appareil ──────────────────────────────────────────────────────────────────
// Nom d'appareil : hostname, identifiant client MQTT et topic de statut
// ("hydro1/status"). Peut être modifié depuis la page /status d'ESPConnect.
constexpr const char* CONFIG_DEVICE_NAME = "hydro1";
// Point d'accès ouvert du portail de configuration Wi-Fi (premier démarrage)
constexpr const char* CONFIG_AP_NAME     = "HydroponX-Setup";

// ── OTA ───────────────────────────────────────────────────────────────────────
// CONFIG_OTA_PASSWORD est défini par platformio.ini depuis secrets.ini
#ifndef CONFIG_OTA_PASSWORD
#error "CONFIG_OTA_PASSWORD non défini : créer secrets.ini à partir de secrets.ini.example"
#endif
static_assert(sizeof(CONFIG_OTA_PASSWORD) > 1, "Le mot de passe OTA ne doit pas être vide");
constexpr uint16_t CONFIG_OTA_PORT = 3232;

// ── PIN ───────────────────────────────────────────────────────────────────────
constexpr uint8_t CONFIG_PIN_CAN_TX = 17;
constexpr uint8_t CONFIG_PIN_CAN_RX = 16;
constexpr uint8_t CONFIG_PIN_RELAY1 = 32;
constexpr uint8_t CONFIG_PIN_RELAY2 = 33;

// ── Série ─────────────────────────────────────────────────────────────────────
constexpr uint32_t CONFIG_BAUDRATE = 9600;

// ── MQTT Topics ───────────────────────────────────────────────────────────────
constexpr const char* CONFIG_TOPIC_waterTemp    = "hydro1/waterTemp";
constexpr const char* CONFIG_TOPIC_waterEc      = "hydro1/waterEc";
constexpr const char* CONFIG_TOPIC_waterLevel   = "hydro1/waterLevel";
constexpr const char* CONFIG_TOPIC_waterPresent = "hydro1/waterPresent";
constexpr const char* CONFIG_TOPIC_CMD          = "hydro1/cmd/#";  // wildcard
constexpr const char* CONFIG_TOPIC_CMD_RELAY1   = "hydro1/cmd/out1";
constexpr const char* CONFIG_TOPIC_CMD_RELAY2   = "hydro1/cmd/out2";

// ── CAN ───────────────────────────────────────────────────────────────────────
constexpr uint32_t CONFIG_CAN_BAUDRATE        = 500000;
constexpr uint32_t CONFIG_CAN_ID_waterTemp    = 0x101;
constexpr uint32_t CONFIG_CAN_ID_waterEc      = 0x102;
constexpr uint32_t CONFIG_CAN_ID_waterLevel   = 0x103;
constexpr uint32_t CONFIG_CAN_ID_waterPresent = 0x104;
