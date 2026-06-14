#include "CANManager.h"

/**
 * @file CANManager.cpp
 * @brief Implémentation du gestionnaire CAN pour ESP32
 */

CANManager::CANManager() : status(CAN_UNINITIALIZED) {
    // Configuration par défaut du TWAI (CAN du ESP32)
    generalConfig = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)17, (gpio_num_t)16, TWAI_MODE_NORMAL);
    
    // Configuration temporelle pour 1 Mbps
    timingConfig = TWAI_TIMING_CONFIG_1MBITS();
    
    // Pas de filtrage (accepte tous les messages)
    filterConfig = TWAI_FILTER_CONFIG_ACCEPT_ALL();
}

bool CANManager::begin(int rxPin, int txPin, uint32_t baudrate) {
    // Reconfigure les broches
    generalConfig.tx_io = (gpio_num_t)txPin;
    generalConfig.rx_io = (gpio_num_t)rxPin;

    // Configure la vitesse de transmission
    switch (baudrate) {
        case 250000:
            timingConfig = TWAI_TIMING_CONFIG_250KBITS();
            break;
        case 500000:
            timingConfig = TWAI_TIMING_CONFIG_500KBITS();
            break;
        case 1000000:
        default:
            timingConfig = TWAI_TIMING_CONFIG_1MBITS();
            break;
    }

    // Installe le pilote TWAI
    if (twai_driver_install(&generalConfig, &timingConfig, &filterConfig) != ESP_OK) {
        status = CAN_ERROR;
        return false;
    }

    // Démarre le pilote TWAI
    if (twai_start() != ESP_OK) {
        status = CAN_ERROR;
        twai_driver_uninstall();
        return false;
    }

    status = CAN_RUNNING;
    return true;
}

bool CANManager::end() {
    if (status == CAN_UNINITIALIZED) {
        return false;
    }

    if (twai_stop() != ESP_OK) {
        status = CAN_ERROR;
        return false;
    }

    if (twai_driver_uninstall() != ESP_OK) {
        status = CAN_ERROR;
        return false;
    }

    status = CAN_UNINITIALIZED;
    return true;
}

bool CANManager::sendBool(uint32_t id, bool value) {
    if (status != CAN_RUNNING) {
        return false;
    }

    twai_message_t message = {};
    message.identifier = id;
    message.data_length_code = 1;
    message.data[0] = value ? 0x01 : 0x00;

    esp_err_t result = twai_transmit(&message, pdMS_TO_TICKS(10));
    return result == ESP_OK;
}

bool CANManager::sendFloat(uint32_t id, float value) {
    if (status != CAN_RUNNING) {
        return false;
    }

    twai_message_t message = {};
    message.identifier = id;
    message.data_length_code = 4;
    
    floatToBytes(value, message.data);

    esp_err_t result = twai_transmit(&message, pdMS_TO_TICKS(10));
    return result == ESP_OK;
}

bool CANManager::receive(CANMessage &message) {
    if (status != CAN_RUNNING) {
        return false;
    }

    twai_message_t rxMessage;
    esp_err_t result = twai_receive(&rxMessage, 0);

    if (result != ESP_OK) {
        return false;
    }

    message.id = rxMessage.identifier;
    message.dlc = rxMessage.data_length_code;
    
    for (int i = 0; i < 8; i++) {
        message.data[i] = rxMessage.data[i];
    }

    // Détermine le type de données en fonction de la longueur
    if (message.dlc == 1) {
        message.type = DATA_BOOL;
    } else if (message.dlc == 4) {
        message.type = DATA_FLOAT;
    }

    return true;
}

CANManager::CANStatus CANManager::getStatus() const {
    return status;
}

const char* CANManager::getStatusString() const {
    switch (status) {
        case CAN_UNINITIALIZED:
            return "Non initialisé";
        case CAN_INITIALIZED:
            return "Initialisé";
        case CAN_RUNNING:
            return "En cours d'exécution";
        case CAN_ERROR:
            return "Erreur";
        default:
            return "État inconnu";
    }
}

bool CANManager::decodeBool(const CANMessage &message) {
    if (message.dlc < 1) {
        return false;
    }
    return message.data[0] != 0x00;
}

float CANManager::decodeFloat(const CANMessage &message) {
    if (message.dlc < 4) {
        return 0.0f;
    }
    return bytesToFloat(message.data);
}

bool CANManager::setFilter(uint32_t id, uint32_t mask, bool idExtended) {
    // Configuration du filtre
    twai_filter_config_t newFilter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    // Configure le filtre personnalisé
    newFilter.acceptance_code = id << (idExtended ? 0 : 21);
    newFilter.acceptance_mask = mask << (idExtended ? 0 : 21);
    newFilter.single_filter = true;

    // Note: La reconfiguration du filtre nécessite une réinstallation du pilote
    // Ce n'est donc pas implémenté ici pour éviter les interruptions
    // Les filtres doivent être configurés avant l'appel à begin()

    return true;
}

void CANManager::floatToBytes(float value, uint8_t *bytes) {
    // Interprète le float comme un tableau de 4 bytes (IEEE 754)
    uint8_t *ptr = (uint8_t *)&value;
    bytes[0] = ptr[0];
    bytes[1] = ptr[1];
    bytes[2] = ptr[2];
    bytes[3] = ptr[3];
}

float CANManager::bytesToFloat(const uint8_t *bytes) {
    // Reconstruit le float à partir de 4 bytes
    float value;
    uint8_t *ptr = (uint8_t *)&value;
    ptr[0] = bytes[0];
    ptr[1] = bytes[1];
    ptr[2] = bytes[2];
    ptr[3] = bytes[3];
    return value;
}
