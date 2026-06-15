flowchart LR
    subgraph ESP32["ESP32, firmware modulaire"]
        SENS["sensors/\nDHT22, joystick, bouton poussoir\nfiltrage EMA, anomalies, timestamp"]
        ACT["actuators/\nLED blanche GPIO33, micro servo SG90"]
        NET["network/\nWiFi, MQTT QoS 1,\nreconnexion, commandes"]
        STO["storage/\nSPIFFS offline JSONL\nNVS Preferences"]
        WEB["web/\nindex.html, app.js, style.css\nAPI locale protégée"]
        SEC["security/\nvalidation JSON\ntoken API\nauth MQTT"]
        SUP["supervision\nheap, uptime, latence"]

        SENS -->|"Queue FreeRTOS"| NET
        NET -->|"commande validée"| SEC
        SEC --> ACT
        WEB --> SEC
        WEB --> ACT
        NET --> STO
        STO --> NET
        SUP --> WEB
        SUP --> NET
    end

    MQTT["Broker MQTT Mosquitto\nauthentification"]
    NR["Node-RED\nréception, dashboard,\ncommandes"]
    MONGO["MongoDB\nhistorisation NoSQL"]
    INFLUX["InfluxDB\nbase temporelle"]
    GRAF["Grafana\ndashboard et alerte"]

    NET -->|"campus/groupe/device/data"| MQTT
    MQTT --> NR
    NR --> MONGO
    NR --> INFLUX
    INFLUX --> GRAF
    NR -->|"campus/groupe/device/cmd"| MQTT
    MQTT --> NET
