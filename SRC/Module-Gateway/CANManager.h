#ifndef CAN_MANAGER_H
#define CAN_MANAGER_H

#include <Arduino.h>
#include <driver/twai.h>

/**
 * @file CANManager.h
 * @brief Gestionnaire CAN pour ESP32 avec module SN65HVD230
 * @details Permet l'envoi et la réception de messages CAN contenant des booleans et des floats
 */

class CANManager {
public:
    /**
     * @brief Énumération des états possibles du gestionnaire CAN
     */
    enum CANStatus {
        CAN_UNINITIALIZED,
        CAN_INITIALIZED,
        CAN_RUNNING,
        CAN_ERROR
    };

    /**
     * @brief Énumération des types de données supportées
     */
    enum DataType {
        DATA_BOOL,
        DATA_FLOAT
    };

    /**
     * @brief Structure pour recevoir les messages CAN
     */
    struct CANMessage {
        uint32_t id;
        uint8_t dlc;
        uint8_t data[8];
        DataType type;
    };

    /**
     * @brief Constructeur par défaut
     */
    CANManager();

    /**
     * @brief Initialise le bus CAN
     * @param rxPin Broche de réception (GPIO16 pour ESP32 par défaut)
     * @param txPin Broche de transmission (GPIO17 pour ESP32 par défaut)
     * @param baudrate Vitesse en bauds (1000 kbps par défaut)
     * @return true si l'initialisation a réussi, false sinon
     */
    bool begin(int rxPin = 16, int txPin = 17, uint32_t baudrate = 1000000);

    /**
     * @brief Arrête le bus CAN
     * @return true si l'arrêt a réussi, false sinon
     */
    bool end();

    /**
     * @brief Envoie une valeur booléenne sur le bus CAN
     * @param id Identifiant du message CAN (11 bits ou 29 bits)
     * @param value Valeur booléenne à envoyer (true ou false)
     * @return true si l'envoi a réussi, false sinon
     */
    bool sendBool(uint32_t id, bool value);

    /**
     * @brief Envoie une valeur flottante sur le bus CAN
     * @param id Identifiant du message CAN (11 bits ou 29 bits)
     * @param value Valeur float à envoyer
     * @return true si l'envoi a réussi, false sinon
     */
    bool sendFloat(uint32_t id, float value);

    /**
     * @brief Reçoit un message du bus CAN (non bloquant)
     * @param message Structure où stocker le message reçu
     * @return true si un message a été reçu, false sinon
     */
    bool receive(CANMessage &message);

    /**
     * @brief Obtient l'état actuel du gestionnaire CAN
     * @return État du gestionnaire
     */
    CANStatus getStatus() const;

    /**
     * @brief Retourne une description textuelle de l'état
     * @return Chaîne de caractères décrivant l'état
     */
    const char* getStatusString() const;

    /**
     * @brief Décodifie un message CAN contenant un booléen
     * @param message Message CAN reçu
     * @return Valeur booléenne décodée
     */
    static bool decodeBool(const CANMessage &message);

    /**
     * @brief Décodifie un message CAN contenant un float
     * @param message Message CAN reçu
     * @return Valeur float décodée
     */
    static float decodeFloat(const CANMessage &message);

    /**
     * @brief Configure le filtre d'acceptation des messages
     * @param id ID du message à accepter
     * @param mask Masque de filtrage
     * @param idExtended true pour les identifiants 29 bits, false pour 11 bits
     * @return true si la configuration a réussi
     */
    bool setFilter(uint32_t id, uint32_t mask, bool idExtended = false);

private:
    CANStatus status;
    twai_general_config_t generalConfig;
    twai_timing_config_t timingConfig;
    twai_filter_config_t filterConfig;

    /**
     * @brief Convertit un float en tableau de 4 bytes (IEEE 754)
     * @param value Valeur float
     * @param bytes Tableau de 4 bytes (sortie)
     */
    static void floatToBytes(float value, uint8_t *bytes);

    /**
     * @brief Convertit un tableau de 4 bytes en float (IEEE 754)
     * @param bytes Tableau de 4 bytes
     * @return Valeur float
     */
    static float bytesToFloat(const uint8_t *bytes);
};

#endif // CAN_MANAGER_H
