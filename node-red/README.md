# Node-RED

Importer `flows.json` depuis l'interface Node-RED.

L'image Docker personnalisée installe :
- `node-red-node-mongodb`, pour stocker les données dans MongoDB,
- `node-red-dashboard`, pour afficher un dashboard Node-RED local.

Le flow fourni :
- écoute `campus/+/+/data` en QoS 1,
- valide le payload,
- écrit les documents dans MongoDB,
- écrit les mesures dans InfluxDB pour Grafana,
- affiche température, humidité, joystick, bouton poussoir, heap, latence et anomalie,
- permet d'envoyer une commande MQTT de démonstration vers l'ESP32.
