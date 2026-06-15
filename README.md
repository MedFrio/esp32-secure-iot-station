# Système IoT sécurisé et autonome, ESP32

## Choix matériels

| Type | Composant | Rôle |
|---|---|---|
| Capteur environnement | DHT22 | Température et humidité |
| Capteur interaction | Joystick | Valeur locale : gauche 25 %, haut 50 %, droite 75 %, bas 100 % |
| Capteur interaction | Bouton poussoir | Entrée locale câblée en `INPUT_PULLUP` vers GND |
| Actionneur simple | LED blanche sur GPIO 33 | Indication visuelle et commande locale distante |
| Actionneur simple | Micro servo SG90 | Actionneur commandable via Web ou MQTT |


## Architecture

```text
ESP32
├── Task acquisition capteurs
├── Task communication réseau et MQTT
├── Task serveur web embarqué
├── Task supervision système
├── SPIFFS, fichiers web et file offline JSONL
└── NVS Preferences, configuration MQTT et API token
```

Voir `docs/architecture.mmd`.

## Démarrage rapide

1. Installer PlatformIO.
2. Copier `src/network/Secrets.example.h` vers `src/network/Secrets.h`.
3. Renseigner le WiFi, le broker MQTT, les identifiants et le token API local.
4. Compiler et téléverser le firmware.
5. Téléverser le système de fichiers SPIFFS, dossier `data`.
6. Démarrer l'infrastructure locale si nécessaire.

```bash
pio run
pio run -t upload
pio run -t uploadfs
pio device monitor
```

## Infrastructure locale

L'infrastructure de démonstration est fournie dans `infra/`.

```bash
cd infra
cp .env.example .env
docker compose up -d
```

Générer le mot de passe Mosquitto au premier lancement :

```bash
docker compose exec mosquitto mosquitto_passwd -c -b /mosquitto/config/passwords iot_user change_me_password
docker compose restart mosquitto
```

Importer ensuite `node-red/flows.json` dans Node-RED. L'image Node-RED personnalisée installe les noeuds MongoDB et dashboard classiques.

## Topics MQTT

Données :

```text
campus/<groupe>/<deviceID>/data
```

Commandes :

```text
campus/<groupe>/<deviceID>/cmd
```

Exemple commande MQTT :

```json
{
  "apiToken": "changeme-local-api",
  "led": {"enabled": true, "brightness": 180},
  "servo": {"angle": 90}
}
```

## API locale

L'API locale est protégée par le header `X-API-Key`.

| Méthode | Route | Description |
|---|---|---|
| GET | `/api/state` | Etat capteurs, actionneurs, réseau, métriques |
| GET | `/api/config` | Configuration MQTT courante, sans mot de passe |
| POST | `/api/config` | Mise à jour configuration MQTT |
| POST | `/api/actuators` | Commande LED blanche et servo |

## Livrables

| Livrable demandé | Emplacement |
|---|---|
| Code source structuré | `src/` |
| Interface Web embarquée | `data/index.html`, `data/app.js`, `data/style.css` |
| Diagramme architecture | `docs/architecture.mmd` |
| Dashboard Node-RED | `node-red/flows.json` |
| Bonus Grafana | `infra/grafana/dashboards/iot-station-dashboard.json` |

Rapport technique en PDF (envoyé par mail)

## Notes de conception

La file offline est stockée dans `/offline.jsonl` sur SPIFFS. Chaque ligne est un JSON valide respectant le format imposé, avec des champs additionnels pour les capteurs et métriques. Dès que MQTT revient, la task réseau rejoue automatiquement la file.
