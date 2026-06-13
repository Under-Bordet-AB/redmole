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

**Data in: producenter till lagrade kanaler**

```mermaid
sequenceDiagram
    participant BME as fysisk BME280
    participant S as Bme280Sensor
    participant P as Bme280Producer
    participant M as MeasurementsManager
    participant Store as MeasurementStore

    BME-->>S: rå temperatur, fukt och tryck
    S-->>P: Bme280Reading med kompenserade sensorvärden
    P-->>M: MeasurementBatch med logiska kanaler
    M->>Store: validerad batch och tidsstämpel
    Note over Store: lagrar senaste värdet per kanal
```

**Alternativ Data in: funktionsanrop genom modulerna**

```mermaid
sequenceDiagram
    participant M as MeasurementsManager
    participant P as Bme280Producer
    participant S as Bme280Sensor
    participant Store as MeasurementStore

    M->>P: read(batch)
    P->>S: read(reading)
    S-->>P: Bme280Reading
    P-->>M: MeasurementBatch
    M->>Store: publish_batch(batch, now_ms())
    Store-->>M: resultat
```

## MODUL: `environment_measurements`

**Data ut: lagrade kanaler till C-API**

```mermaid
sequenceDiagram
    participant Store as MeasurementStore
    participant A as C-API
    participant G as GUI / UART

    G->>A: environment_measurements_get_latest()
    A->>Store: copy_channels(temperatur, fukt, tryck)
    Store-->>A: sammanhängande snapshot
    Note over A: bygger environment_measurement_sample_t
    A-->>G: temperatur, fukt, tryck
```

- Producenter översätter olika datakällor till logiska kanaler.
- De tre kanalerna är temperatur, luftfuktighet och tryck. Kanaltyperna
  definieras i `src/measurement_types.hpp`, medan kanaluppsättningen som den
  nuvarande C-API-funktionen hämtar definieras i `src/environment_measurements.cpp`.
- Kanalerna transporteras tillsammans i en `MeasurementBatch`.
- Store lagrar senaste värdet separat för varje kanal.
- C-API:t hämtar en sammanhängande snapshot och bygger en enkel sample.
