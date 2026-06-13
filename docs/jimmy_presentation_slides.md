# Slides Jimmy

## MODUL: `rm_nvs`

```mermaid
sequenceDiagram
    participant C as Anropare
    participant N as nvs
    participant E as ESP-IDF NVS

    C->>N: rm_nvs_set_u8(key, value)
    N->>E: öppna namespace
    N->>E: skriv värde
    N->>E: commit
    N->>E: stäng handle
    N-->>C: resultat
```

- Wrappar ESP-IDF NVS bakom ett litet och enhetligt API.
- Samlar namespace, handles, commit och felhantering i en modul.
- Öppnar och stänger ett handle för varje operation.
- Prioriterar tydligt ägarskap och förutsägbart beteende.

## MODUL: `board_i2c`

```mermaid
sequenceDiagram
    participant S as I2C DEVICE
    participant B as board_i2c
    participant I as ESP-IDF I2C-driver
    participant H as BME280

    S->>B: board_i2c_read_reg(device, register, buffer)
    B->>B: validera + ta lås
    B->>I: transmit_receive()
    I->>H: registeradress
    H-->>I: registerdata
    I-->>B: resultat
    B->>B: släpp lås
    B-->>S: resultat
```

- Äger den delade fysiska I2C-bussen och dess livscykel.
- Validerar argument innan en fysisk transaktion startas.
- Serialiserar samtidiga transaktioner med ett lås.
- Håller sensorprotokoll utanför bussmodulen.

## MODUL: `environment_measurements`

**Dataflöde: från sensorproducent till lagrade kanaler**

```mermaid
sequenceDiagram
    participant M as MeasurementsManager<br/>polling-task
    participant P as Bme280Producer
    participant S as Bme280Sensor
    participant Store as MeasurementStore

    M->>P: read(batch)
    P->>S: read(reading)
    S->>S: läs, kalibrera och kompensera
    S-->>P: Bme280Reading
    P->>P: översätt till logiska kanaler
    P-->>M: MeasurementBatch med tre kanaler
    M->>M: validera batch och skapa tidsstämpel
    M->>Store: publish_batch(batch, timestamp)
    Store->>Store: lås mutex och uppdatera alla kanaler
```

- Producenter översätter datakällor till logiska mätkanaler.
- `MeasurementProducer` ger riktiga och simulerade producenter samma interface.
- `MeasurementStore` lagrar senaste värdet och metadata per kanal.
- C-API:t väljer och kombinerar kanaler efter konsumentens behov.
