# AirQualityStationProject

[EN/IT]

[EN]

University internship project.

Objective: To design and build an Air Quality Monitoring Station that used LoRa to communicate with a gateway, allowing it to be placed in relatively remote locations.

Components: 
-Esp32

-Raspberry Pi 3

-MQ135 sensor

-DHT11 Temperature and humidity sensor

-PMS5003 Particulate matter sensor

-Rfm95w LoRa Chip

SENSOR NODE

The sensor node is designed around the ESP32, to which the sensors are connected. The code wrote for the node (see sensorNode.cpp) was written using PlatformIo (see platformio.ini).

The code includes the setup for the sensors, fixed geographic (randomly selected) data, currently hard-coded (as a gps component would have been an additional expense not necessary for the project's objective) and functions for extracting measurements out of the sensors. Once the measurements are extracted, they are analyzed for invalid results and, if the measurements are valid, they are sent to the gateway. If the results are invalid, an error message is sent instead. To speed up testing, the measurements are done once a minute. However, an additional function is provided to measure once every hour, as that would be a more suitable approach for non-testing situations.

Note: As a result of the chosen chip and incompatibilities with a few libraries, the LoRa communication can't actually use all the parameters available. As this was done at personal expenses, additional expenses for alternative chips were not deemed necessary, as the communication still works.

GATEWAY

Implemented using a Raspberry Pi 3. It uses a LoRa chip to receive data from the sensor node and upload it to the time-series database InfluxDB (on cloud). It splits the message and uploads the measurements to the database, which are then available to visualize using Grafana (also cloud-based). This means that all data acquired from sensor node(s) can be analyzed remotely and, thanks to the geographical data, they can be placed on a map for better understanding (e.g.: The effects of a certain event on the air quality of a certain area).

PHYSICAL DESIGN

The components were installed on two custom-designed PCBs. The gateway PCB, as opposed to the sensor node PCB, was build as a "shield" for the raspberry.

[IT]

Progetto di tirocinio universitario.

Obiettivo: Ideazione e costruzione di una stazione di monitoraggio della qualità dell'aria, con comunicazione a lunga distanza verso un gateway usando LoRa. Ciò permette al nodo sensori di essere posizionato in località relativamente remote.

Componenti:

-ESP32

-Raspberry Pi 3

-Sensore MQ135

-Sensore di temperatura e umidità DHT11

-Sensore PMS5003

-Chip LoRa Rfm95w

NODO SENSORI

Il nodo sensori è costruito attorno all'ESP32, al quale sono connessi i vari sensori. Il codice scritto per il nodo sensori (vedi sensorNode.cpp) è stato scritto utilizzando PlatformIO (vedi platformio.ini)

Il codice includo il setup per i sensori, dati geografici attualmente fissi (in quanto una componente gps sarebbe stata una spesa aggiuntiva non necessaria al progetto dato lo scopo) e funzioni per estrarre le misurazioni dai sensori. Una volta estratti, sono analizzati per validare i risultati e, se validi, questi vengono inviati al gateway. Se i risultati sono invalidi, viene inviato un messaggio di errore. Per motivi legati al testing, le misurazioni sono state raccolte una volta al minuto. Tuttavia, viene implementata una funzione aggiuntiva per la misurazione oraria, in quanto più vicina al caso d'uso classico. 

Nota: Date alcune incompatibilità con il chip e le librerie utilizzate, non possono essere sfruttati tutti i parametri della comunicazione LoRa. In quanto il progetto è stato svolto a carico personale, spese aggiuntive per ulteriori componenti non sono state considerate necessarie. La comunicazione è comunque funzionante.

GATEWAY

Implementato con una Raspberry Pi 3. Utilizza un chip LoRa per ricevere dati dal nodo sensori e carica i dati al database time-series InfluxDB (su cloud). Divide il messaggio e carica le misurazioni nel database, rendendole disponibili per la visualizzazione tramite Grafana (sempre su cloud). Questo significa che tutti i dati acquisiti dai nodi sensori possono essere visualizzati e analizzati da remoto e, grazie ai dati geografici, possono essere piazzati su una mappa per una migliore leggibilità (esempio: effetti di un certo evento sulla qualità dell'aria di una data area).

DESIGN FISICO

I componenti sono stati installati su due circuiti stampati ideati appositamente per il progeto. Il PCB del gateway, a differenza di quello del nodo sensori, è stato costruito in stile "Shield" per la Raspberry Pi 3.

<img width="214" height="335" alt="image" src="https://github.com/user-attachments/assets/0305127a-1236-4402-8d1a-81e58df1bf8a" />

<img width="502" height="407" alt="image" src="https://github.com/user-attachments/assets/a06b99c7-71fd-490c-8b5b-dbd04e51fc60" />


