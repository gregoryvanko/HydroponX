# HydroponX — Module Gateway (ESP32)

Firmware de l'ESP32 « Module Gateway » du système hydroponique HydroponX.
Ce module fait le pont entre le **bus CAN** et un **broker MQTT** (via Wi-Fi) :

- il écoute les mesures envoyées par le [Module Capteur](../PlatformIO-Capteur/README.md) et les republie sur des topics MQTT ;
- il souscrit à des topics de commande MQTT pour piloter deux relais (pompes).

> Code migré depuis le projet Arduino IDE [`SRC/Module-Gateway`](../SRC/Module-Gateway).

---

## Structure du projet

```
PlatformIO-Gateway/
├── platformio.ini              Carte, framework, vitesse série et dépendances externes
├── src/
│   ├── main.cpp                Programme principal : setup() / loop(), routage CAN ↔ MQTT
│   ├── Config.h                Broches, Wi-Fi, broker et topics MQTT, débit et IDs CAN
│   └── WifiHelper.h            Connexion et reconnexion Wi-Fi
└── lib/                        Bibliothèques locales (une classe par dossier)
    ├── CANManager/             CANManager.h / .cpp        → bus CAN (TWAI)
    ├── MqttGateway/            MqttGateway.h / .cpp       → client MQTT (PubSubClient)
    └── RelayController/        RelayController.h / .cpp   → sorties relais
```

Chaque dossier de `lib/` est compilé par PlatformIO comme une bibliothèque indépendante : son en-tête est trouvé automatiquement quand `main.cpp` fait `#include "<Nom>.h"`, sans réglage dans `platformio.ini`.
La bibliothèque externe PubSubClient est téléchargée via `lib_deps`.

---

## Vue d'ensemble

```
                      ┌─────────────── ESP32 Gateway ───────────────┐
 Bus CAN ──► Transceiver ──► CANManager ──► traiterCanMsg() ──► MqttGateway ──(Wi-Fi)──► Broker MQTT
                      │                                             │
                      │   RelayController ◄── onMqttMessage() ◄─────┘◄── hydro1/cmd/#
                      └──────┬──────────────┬───────────────────────┘
                          Relais 1       Relais 2
                         (GPIO 32)      (GPIO 33)
```

---

## Fonctionnement du programme

### `setup()`
1. Ouvre le port série (9600 bauds) pour les logs.
2. **Wi-Fi** : `connectToWiFi()` se connecte en mode station au SSID configuré. L'appel est **bloquant** tant que la connexion n'est pas établie.
3. **MQTT** :
   - renseigne l'identifiant / mot de passe du broker ;
   - déclare un *Last Will* : si l'ESP32 se déconnecte brutalement, le broker publie `offline` (retenu, QoS 1) sur `hydro1/status` ;
   - enregistre le callback `onMqttMessage` ;
   - tente la connexion (5 essais espacés de 5 s). En cas d'échec, l'ESP32 **redémarre** (`ESP.restart()`) ;
   - souscrit à `hydro1/cmd/#` (QoS 1) et publie `online` (retenu) sur `hydro1/status`.
4. **CAN** : démarre le bus à 500 kbit/s sur RX = GPIO 16, TX = GPIO 17. En cas d'échec, le programme se bloque dans une boucle infinie.
5. **Relais** : configure les deux sorties et met les relais à l'arrêt.

### `loop()`
La boucle tourne en continu, sans délai :

1. `handleWiFiReconnect()` : si le Wi-Fi est perdu, nouvelle tentative au plus toutes les 5 s.
2. `mqtt.loop()` : maintient la connexion au broker (reconnexion automatique, 1 tentative toutes les 5 s) et traite les messages entrants.
3. `can.receive()` (non bloquant) : si une trame CAN est disponible, elle est passée à `traiterCanMsg()`.

### CAN → MQTT : `traiterCanMsg()`
L'identifiant de la trame détermine son décodage et le topic de publication :

| ID CAN  | Décodage      | Topic MQTT            | Payload publié              |
|---------|---------------|-----------------------|-----------------------------|
| `0x101` | `decodeFloat` | `hydro1/waterTemp`    | température, 2 décimales    |
| `0x102` | `decodeFloat` | `hydro1/waterEc`      | conductivité, 2 décimales   |
| `0x103` | `decodeFloat` | `hydro1/waterLevel`   | distance en cm, 2 décimales |
| `0x104` | `decodeBool`  | `hydro1/waterPresent` | `1` ou `0`                  |

Les messages sont publiés en QoS 0, non retenus. Un ID inconnu est simplement signalé sur le port série.

> Remarques : les valeurs sont relayées telles quelles. La conductivité est calculée en µS/cm par le Module Capteur, le niveau est une distance capteur → surface (et non un pourcentage), et les valeurs d'erreur des capteurs (`-127` pour la température, `-1` pour le sonar) sont publiées sans filtrage.

### MQTT → relais : `onMqttMessage()`

| Topic             | Payload        | Action                        |
|-------------------|----------------|-------------------------------|
| `hydro1/cmd/out1` | `ON` ou `1`    | Relais 1 (GPIO 32) en marche  |
| `hydro1/cmd/out1` | autre valeur   | Relais 1 à l'arrêt            |
| `hydro1/cmd/out2` | `ON` ou `1`    | Relais 2 (GPIO 33) en marche  |
| `hydro1/cmd/out2` | autre valeur   | Relais 2 à l'arrêt            |

Tout autre topic sous `hydro1/cmd/` est ignoré.

### Topic d'état

| Topic           | Payload   | Quand                                         |
|-----------------|-----------|-----------------------------------------------|
| `hydro1/status` | `online`  | Publié (retenu) après la connexion au broker  |
| `hydro1/status` | `offline` | Publié par le broker (Last Will) à la perte de connexion |

---

## Détail des bibliothèques et fichiers

### `CANManager` — bus CAN (TWAI)
*Dossier : [`lib/CANManager`](lib/CANManager)*

Classe identique à celle du Module Capteur : encapsule le contrôleur CAN intégré de l'ESP32 (`driver/twai.h`) relié à un transceiver SN65HVD230.
- `begin(rx, tx, baudrate)` : 250 k, 500 k ou 1 Mbit/s, mode normal, aucun filtre (toutes les trames sont acceptées).
- `receive(msg)` : lecture non bloquante d'une trame dans la file du driver.
- `decodeBool` (1 octet), `decodeFloat` (4 octets IEEE 754), `decodeInt16` / `decodeInt32` (big-endian).
- `setFilterRange()` / `setFilterMask()` permettent un filtrage matériel (à appeler avant `begin()`), non utilisé pour l'instant.

### `MqttGateway` — client MQTT
*Dossier : [`lib/MqttGateway`](lib/MqttGateway)*

Surcouche de `PubSubClient` :
- buffer de paquet de 512 octets ;
- gestion optionnelle de l'authentification et du Last Will ;
- `connect(retries)` : tentatives espacées de 5 s (`0` = infini) ;
- `publish()` / `subscribe()` / `unsubscribe()` avec logs série ;
- `loop()` : reconnexion non bloquante puis `PubSubClient::loop()` ;
- callback utilisateur de type `std::function<void(const String&, const String&)>`, relayé par un callback statique (une seule instance supportée).

> `PubSubClient` ne publie qu'en QoS 0 : le paramètre `qos` de `publish()` est ignoré.

### `RelayController` — sorties relais
*Dossier : [`lib/RelayController`](lib/RelayController)*

- Relais **actifs à l'état bas** (`LOW` = relais enclenché).
- `begin()` configure la broche en sortie et coupe le relais.
- `turnOn()`, `turnOff()`, `toggle()`, `pulse(ms)` (bloquant), `isOn()`, `getStateString()`.

### `WifiHelper.h` — connexion Wi-Fi
*Fichier : [`src/WifiHelper.h`](src/WifiHelper.h)* (en-tête seul, inclus uniquement par `main.cpp`)

- `connectToWiFi(ssid, password)` : connexion en mode station, attend la connexion (bloquant) puis affiche l'IP et le canal.
- `handleWiFiReconnect()` : à appeler dans `loop()`, relance la connexion au plus toutes les 5 s si elle est perdue. Comme `connectToWiFi()` est bloquant, la boucle principale (MQTT et CAN) est suspendue jusqu'au retour du Wi-Fi.

---

## Configuration — [`src/Config.h`](src/Config.h)

| Constante                 | Valeur par défaut | Rôle                                   |
|---------------------------|-------------------|----------------------------------------|
| `CONFIG_PIN_CAN_TX`       | 17                | TX vers le transceiver CAN             |
| `CONFIG_PIN_CAN_RX`       | 16                | RX depuis le transceiver CAN           |
| `CONFIG_PIN_RELAY1`       | 32                | Relais pompe 1                         |
| `CONFIG_PIN_RELAY2`       | 33                | Relais pompe 2                         |
| `CONFIG_BAUDRATE`         | 9600              | Vitesse du port série                  |
| `CONFIG_WIFI_SSID` / `CONFIG_WIFI_PASSWORD` | —  | Identifiants Wi-Fi                     |
| `CONFIG_MQTT_BROKER`      | IP du broker      | Adresse du broker MQTT                 |
| `CONFIG_MQTT_PORT`        | 1883              | Port du broker                         |
| `CONFIG_MQTT_CLIENT`      | `ESP32-Hydro1`    | Identifiant client MQTT (unique)       |
| `CONFIG_MQTT_USER` / `CONFIG_MQTT_PASS` | —      | Authentification MQTT                  |
| `CONFIG_TOPIC_*`          | `hydro1/...`      | Topics de mesures, commandes et état   |
| `CONFIG_CAN_BAUDRATE`     | 500000            | Débit du bus CAN (bit/s)               |
| `CONFIG_CAN_ID_*`         | `0x101`–`0x104`   | Identifiants des trames de mesure      |

Le débit CAN et les identifiants doivent rester identiques à ceux du `Config.h` du Module Capteur.
⚠️ `Config.h` contient des identifiants Wi-Fi et MQTT : ne pas y committer les vraies valeurs.

Le câblage complet est décrit dans le [README principal](../README.md#cablage-1).

---

## Compilation et téléversement (PlatformIO)

Carte : `esp32dev`, framework Arduino. Bibliothèque déclarée dans `platformio.ini` (téléchargée automatiquement à la première compilation) :

```ini
monitor_speed = 9600
lib_deps =
    knolleary/PubSubClient@^2.8
```

Commandes usuelles :

```bash
pio run -t upload
```

```bash
pio device monitor
```
