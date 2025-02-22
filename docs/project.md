# Bilancia da Frigo

## Introduzione
// ...existing code...
In questo progetto descriviamo una bilancia intelligente basata su ESP32, dotata di un sensore di peso e di un lettore di codici a barre. La bilancia è in grado di misurare la massa di un oggetto, leggere il codice a barre per identificarlo e, eventualmente, inviare notifiche o memorizzare i dati su un servizio esterno.
///
/// ## Hardware
Descriviamo i componenti principali:
- **ESP32**: MCU principale che gestisce la logica, la comunicazione e l’elaborazione dei dati.
- **Sensore di peso**: connesso all’ESP32 per la misura del carico.
- **Lettore di codici a barre**: consente di riconoscere automaticamente il prodotto appoggiato sulla bilancia.
- **Circuito di carica**: composto da un TP4056 e una batteria gestisce la carica della batteria e invia a ESP32 il voltaggio corretto di alimentazione
- **Schermo LCD**: utilizzato per messaggi relativi all'oggetto, peso, comunicazioni o eventuali errori.
///
/// ## Funzionamento
1. **Identificazione dell’oggetto**: Viene eseguita la scansione del codice a barre per recuperare informazioni sul prodotto. Il codice GTIN rilevato veine inviato a un servizio ... che restituisce i dati sotoforma di ... Registriamo il peso previsto o il volume (500ml, 1000ml) e calcoliamo le soglie. 
2. **Lettura del peso**: L’ESP32 acquisisce i segnali dal sensore di peso. L'oggetto vine appoggiato sulla poattaforma di peso.
3. **Elaborazione dei dati**: L’ESP32 elabora peso e codice, eseguendo calcoli e memorizzando i risultati. Quando la precentuale di prodotto residuo si attesta al di sotto di una soglia prestabilita viene inviato un messaggio al server MQTT (...o altro servizio)
4. **Notifica o interfacciamento**: ESP32 si collega a un server (...) che accetta richieste con il codice GTIN del prodotto letto dal sensore 1D/2D. l server restituisce dati del prodotto in formato ...
Questi dati possono essere utilizzati per calcolare il valore peso soglia sotto il quale viene inviato un mesaggio al servere MQTT (...)
///
/// ## Conclusioni
Il sistema illustrato rappresenta un prototipo di bilancia “smart” che può essere impiegata in contesti domestici per esempio in frigo dove appoggiando un prodotto saremo automaticamente avvertiti se la quantità residua del prodotto è sotto una certa soglia e quindi vine e richiesto il nuvo approviggionamento dello stesso prodotto.
// ...existing code...
