#pragma once

constexpr uint8_t CONFIG_PIN_DS18B20 = 4;
constexpr uint8_t CONFIG_PIN_WaterPres = 35;
constexpr uint8_t CONFIG_PIN_TRIG = 5;
constexpr uint8_t CONFIG_PIN_ECHO = 18;
constexpr uint8_t CONFIG_PIN_TDS = 34;

constexpr uint16_t CONFIG_BAUDRATE = 9600;
constexpr uint32_t CONFIG_MESURE_INTERVAL = 5000;

static constexpr uint8_t ESPNOW_CHANNEL = 1;
static const uint8_t RECEIVER_MAC[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
