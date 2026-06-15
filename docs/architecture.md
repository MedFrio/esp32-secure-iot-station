# Architecture globale du système IoT sécurisé

Le système IoT repose sur un ESP32 organisé en modules logiciels distincts. Chaque module prend en charge une responsabilité précise : acquisition des données, pilotage des actionneurs, communication réseau, stockage local, interface web, sécurité et supervision.

## 1. Architecture interne de l'ESP32

| Module         | Rôle                                           | Éléments principaux                                                                    |
| -------------- | ---------------------------------------------- | -------------------------------------------------------------------------------------- |
| `sensors/`     | Acquisition et traitement des données capteurs | DHT22, joystick, bouton poussoir, filtrage EMA, détection d'anomalies, timestamp       |
| `actuators/`   | Pilotage des actionneurs locaux                | LED blanche sur GPIO33, micro servo SG90                                               |
| `network/`     | Communication réseau                           | WiFi, MQTT QoS 1, reconnexion automatique, réception des commandes                     |
| `storage/`     | Persistance locale                             | SPIFFS pour les données offline au format JSONL, NVS Preferences pour la configuration |
| `web/`         | Interface locale embarquée                     | `index.html`, `app.js`, `style.css`, API locale protégée                               |
| `security/`    | Sécurisation des échanges et commandes         | Validation JSON, token API, authentification MQTT                                      |
| `supervision/` | Suivi de l'état du système                     | Heap, uptime, latence, état réseau                                                     |

## 2. Flux internes de l'ESP32

| Source         | Destination  | Description                                              |
| -------------- | ------------ | -------------------------------------------------------- |
| `sensors/`     | `network/`   | Envoi des mesures via une queue FreeRTOS                 |
| `network/`     | `security/`  | Transmission des commandes reçues pour validation        |
| `security/`    | `actuators/` | Exécution des commandes validées                         |
| `web/`         | `security/`  | Appels à l'API locale protégée                           |
| `web/`         | `actuators/` | Pilotage local des actionneurs depuis l'interface web    |
| `network/`     | `storage/`   | Stockage temporaire des données en cas de coupure réseau |
| `storage/`     | `network/`   | Renvoi des données stockées après reconnexion            |
| `supervision/` | `web/`       | Affichage local des informations système                 |
| `supervision/` | `network/`   | Publication des métriques de supervision                 |

## 3. Services externes

| Composant             | Rôle                                                                   |
| --------------------- | ---------------------------------------------------------------------- |
| Broker MQTT Mosquitto | Réception des données de l'ESP32 et diffusion des commandes            |
| Node-RED              | Orchestration des flux, réception des messages, dashboard et commandes |
| MongoDB               | Historisation NoSQL des données                                        |
| InfluxDB              | Stockage temporel des mesures                                          |
| Grafana               | Visualisation des données, tableaux de bord et alertes                 |

## 4. Flux de données externe

| Source    | Destination | Topic ou usage                                           |
| --------- | ----------- | -------------------------------------------------------- |
| ESP32     | Mosquitto   | Publication des données sur `campus/groupe/device/data`  |
| Mosquitto | Node-RED    | Transmission des messages MQTT reçus                     |
| Node-RED  | MongoDB     | Historisation NoSQL                                      |
| Node-RED  | InfluxDB    | Stockage des séries temporelles                          |
| InfluxDB  | Grafana     | Visualisation des mesures                                |
| Node-RED  | Mosquitto   | Publication des commandes sur `campus/groupe/device/cmd` |
| Mosquitto | ESP32       | Réception des commandes par le firmware                  |

## 5. Résumé du fonctionnement

L'ESP32 collecte les données issues des capteurs, applique un filtrage léger et détecte les anomalies éventuelles. Les mesures sont ensuite envoyées au broker MQTT avec un niveau de qualité de service adapté. En cas de perte de connexion, les données sont conservées localement dans le système de fichiers SPIFFS au format JSONL, puis renvoyées automatiquement lorsque la connexion est rétablie.

Les commandes sont émises depuis Node-RED ou depuis l'interface web locale. Avant toute exécution, elles passent par un module de sécurité chargé de vérifier leur validité, leur format JSON et leur autorisation. Les actionneurs, comme la LED blanche et le micro servo, ne sont donc pilotés qu'après validation.

La supervision permet de suivre l'état du système, notamment la mémoire disponible, le temps de fonctionnement, la latence et l'état réseau. Les données sont historisées dans MongoDB et InfluxDB, puis visualisées dans Grafana afin de faciliter l'analyse et la détection d'incidents.
