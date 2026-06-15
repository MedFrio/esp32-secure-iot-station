# Schéma de câblage proposé

| Composant | Broche ESP32 | Remarque |
|---|---:|---|
| DHT22 DATA | GPIO 4 | Ajouter une résistance de pull-up 10 kΩ vers 3V3 si le module n'en intègre pas |
| DHT22 VCC | 3V3 | Alimentation capteur |
| DHT22 GND | GND | Masse commune |
| Joystick VRx | GPIO 34 | Axe horizontal : gauche 25 %, droite 75 % |
| Joystick VRy | GPIO 35 | Axe vertical : haut 50 %, bas 100 % |
| Joystick VCC | 5V si module imposé | Ne jamais envoyer 5V directement sur `VRx`/`VRy` : utiliser un diviseur de tension ou alimenter le joystick en 3V3 si possible |
| Joystick GND | GND | Masse commune |
| Bouton poussoir | GPIO 27 et GND | `INPUT_PULLUP`, appuyé quand GPIO 27 est relié à GND |
| LED blanche anode, patte longue | GPIO 33 via résistance 220 à 330 Ω | Sortie PWM |
| LED blanche cathode, patte courte | GND | Masse commune |
| Micro servo SG90 signal | GPIO 13 | Alimentation externe recommandée |
| Servo VCC | 5V externe | Masse commune obligatoire |
| Servo GND | GND commun | Masse ESP32 et alimentation servo communes |

Le montage est prévu sur breadboard. Au repos, le joystick publie `0`; les directions publient `25`, `50`, `75` ou `100` selon la direction détectée.

Attention : les entrées analogiques de l'ESP32 ne sont pas tolérantes au 5V. Si le joystick est alimenté en 5V, `VRx` et `VRy` peuvent monter à 5V. Dans ce cas, placer un diviseur de tension sur chaque axe avant `GPIO 34` et `GPIO 35`, par exemple 10 kΩ côté signal joystick et 20 kΩ côté GND pour ramener 5V vers environ 3,3V.
