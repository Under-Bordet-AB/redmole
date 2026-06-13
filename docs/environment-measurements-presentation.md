# Environment Measurements Module Presentation

Target length: 3-5 minutes  
Recommended format: 2 slides  
Main message: The module turns an unreliable physical sensor into a safe, simple,
and reusable source of current environment data.

## Slide 1: From BME280 To Reliable Application Data

```text
BME280 sensor
temperature + humidity + pressure
        |
        v  I2C, calibration, compensation
Bme280Sensor -> Bme280Producer
        |
        v  one coherent batch
MeasurementsManager
one FreeRTOS polling task
        |
        v  atomic publish
MeasurementStore
mutex-protected latest values
        |
        v
Stable C API -> GUI and UART
```

Key points:

- Reads calibrated temperature, humidity, and pressure over shared I2C.
- One FreeRTOS task polls at a configurable interval without busy-waiting.
- All three values are published atomically, so consumers receive one coherent sample.
- A small C API hides the internal C++ and hardware details from GUI and UART.

Strong course-goal evidence: **S6, S7 (I2C), S8, S11**

## Slide 2: Design Decisions For Robustness And Testability

| Problem | Design decision |
|---|---|
| Sensor is disconnected or I2C read fails | Invalidate the sample, retry full initialization later, and log failure/recovery once |
| Different tasks read shared values | Protect the latest-value store with a mutex and copy all channels atomically |
| Old data can look correct | Timestamp every publication and reject stale samples |
| Embedded memory must be predictable | Use static task, stack, mutex, semaphore, and fixed-capacity arrays |
| Hardware may be unavailable during development | Select a deterministic simulated producer through configuration |
| Future sensors should not force an API redesign | Separate physical producers from logical measurement channels |

Honest limitation:

- One Unity unit test proves that the simulated producer returns a complete
  three-measurement batch without sensor hardware. Meaningful manager, store,
  conversion, and failure-recovery tests are still the next quality step.

## Course Goals Checklist Audit

This module is one contribution to the group project. Do not claim that it alone
fulfills goals that require whole-system evidence.

| Goal | Evidence in this module | Assessment |
|---|---|---|
| K1: embedded constraints and reliability | Static memory, bounded sensor waits, physical I/O, recovery, stale-data handling | Strong reflection example |
| K2: RTOS and real-time | One task, priority, blocking wait, mutex, binary semaphore, task notification, atomic flag; no queue or ISR | Strong but partial |
| K3: UART/SPI/I2C | Implements addressed I2C register communication and shared-bus use; does not implement UART or SPI | Strong I2C example only |
| K4: constrained embedded C++ | Classes, interface, strong types, `constexpr`, fixed arrays, deleted copying, explicit error codes, C API boundary | Strong; do not claim full RAII |
| K5: automation and CI | Simulator and injectable producer/clock support testability; one Unity unit test verifies the simulated batch contract; repository CI builds releases | Partial |
| S6: RTOS-based system | Clear polling-task responsibility, intentional synchronization, blocking instead of busy-wait, robust error handling | Strong contribution |
| S7: UART/SPI/I2C implementation | BME280 I2C driver, register parsing, calibration, compensation, and communication-error handling | Strong I2C contribution only |
| S8: constrained C++ component | Modular C++, predictable memory, explicit ownership rules, interfaces, error codes | Strong contribution |
| S9: hardware-near debugging | Diagnostic logs for initialization, chip ID, failure, and recovery; no documented debugger or measurement-tool evidence | Partial |
| S10: simple test automation | Unity test project runs a focused hardware-free `SimProducer` unit test | Basic evidence |
| S11: documentation | README, API Doxygen, architecture flow, data structures, configuration, error behavior, and test setup | Strong contribution |

Best goals to mention during the short presentation:

- **S6:** RTOS task, synchronization, blocking waits, and robust failure handling.
- **S7:** real BME280 communication over I2C, including calibration and errors.
- **S8:** structured C++ with predictable memory and a stable C boundary.
- **S11:** documented responsibilities, flow, interfaces, configuration, and failures.

Useful reflection links to knowledge goals:

- **K1:** predictable resources and recovery matter because this controls physical hardware.
- **K2:** this is soft real-time; it uses blocking RTOS mechanisms and does not claim a hard deadline.
- **K3:** I2C is a shared, addressed two-wire bus; this module handles one device through `board_i2c`.
- **K4:** abstraction is useful only while runtime and memory costs stay predictable.
- **K5:** testable structure is not enough; meaningful tests must also be written and run automatically.

## Speaker Script

### 0:00-0:30 - Purpose

"I worked on the environment measurements module. Its job is not only to read a
BME280 sensor. Its real job is to turn unreliable hardware communication into a
simple and trustworthy sample that the GUI and UART can use.

The public result contains temperature, humidity, pressure, a timestamp, and a
valid flag. Application code does not need to know anything about BME280
registers, calibration, I2C, or FreeRTOS synchronization."

### 0:30-2:15 - Slide 1: Architecture And Data Flow

"The data flow starts with the BME280. It communicates over the board's shared
I2C bus. The Bme280Sensor class verifies the chip, loads its factory calibration
values, reads the raw registers, and applies the compensation formulas.

One physical sensor read gives temperature, humidity, and pressure together.
The Bme280Producer converts this into one measurement batch. Keeping the values
in one batch is important because consumers should not see a new temperature
combined with an old humidity value.

The MeasurementsManager owns one FreeRTOS task. It polls the producer at a
configurable interval and blocks between polls, so it does not waste CPU with
busy-waiting. It then publishes the complete batch into the MeasurementStore.

The store uses a FreeRTOS mutex. It writes and copies the complete batch while
holding that mutex, which prevents race conditions and gives readers a coherent
snapshot.

Finally, the module exposes a stable C API. This allows C modules such as the GUI
and UART to use the data while the internal implementation remains structured
C++."

### 2:15-4:00 - Slide 2: Important Design Decisions

"My main design focus was robustness.

First, sensor failure is treated as normal runtime behavior, not as a reason to
crash the system. If an I2C read fails, the module immediately marks the values
invalid so consumers cannot continue using misleading old data. Later polling
attempts retry the complete sensor initialization, so the module can recover
after the sensor is reconnected.

Second, every successful publication gets a monotonic timestamp. The public API
rejects stale samples, even if the stored numbers still look reasonable.

Third, memory usage is predictable. The polling task, task stack, mutex,
semaphore, and measurement batches use static or fixed-capacity storage. There
is no dynamic allocation in the measurement path.

I also separated the generic measurement pipeline from the BME280 driver. A
producer interface means the same manager and store can work with the real
sensor or a deterministic simulated producer selected at build time. This lets
the GUI and integration be developed when hardware is unavailable, and it makes
future sensor types easier to add.

This module is strong evidence for four practical course goals: S6 through its
FreeRTOS task and synchronization, S7 through real BME280 I2C communication, S8
through resource-aware C++, and S11 through its architecture and API
documentation. It only covers the I2C part of S7, not UART or SPI."

### 4:00-4:30 - Reflection And Close

"The largest lesson was that reading a sensor is the easy part. The harder and
more valuable part is defining ownership, handling failures, keeping shared data
coherent, and giving the rest of the application a small reliable interface.

For S10 test automation, I added a focused Unity unit test. It constructs the
simulated producer, performs one read, and verifies that a complete batch
contains three measurements without requiring sensor hardware. The next step
would be broader tests for atomic publication, stale data, invalid batches, and
recovery after read failures."

## Short Version If Time Is Limited

For a three-minute presentation:

- Explain the purpose in 20 seconds.
- Spend about 90 seconds on Slide 1.
- Spend about 60 seconds on failure handling, atomic snapshots, and static memory.
- End with the lesson and honest testing limitation in 20 seconds.

## Likely Questions

**Why use a mutex instead of a queue?**  
The module stores only the latest sample, and several consumers may request it at
different times. A mutex-protected latest-value store matches that requirement.
A queue would be better if every sample had to be processed or if history were
required.

**Why one sensor task instead of one task per sensor?**  
The reads are short and periodic, and there is currently one producer. Sequential
polling uses less stack memory and avoids unnecessary task coordination.

**Why use scaled integers instead of floats in the public API?**  
Scaled integers make units explicit and give predictable representation. The
module retains higher internal precision and converts only at the public boundary.

**What happens when the BME280 is unplugged?**  
The read fails, all channels owned by that producer become invalid, consumers
stop receiving a valid sample, and later polls retry full initialization until
the sensor recovers.

**Does this meet hard real-time requirements?**  
No hard deadline is claimed. It is a periodic, soft real-time measurement task.
The design blocks between polls, uses bounded sensor waits, and prioritizes
robustness over minimum latency.

**Why not use `vTaskDelayUntil()` for exact periodic timing?**  
The task uses a timed task-notification wait so `stop()` can wake it immediately.
This makes shutdown responsive, but the polling period includes read execution
time and is therefore not an exact fixed-rate schedule.

**Does the module fulfill the complete communication goal?**  
No. It is strong evidence for the I2C part of S7. UART and SPI are implemented
elsewhere in the group project.

**Does the module use RAII?**  
It uses C++ classes and clear process-lifetime ownership, but it does not release
the retained I2C handle in a destructor. The accurate claim is resource-aware
C++ with predictable lifetime, not complete RAII.

## Svenskt Talmanus

### Inledning - cirka 30 sekunder

"Jag har främst arbetat med tre moduler: environment measurements, board I2C
och NVS. Jag kommer framför allt fokusera på environment-modulen, men I2C- och
NVS-modulerna är viktiga delar av lösningen.

Environment-modulens uppgift är inte bara att läsa en BME280-sensor. Den ska
göra osäker hårdvarukommunikation till ett enkelt och pålitligt mätvärde som
resten av systemet kan använda."

### Bild 1 - Arkitektur och dataflöde - cirka 2 minuter

"Dataflödet börjar i BME280-sensorn. Sensorn kommunicerar över I2C och ger oss
rådata för temperatur, luftfuktighet och lufttryck.

Jag skapade även board I2C-modulen. Den äger den delade I2C-bussen och ser till
att flera delar av systemet kan använda bussen på ett säkert sätt. Modulen
hanterar bland annat initiering, enhetsadresser, registerläsningar och
synkronisering.

Bme280Sensor-klassen kontrollerar sensorns chip-ID, läser
kalibreringskonstanter och omvandlar rådata med kompensationsformlerna från
databladet.

De tre mätvärdena kommer från samma fysiska avläsning och skickas därför vidare
som en gemensam batch. Det förhindrar att systemet kombinerar exempelvis en ny
temperatur med ett gammalt luftfuktighetsvärde.

MeasurementsManager äger en FreeRTOS-task som läser sensorn med ett
konfigurerbart intervall. Tasken blockerar mellan avläsningarna i stället för
att använda busy-waiting och slösa processortid.

Resultatet lagras i MeasurementStore. Lagringen skyddas av en mutex och hela
batchen publiceras atomiskt. Det betyder att GUI och UART alltid får en
sammanhängande ögonblicksbild.

Till sist exponeras ett litet C-API. Resten av applikationen behöver därför inte
känna till I2C-register, BME280-kalibrering eller den interna C++-designen."

### Bild 2 - Designbeslut och robusthet - cirka 1,5 minuter

"Mitt viktigaste designmål var robusthet.

Om sensorn kopplas ur eller en I2C-läsning misslyckas markerar modulen direkt
mätvärdena som ogiltiga. Det är bättre än att fortsätta visa gamla värden som
om de fortfarande vore korrekta. Vid senare avläsningar försöker modulen
initialisera sensorn igen, vilket gör att den kan återhämta sig efter ett fel.

Varje publicering får också en monoton tidsstämpel. Det publika API:t avvisar
värden som blivit för gamla.

Minnesanvändningen är förutsägbar. Task, stack, mutex, semafor och mätbatcher
använder statisk eller fast lagring. Det finns ingen dynamisk allokering i
mätflödet.

Jag skapade också en simulerad producent. Den följer samma interface som den
riktiga sensorn och gör att systemet kan utvecklas och testas utan ansluten
BME280.

NVS-modulen används på en annan nivå för beständig lagring. Den kapslar in
ESP-IDF:s NVS, validerar argument och skriver inställningar till flash med
omedelbar commit."

### Tester, kursmål och avslutning - cirka 1 minut

"För enkel testautomation använder projektet Unity.

Environment-testet verifierar att den simulerade producenten returnerar en
komplett batch med tre mätvärden. I2C-testet verifierar att ogiltiga argument
och adresser avvisas innan hårdvaran används. NVS-testet skriver, läser,
verifierar och raderar ett värde i flash.

Det här ger praktiska exempel på flera kursmål. S6 visas genom FreeRTOS-tasken,
blockerande väntan och synkronisering. S7 visas genom riktig
I2C-kommunikation. S8 visas genom strukturerad och resursmedveten C++. S10 visas
genom Unity-testerna och S11 genom dokumentationen av arkitektur, API och
felhantering.

Min viktigaste lärdom är att själva sensorläsningen bara är en del av arbetet.
Det svårare är att definiera ägarskap, hantera fel, skydda delad data och ge
resten av systemet ett litet och pålitligt interface."
