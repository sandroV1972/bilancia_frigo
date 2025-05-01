# Bilancia da Frigo
### Progetto per il corso di Sistemi Embedded UniMI 2025

La bilancia è un oggetto piatto che può alloggiare un Tetrapack  quadrato. Lo strumento sarà in grado di pesare un Tetrapack pieno di liquido come latte, succhi, bianco d'uovo ecc.. sia da 500ml che da 1lt. Una bilancia deve essere attivata con un interruttore e facendo la scansione del QR code sul contenitore o con una etichetta RF incollata al contenitore. Se viene fatta la scansione la bilancia si tara su contenuto e volume, quando viene appoggiato il contenitore calcola il contenuto residuo in base al peso e decide se inviare un messaggio (...da decidere come...) che avverte che il contenitore è quasi vuote e serve acquistarlo nuovamente. Il sistema utilizzerà ESP32, alimentato da una batteria (con un TP4056) e un regolatore di voltaggio. I sensori saranno: sensore di pressione, un lettore RFID (se implementiamo RFID), un lettore/scanner di codici 1D 2D. Verra incluso un interruttore, un tasto per inizializzare e leggere il codice del cartone. Quando il peso del cartone appoggiato scende sotto una soglia pre-impostata viene inviato un messaggio (da decidere come e dove). Il firmware si occupa di registrare la lettura del codice 1D/2D, di calcolare il liquido residuo e di inviare eventualmente i messaggi di avviso.

## Componenti Hardware

### Main Board
- ESP32-WROOM32  MCU NodeMCU-32S
### Alimentazione
- TP4056
- Batteria Li-Ion 3.7v 1000mA
### Sensori
- Lettore 1D 2D basato su GM65
- ~~Sensore di pressione (peso)~~ [sensore di pressione troppo impreciso e con risposte randomiche]
- Cella di carico con amplificatore HX711
### Attuatori 
- Schermo OLED 1.3 pollici I2C
- LED:
  - blu (comunicazione wifi attiva)
  - verde (prodotto memorizzato)
  - giallo (nessun prodotto memorizzato)
  - giallo lampeggiante (lettore QR attivo)
### Sistemi di Comunicazione
- HTTP a server @openfoodfacts.org/api/v0/ per riconoscimento oggetti da pesare passando il codice letto (GTIN)
- MQQT trasmissione risultati pesate se al di sotto dei livelli minimi consentiti
