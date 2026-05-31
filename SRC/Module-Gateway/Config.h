#pragma once

// ── Loop ──────────────────────────────────────────────────────────────────────
constexpr uint16_t CONFIG_BAUDRATE = 9600;
constexpr uint32_t CONFIG_MESURE_INTERVAL = 5000;

// ── WiFi ──────────────────────────────────────────────────────────────────────
constexpr char* CONFIG_WIFI_SSID     = "blacknet-IOT";
constexpr char* CONFIG_WIFI_PASSWORD = "gregoryvk99iot";

// ── MQTT ──────────────────────────────────────────────────────────────────────
static const char* CONFIG_MQTT_BROKER   = "192.168.40.40";  // IP ou hostname du broker
static const uint16_t CONFIG_MQTT_PORT  = 1883;
static const char* CONFIG_MQTT_CLIENT   = "ESP32-Hydro1";
static const char* CONFIG_MQTT_USER     = "gregory";     // nullptr si pas d'auth
static const char* CONFIG_MQTT_PASS     = "gregory";
 
// ── Topics ────────────────────────────────────────────────────────────────────
static const char* CONFIG_TOPIC_waterTemp    = "hydro1/waterTemp";
static const char* CONFIG_TOPIC_waterEc      = "hydro1/waterEc";
static const char* CONFIG_TOPIC_waterLevel   = "hydro1/waterLevel";
static const char* CONFIG_TOPIC_waterPresent = "hydro1/waterPresent";
static const char* CONFIG_TOPIC_CMD          = "hydro1/cmd/#";  // wildcard
static const char* CONFIG_TOPIC_STATUS       = "hydro1/status";