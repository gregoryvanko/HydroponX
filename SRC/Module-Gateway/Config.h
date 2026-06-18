#pragma once

// ── PIN -──────────────────────────────────────────────────────────────────────
constexpr uint8_t CONFIG_PIN_CAN_TX = 17;
constexpr uint8_t CONFIG_PIN_CAN_RX = 16;
constexpr uint8_t CONFIG_PIN_RELAY1 = 32;
constexpr uint8_t CONFIG_PIN_RELAY2 = 33;

// ── Loop ──────────────────────────────────────────────────────────────────────
constexpr uint16_t CONFIG_BAUDRATE = 9600;

// ── WiFi ──────────────────────────────────────────────────────────────────────
constexpr char* CONFIG_WIFI_SSID     = "blacknet-IOT";
constexpr char* CONFIG_WIFI_PASSWORD = "gregoryvk99iot";

// ── MQTT ──────────────────────────────────────────────────────────────────────
static const char* CONFIG_MQTT_BROKER   = "192.168.40.40";  // IP ou hostname du broker
static const uint16_t CONFIG_MQTT_PORT  = 1883;
static const char* CONFIG_MQTT_CLIENT   = "ESP32-Hydro1";
static const char* CONFIG_MQTT_USER     = "gregory";     // nullptr si pas d'auth
static const char* CONFIG_MQTT_PASS     = "gregory";
 
// ── MQTT Topics ───────────────────────────────────────────────────────────────
static const char* CONFIG_TOPIC_waterTemp    = "hydro1/waterTemp";
static const char* CONFIG_TOPIC_waterEc      = "hydro1/waterEc";
static const char* CONFIG_TOPIC_waterLevel   = "hydro1/waterLevel";
static const char* CONFIG_TOPIC_waterPresent = "hydro1/waterPresent";
static const char* CONFIG_TOPIC_CMD          = "hydro1/cmd/#";  // wildcard
static const char* CONFIG_TOPIC_STATUS       = "hydro1/status";

// ── CAN -──────────────────────────────────────────────────────────────────────
constexpr uint32_t CONFIG_CAN_BAUDRATE        = 500000;
constexpr uint32_t CONFIG_CAN_ID_waterTemp    = 0x101;
constexpr uint32_t CONFIG_CAN_ID_waterEc      = 0x102;
constexpr uint32_t CONFIG_CAN_ID_waterLevel   = 0x103;
constexpr uint32_t CONFIG_CAN_ID_waterPresent = 0x104;