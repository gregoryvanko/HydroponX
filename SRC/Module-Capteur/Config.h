#pragma once

// ── PIN -──────────────────────────────────────────────────────────────────────
constexpr uint8_t CONFIG_PIN_DS18B20 = 4;
constexpr uint8_t CONFIG_PIN_WaterPres = 35;
constexpr uint8_t CONFIG_PIN_TRIG = 5;
constexpr uint8_t CONFIG_PIN_ECHO = 18;
constexpr uint8_t CONFIG_PIN_TDS = 34;
constexpr gpio_num_t CONFIG_PIN_CAN_TX = GPIO_NUM_21;
constexpr gpio_num_t CONFIG_PIN_CAN_RX = GPIO_NUM_22;

// ── Loop ──────────────────────────────────────────────────────────────────────
constexpr uint16_t CONFIG_BAUDRATE = 9600;
constexpr uint32_t CONFIG_MESURE_INTERVAL = 5000;

// ── ESP-Now ───────────────────────────────────────────────────────────────────
static const uint8_t RECEIVER_MAC[] = { 0xB4, 0xBF, 0xE9, 0x0B, 0x73, 0x08 };

// ── CAN -──────────────────────────────────────────────────────────────────────
constexpr uint32_t CONFIG_CAN_BAUDRATE        = 500000;
constexpr uint32_t CONFIG_CAN_ID_waterTemp    = 0x101;
constexpr uint32_t CONFIG_CAN_ID_waterEc      = 0x102;
constexpr uint32_t CONFIG_CAN_ID_waterLevel   = 0x103;
constexpr uint32_t CONFIG_CAN_ID_waterPresent = 0x104;