# HydroponX — Module Capteur (ESP32)

Firmware de l'ESP32 « Module Capteur » du système hydroponique HydroponX.
Ce module mesure l'état de l'eau du réservoir et envoie ses mesures toutes les minutes sur le **bus CAN**, où elles sont reprises par le [Module Gateway](../PlatformIO-Gateway/README.md) puis publiées sur MQTT.

> Code migré depuis le projet Arduino IDE [`SRC/Module-Capteur`](../SRC/Module-Capteur).

---

## Structure du projet

```
PlatformIO-Capteur/
├── platformio.ini              Carte, framework, vitesse série et dépendances externes
├── src/
│   ├── main.cpp                Programme principal : setup() / loop()
│   └── Config.h                Broches, intervalle de mesure, débit et IDs CAN
└── lib/                        Bibliothèques locales (une classe par dossier)
    ├── CANManager/             CANManager.h / .cpp          → bus CAN (TWAI)
    ├── SensorTemperature/      SensorTemperature.h / .cpp   → DS18B20
    ├── SensorTDS/              SensorTDS.h / .cpp           → conductivité (sonde TDS)
    ├── SensorSonar/            SensorSonar.h / .cpp         → niveau d'eau (JSN-SR04T)
    └── SensorWaterPresent/     SensorWaterPresent.h / .cpp  → détection de retour d'eau
```

Chaque dossier de `lib/` est compilé par PlatformIO comme une bibliothèque indépendante : son en-tête est trouvé automatiquement quand `main.cpp` fait `#include "<Nom>.h"`, sans réglage dans `platformio.ini`.
Les bibliothèques externes (OneWire, DallasTemperature) sont téléchargées via `lib_deps`.

---

## Vue d'ensemble

```
 DS18B20 ──(OneWire)──┐
 Sonde TDS ──(ADC)────┤
 JSN-SR04T ─(Trig/Echo)┤──► ESP32 ──(TWAI)──► Transceiver CAN ──► Bus CAN ──► Module Gateway
 Présence eau ─(GPIO)─┘
```

| Grandeur mesurée        | Capteur              | Classe               | ID CAN  | Type CAN |
|-------------------------|----------------------|----------------------|---------|----------|
| Température de l'eau    | DS18B20              | `SensorTemperature`  | `0x101` | float    |
| Conductivité (EC)       | Sonde TDS CQRobot    | `SensorTDS`          | `0x102` | float    |
| Niveau d'eau (distance) | JSN-SR04T (ultrason) | `SensorSonar`        | `0x103` | float    |
| Retour d'eau présent    | Capteur eau CQRobot  | `SensorWaterPresent` | `0x104` | bool     |

---

## Fonctionnement du programme

### `setup()`
1. Ouvre le port série (`CONFIG_BAUDRATE` = 9600 bauds) pour les logs.
2. Initialise chaque capteur via sa méthode `begin()` (configuration des GPIO / ADC, détection du DS18B20).
3. Démarre le bus CAN à 500 kbit/s (`CONFIG_CAN_BAUDRATE`) sur RX = GPIO 16, TX = GPIO 17.
   En cas d'échec, le programme se bloque dans une boucle infinie (le message `[Setup] ERREUR` est affiché sur le port série).

### `loop()`
La boucle est cadencée par `millis()` (non bloquante entre deux cycles) : toutes les `CONFIG_MESURE_INTERVAL` ms (60 000 ms = 1 min) :

1. **Lecture des capteurs**
   - température de l'eau (DS18B20) ;
   - présence d'eau (entrée numérique) ;
   - distance jusqu'à la surface de l'eau : médiane de 9 mesures ultrason ;
   - conductivité, compensée avec la température qui vient d'être mesurée.
2. **Envoi sur le bus CAN** de 4 trames successives, espacées de 10 ms.
3. **Log série** récapitulatif, par ex. :
   `Send CAN => Temp=22.50°C  EC=1850.00µS/cm  Niveau=34.2cm  Flux=OUI`

Un cycle complet dure environ 2 secondes (≈ 750 ms pour la conversion DS18B20, 20 × 40 ms pour le moyennage TDS, 9 × 30 ms pour le sonar).

---

## Détail des bibliothèques (`lib/`)

### `SensorTemperature` — DS18B20
*Dossier : [`lib/SensorTemperature`](lib/SensorTemperature)*
- Utilise les bibliothèques externes `OneWire` et `DallasTemperature`.
- `begin()` signale sur le port série si aucun capteur n'est détecté sur le bus OneWire.
- `read()` lance une conversion et renvoie la température (°C) du premier capteur.
  En cas d'erreur, la valeur `DEVICE_DISCONNECTED_C` (**-127 °C**) est renvoyée telle quelle et sera transmise sur le CAN.

### `SensorTDS` — conductivité (sonde TDS CQRobot)
*Dossier : [`lib/SensorTDS`](lib/SensorTDS)*
- ADC 12 bits, atténuation 11 dB (plage 0–3,3 V), référence 3,3 V.
- `readVoltage()` : moyenne de 20 lectures espacées de 40 ms.
- `readTDS(t)` : compensation thermique `k = 1 + 0,02 × (t − 25)`, puis conversion tension → TDS (ppm) par le polynôme CQRobot :
  `TDS = (133,42·V³ − 255,86·V² + 857,39·V) × 0,5`
- `readEC(t)` : `EC ≈ TDS × 2`, exprimée en **µS/cm**.
- Les paramètres par défaut (`EC_VREF`, `EC_ADC_RESOLUTION`, `EC_NUM_SAMPLES`, `EC_SAMPLE_INTERVAL_MS`) peuvent être redéfinis par `#define` (ou `build_flags` PlatformIO).

### `SensorSonar` — JSN-SR04T
*Dossier : [`lib/SensorSonar`](lib/SensorSonar)*
- `measureCm()` : impulsion TRIG de 10 µs, mesure de la durée ECHO avec `pulseIn()` (timeout 30 ms), conversion avec une vitesse du son de 0,034329 cm/µs (20 °C).
  Renvoie **-1** en cas de timeout ou si la distance dépasse 400 cm.
- `measureMedianCm(n, delayMs)` : réalise entre 3 et 9 mesures (30 ms d'écart par défaut), écarte les mesures invalides et renvoie la médiane (tri à bulles), ou -1 si toutes ont échoué.
- La valeur envoyée sur le CAN est la **distance capteur → surface de l'eau, en cm** (pas un pourcentage de remplissage).

### `SensorWaterPresent` — détection de retour d'eau
*Dossier : [`lib/SensorWaterPresent`](lib/SensorWaterPresent)*
- Entrée numérique simple (pull-up interne désactivé dans `src/main.cpp`).
- `read()` renvoie `true` si la broche est à l'état `HIGH`.

### `CANManager` — bus CAN (TWAI)
*Dossier : [`lib/CANManager`](lib/CANManager)*
Encapsule le contrôleur CAN intégré de l'ESP32 (driver `driver/twai.h`), utilisé avec un transceiver SN65HVD230.
- `begin(rx, tx, baudrate)` : installe et démarre le driver (250 k, 500 k ou 1 Mbit/s ; mode normal, sans filtre par défaut).
- Envoi : `sendBool` (1 octet : `0x00`/`0x01`), `sendFloat` (4 octets IEEE 754, ordre mémoire little-endian de l'ESP32), `sendInt16` / `sendInt32` (big-endian). Timeout d'émission : 10 ms.
- Réception et décodage (`receive`, `decodeBool`, `decodeFloat`, …) et filtres matériels (`setFilterRange`, `setFilterMask`, à appeler avant `begin()`) : non utilisés par ce module mais partagés avec le Gateway.
- Les identifiants sont au format standard 11 bits.

---

## Configuration — [`src/Config.h`](src/Config.h)

| Constante                    | Valeur    | Rôle                                 |
|------------------------------|-----------|--------------------------------------|
| `CONFIG_PIN_DS18B20`         | 4         | Data OneWire DS18B20                 |
| `CONFIG_PIN_WaterPres`       | 35        | Capteur de présence d'eau            |
| `CONFIG_PIN_TRIG`            | 5         | TRIG du JSN-SR04T                    |
| `CONFIG_PIN_ECHO`            | 18        | ECHO du JSN-SR04T (via diviseur 5 V → 3,3 V) |
| `CONFIG_PIN_TDS`             | 34        | Sortie analogique de la sonde TDS    |
| `CONFIG_PIN_CAN_TX`          | 17        | TX vers le transceiver CAN           |
| `CONFIG_PIN_CAN_RX`          | 16        | RX depuis le transceiver CAN         |
| `CONFIG_BAUDRATE`            | 9600      | Vitesse du port série                |
| `CONFIG_MESURE_INTERVAL`     | 60000     | Période de mesure (ms)               |
| `CONFIG_CAN_BAUDRATE`        | 500000    | Débit du bus CAN (bit/s)             |
| `CONFIG_CAN_ID_waterTemp`    | `0x101`   | ID CAN température                   |
| `CONFIG_CAN_ID_waterEc`      | `0x102`   | ID CAN conductivité                  |
| `CONFIG_CAN_ID_waterLevel`   | `0x103`   | ID CAN niveau                        |
| `CONFIG_CAN_ID_waterPresent` | `0x104`   | ID CAN présence d'eau                |

Le débit CAN et les identifiants doivent rester identiques à ceux du `Config.h` du Module Gateway.
Le câblage complet est décrit dans le [README principal](../README.md#cablage).

---

## Compilation et téléversement (PlatformIO)

Carte : `esp32dev`, framework Arduino. Bibliothèques déclarées dans `platformio.ini` (téléchargées automatiquement à la première compilation) :

```ini
monitor_speed = 9600
lib_deps =
    paulstoffregen/OneWire@^2.3.8
    milesburton/DallasTemperature@^4.0.4
```

Commandes usuelles :

```bash
pio run -t upload
```

```bash
pio device monitor
```
