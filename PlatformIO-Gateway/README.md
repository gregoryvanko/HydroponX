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
├── platformio.ini              Carte, framework, vitesse série, dépendances, env OTA
├── secrets.ini.example         Modèle de secrets.ini (mot de passe et adresse OTA, ignoré par git)
├── src/
│   ├── main.cpp                Programme principal : setup() / loop(), routage CAN ↔ MQTT
│   └── Config.h                Nom d'appareil, broches, topics MQTT, débit et IDs CAN
└── lib/                        Bibliothèques locales (une classe par dossier)
    ├── CANManager/             CANManager.h / .cpp        → bus CAN (TWAI)
    ├── ESPConnect/             ESPConnect.h / .cpp        → Wi-Fi (portail captif) + client MQTT
    └── RelayController/        RelayController.h / .cpp   → sorties relais
```

Chaque dossier de `lib/` est compilé par PlatformIO comme une bibliothèque indépendante : son en-tête est trouvé automatiquement quand `main.cpp` fait `#include "<Nom>.h"`, sans réglage dans `platformio.ini`.
Les dépendances d'ESPConnect (AsyncTCP, ESPAsyncWebServer) sont téléchargées via `lib_deps`.

---

## Vue d'ensemble

```
                      ┌─────────────── ESP32 Gateway ───────────────┐
 Bus CAN ──► Transceiver ──► CANManager ──► traiterCanMsg() ──► ESPConnect ──(Wi-Fi)──► Broker MQTT
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
2. **Relais** : configure les deux sorties et met les relais à l'arrêt.
3. **CAN** : démarre le bus à 500 kbit/s sur RX = GPIO 16, TX = GPIO 17. En cas d'échec, le programme se bloque dans une boucle infinie.
4. **Wi-Fi + MQTT** (ESPConnect) : enregistre le callback `onMqttMessage`, déclare la souscription à `hydro1/cmd/#` (QoS 1, renouvelée à chaque reconnexion) puis démarre ESPConnect avec le nom d'appareil `hydro1`. Le démarrage n'est **pas bloquant**.

### `loop()`
La boucle tourne en continu, sans délai :

1. `espConnect.loop()` : connexion et reconnexion Wi-Fi, client MQTT, bouton de réinitialisation et appel des callbacks.
2. `can.receive()` (non bloquant) : si une trame CAN est disponible, elle est passée à `traiterCanMsg()`.

### Configuration Wi-Fi et MQTT
Les identifiants ne sont plus dans le code : ils sont saisis depuis le navigateur et stockés en flash (NVS).

1. **Premier démarrage** : l'ESP32 ouvre le point d'accès ouvert `HydroponX-Setup`. S'y connecter, puis choisir le réseau Wi-Fi sur la page qui s'ouvre (ou http://192.168.1.1).
2. **MQTT** : une fois l'ESP32 connecté au Wi-Fi, ouvrir `http://<ip-esp32>/status`, cocher *Activer la connexion MQTT* et saisir le broker (`mqtt://`, port 1883), l'utilisateur et le mot de passe.
3. **Réinitialiser le Wi-Fi** : maintenir le bouton BOOT (GPIO 0) 2 s.

### Mise à jour par le réseau (OTA)
`espConnect.enableOTA()` active ArduinoOTA (port 3232, protégé par mot de passe) dès que le Wi-Fi est connecté. Il n'est pas disponible sur le portail de configuration. Au début d'un téléversement, l'ESP32 publie `offline`, puis redémarre sur le nouveau firmware.

Voir le [README d'ESPConnect](lib/ESPConnect/README.md) pour le détail.

### CAN → MQTT : `traiterCanMsg()`
L'identifiant de la trame détermine son décodage et le topic de publication :

| ID CAN  | Décodage      | Topic MQTT            | Payload publié              |
|---------|---------------|-----------------------|-----------------------------|
| `0x101` | `decodeFloat` | `hydro1/waterTemp`    | température, 2 décimales    |
| `0x102` | `decodeFloat` | `hydro1/waterEc`      | conductivité, 2 décimales   |
| `0x103` | `decodeFloat` | `hydro1/waterLevel`   | distance en cm, 2 décimales |
| `0x104` | `decodeBool`  | `hydro1/waterPresent` | `1` ou `0`                  |

Les messages sont publiés en QoS 0, non retenus, uniquement si le broker est connecté. Un ID inconnu est simplement signalé sur le port série.

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
| `hydro1/status` | `online`  | Publié (retenu) à la connexion au broker puis toutes les 60 s |
| `hydro1/status` | `offline` | Publié par le broker (Last Will) à la perte de connexion |

Ce topic est géré par ESPConnect : `<nom d'appareil>/status`.

---

## Détail des bibliothèques et fichiers

### `CANManager` — bus CAN (TWAI)
*Dossier : [`lib/CANManager`](lib/CANManager)*

Classe identique à celle du Module Capteur : encapsule le contrôleur CAN intégré de l'ESP32 (`driver/twai.h`) relié à un transceiver SN65HVD230.
- `begin(rx, tx, baudrate)` : 250 k, 500 k ou 1 Mbit/s, mode normal, aucun filtre (toutes les trames sont acceptées).
- `receive(msg)` : lecture non bloquante d'une trame dans la file du driver.
- `decodeBool` (1 octet), `decodeFloat` (4 octets IEEE 754), `decodeInt16` / `decodeInt32` (big-endian).
- `setFilterRange()` / `setFilterMask()` permettent un filtrage matériel (à appeler avant `begin()`), non utilisé pour l'instant.

### `ESPConnect` — Wi-Fi et MQTT
*Dossier : [`lib/ESPConnect`](lib/ESPConnect)* — voir son [README](lib/ESPConnect/README.md).

- portail captif de configuration Wi-Fi, identifiants en NVS, reconnexion automatique ;
- page `/status` (état, configuration MQTT à chaud) ;
- client MQTT `esp-mqtt` (TCP/TLS/WS/WSS), statut `online`/`offline` avec Last Will ;
- souscriptions mémorisées et renouvelées à chaque reconnexion ; `publish()` avec surcharges (`float` → 2 décimales).

### `RelayController` — sorties relais
*Dossier : [`lib/RelayController`](lib/RelayController)*

- Relais **actifs à l'état bas** (`LOW` = relais enclenché).
- `begin()` configure la broche en sortie et coupe le relais.
- `turnOn()`, `turnOff()`, `toggle()`, `pulse(ms)` (bloquant), `isOn()`, `getStateString()`.

---

## Configuration — [`src/Config.h`](src/Config.h)

| Constante                 | Valeur par défaut | Rôle                                   |
|---------------------------|-------------------|----------------------------------------|
| `CONFIG_DEVICE_NAME`      | `hydro1`          | Nom d'appareil (hostname, client MQTT, topic de statut) |
| `CONFIG_AP_NAME`          | `HydroponX-Setup` | Point d'accès du portail de configuration |
| `CONFIG_OTA_PASSWORD`     | `secrets.ini`     | Mot de passe OTA (injecté par `build_flags`) |
| `CONFIG_OTA_PORT`         | 3232              | Port OTA                               |
| `CONFIG_PIN_CAN_TX`       | 17                | TX vers le transceiver CAN             |
| `CONFIG_PIN_CAN_RX`       | 16                | RX depuis le transceiver CAN           |
| `CONFIG_PIN_RELAY1`       | 32                | Relais pompe 1                         |
| `CONFIG_PIN_RELAY2`       | 33                | Relais pompe 2                         |
| `CONFIG_BAUDRATE`         | 9600              | Vitesse du port série                  |
| `CONFIG_TOPIC_*`          | `hydro1/...`      | Topics de mesures et de commandes      |
| `CONFIG_CAN_BAUDRATE`     | 500000            | Débit du bus CAN (bit/s)               |
| `CONFIG_CAN_ID_*`         | `0x101`–`0x104`   | Identifiants des trames de mesure      |

Le débit CAN et les identifiants doivent rester identiques à ceux du `Config.h` du Module Capteur.

Le câblage complet est décrit dans le [README principal](../README.md#cablage-1).

---

## Compilation et téléversement (PlatformIO)

Carte : `esp32dev`, framework Arduino. Bibliothèques déclarées dans `platformio.ini` (téléchargée automatiquement à la première compilation) :

```ini
monitor_speed = 9600
lib_deps =
    ESP32Async/AsyncTCP @ 3.5.0
    ESP32Async/ESPAsyncWebServer @ 3.12.0
```

### Secrets (obligatoire)
Le mot de passe OTA n'est pas dans le code. Créer `secrets.ini` (ignoré par git) à partir du modèle, puis le renseigner :

```bash
cp secrets.ini.example secrets.ini
```

```ini
[secrets]
ota_password = mon-mot-de-passe   ; compilé dans le firmware et utilisé pour le téléversement
ota_host = 192.168.1.50           ; IP de l'ESP32 (visible sur /status) ou son nom DNS
```

Sans ce fichier, PlatformIO s'arrête avec l'erreur `No section: 'secrets'`.

### Commandes usuelles
Premier flash par USB :

```bash
pio run -t upload
```

```bash
pio device monitor
```

Mises à jour suivantes par le réseau (OTA) :

```bash
pio run -e esp32dev_ota -t upload
```

Le mDNS n'est pas activé : `ota_host` doit être l'adresse IP de l'ESP32 ou le nom que lui donne le DNS du routeur (`hydro1`).
