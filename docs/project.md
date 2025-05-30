# Bilancia da Frigo

## Introduzione
In questo progetto descriviamo una bilancia intelligente basata su ESP32, dotata di un sensore di peso e di un lettore di codici a barre. La bilancia è in grado di misurare la massa di un oggetto, leggere il codice a barre per identificarlo e, eventualmente, inviare notifiche o memorizzare i dati su un servizio esterno.

## Hardware
Componenti principali:
- **ESP32**: MCU principale che gestisce la logica, la comunicazione e l’elaborazione dei dati. NodeMCU-32 board ESP32 WROOM-32 
- **Cella di carico**: connesso all’ESP32 per la misura del carico tramite un regolatore HX711, convertitore analogico-digitale (ADC) a 24 bit. Questo consente di dividere il segnale di tensione della cella di carico in 2^24 (circa 16 milioni) di passi, fornendo una risoluzione estremamente alta e permettendo di misurare cambiamenti molto piccoli nel peso.
- **Lettore di codici a barre**: consente di riconoscere automaticamente il prodotto appoggiato sulla bilancia. MH-ET LIVE basato su GM65. 
- **Circuito di carica**: composto da un TP4056 e una batteria LiPo 3,7v 1000mAh. Tra il TP4056 e ESP32 (Vin 5v) abbiamo inserito un regolatore Step-up (Pololu 5V Step-Up/Step-Down Voltage Regulator S7V8F5). TP4056 gestisce la carica della batteria e invia al regolatore ESP32 il voltaggio corretto di alimentazione 
- **Schermo OLED**: utilizzato per messaggi relativi all'oggetto, peso, comunicazioni o eventuali errori. OLED 1.3" da AZ con connessione IIC

## Altre componenti
- Piatti in plexiglass 3mm per 20cm di diametro
- Rivetti da incollare nel plexiglass
- Bulloni senza testa per fissare la cella di carico ai piatti in plexiglass

## Schema Sistema
![Immagine](imgs/test_battery_bb.png) 
![Circuito](imgs/test_battery_schem.png)

## Funzionamento
1. **Identificazione dell’oggetto**: Viene eseguita la scansione del codice a barre per recuperare informazioni sul prodotto. Il codice GTIN rilevato veine inviato a "https://world.openfoodfacts.org/api/v0/product/" che restituisce i dati sotoforma di file JSON. Registriamo il peso previsto o il volume (500ml, 1000ml) e calcoliamo le soglie 50% o 20% ecc... 
2. **Lettura del peso**: L’ESP32 acquisisce i segnali dal sensore di peso. L'oggetto viene appoggiato sulla piattaforma di peso. Appena il peso si stabilizza viene elaborato l'algoritmo di calcolo delle soglie.
3. **Elaborazione dei dati**: L’ESP32 elabora peso e codice, eseguendo calcoli e memorizzando i risultati. Quando la precentuale di prodotto residuo si attesta al di sotto di una soglia prestabilita viene inviato un messaggio al server MQTT (...o altro servizio).
4. **Notifica o interfacciamento**: ESP32 si collega a un server https di openfoodfacts.org che accetta richieste con il codice GTIN del prodotto letto dal sensore 1D/2D. Il server restituisce dati del prodotto in formato JSON.
Questi dati possono essere utilizzati per calcolare il valore peso soglia sotto il quale viene inviato un mesaggio al servere MQTT (...)

## Librerie Utilizzate
- **WiFi.h**: Per connesione WiFI
- **WiFiClientSecure.h**: Include WiFi library per connesione https
- **Wire**
- **U8g2lib**: Per la gestione del modulo OLED
- **HTTPClient.h**: Collegamento con server http per scaricare i file JSON
- **ArduinoJson.h**: Per il parsing del file JSON

## Librerie  create (non trovate alternative)
- display_manager.h // Gestione più semplice dei font per il testo da inviare all'OLED
- led_rgb_controller.h // Gestione e test componente SMD RGB e attivare colori base chiamando una semplice funzione 

## Conclusioni
Il sistema illustrato rappresenta un prototipo di bilancia “smart” che può essere impiegata in contesti domestici per esempio in frigo dove appoggiando un prodotto saremo automaticamente avvertiti se la quantità residua del prodotto è sotto una certa soglia e quindi vine e richiesto il nuvo approviggionamento dello stesso prodotto.

