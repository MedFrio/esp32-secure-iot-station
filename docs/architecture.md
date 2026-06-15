flowchart LR
    subgraph ESP32["ESP32, firmware modulaire"]
        SENS["sensors/<br/>DHT22, joystick, bouton poussoir<br/>filtrage EMA, anomalies, timestamp"]
        ACT["actuators/<br/>LED blanche GPIO33, micro servo SG90"]
        NET["network/<br/>WiFi, MQTT QoS 1<br/>reconnexion, commandes"]
        STO["storage/<br/>SPIFFS offline JSONL<br/>NVS Preferences"]
        WEB["web/<br/>index.html, app.js, style.css<br/>API locale protégée"]
        SEC["security/<br/>validation JSON<br/>token API<br/>auth MQTT"]
        SUP["supervision<br/>heap, uptime, latence"]

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

    MQTT["Broker MQTT Mosquitto<br/>authentification"]
    NR["Node-RED<br/>réception, dashboard<br/>commandes"]
    MONGO["MongoDB<br/>historisation NoSQL"]
    INFLUX["InfluxDB<br/>base temporelle"]
    GRAF["Grafana<br/>dashboard et alerte"]

    NET -->|"campus/groupe/device/data"| MQTT
    MQTT --> NR
    NR --> MONGO
    NR --> INFLUX
    INFLUX --> GRAF
    NR -->|"campus/groupe/device/cmd"| MQTT
    MQTT --> NET
