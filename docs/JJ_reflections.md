# Individuell reflektion över grupparbetet RedMole

## NVS, delad I2C-buss och miljömätningar

**Författare:** Jimmy Jordan  
**Kurs:** Kurs 4

## Innehållsförteckning

- [Inledning och syfte](#inledning-och-syfte)
- [Arbetets gång och förändring](#arbetets-gång-och-förändring)
- [AI-användning](#ai-användning-stora-möjligheter-stora-risker)
- [Modulernas typ, ägarskap och exekveringsmodell](#modulernas-typ-ägarskap-och-exekveringsmodell)
- [Systemöversikt](#systemöversikt)
- [MODUL: `rm_nvs`](#modul-rm_nvs)
- [MODUL: `board_i2c`](#modul-board_i2c)
- [MODUL: `environment_measurements`](#modul-environment_measurements)
- [Hur modulerna stödjer varandra under drift](#hur-modulerna-stödjer-varandra-under-drift)
- [Reflektion mot kursmål 1–5](#reflektion-mot-kursmål-15)
- [Kritisk utvärdering och nästa steg](#kritisk-utvärdering-och-nästa-steg)
- [Slutsats](#slutsats)

## Inledning och syfte

I projektet har jag arbetat med tre moduler som ligger på olika abstraktionsnivåer:

- `nvs` ger resten av applikationen ett gemensamt och kontrollerat sätt att lagra beständiga värden.
- `board_i2c` äger den gemensamma fysiska I2C-bussen och ser till att bussoperationer inte körs samtidigt.
- `environment_measurements` läser miljösensorer, omvandlar mätningar till logiska kanaler och publicerar en sammanhängande senast känd mätning.

Jag har försökt bygga modulerna så att varje modul ska:

- ha ett tydligt ansvarsområde
- äga sitt eget tillstånd och sina egna data
- skydda interna delar från andra modulers påverkan

Jag har försökt skapa djupa moduler med smala API:er: den publika ytan ska vara enkel, medan modulen själv tar ansvar för den bakomliggande komplexiteten.

## Arbetets gång och förändring
När jag började arbetet tänkte jag framför allt på moduler som ett sätt att gruppera relaterade funktioner och den hårdvara som hörde ihop. Efter hand insåg jag att jag då ofta bara översatte en tillfällig uppdelning från andra delar av systemet till min egen kod. Modulgränserna följde exempelvis hur ESP-IDF hade delat upp sitt API eller hur Waveshare hade valt att bygga kortet, i stället för vilket ansvar vår applikation faktiskt behövde hantera.

Miljömodulen blev det tydligaste exemplet på detta. Min första C-baserade lösning var uppdelad i tre separata komponenter: `bme280` hanterade sensorhårdvaran, `local_sensor_service` pollade sensorn och `sensor_data` lagrade den senaste mätningen. Uppdelningen såg strukturerad ut, men den speglade främst implementationen och den första sensorns form. För resten av applikationen var det viktiga ansvarsområdet egentligen **miljömätningar**. Exakt vilken sensor som används, hur den pollas och hur värdena lagras är implementationdetaljer bakom det ansvaret.

Det förändrade hur jag tänker kring moduldesign. Jag försöker nu börja med frågan *vilket logiskt ansvar ska modulen äga?* i stället för *vilka funktioner eller hårdvarudelar ser ut att höra ihop?* När ansvaret är tydligt går det sedan att bestämma vilket tillstånd, vilka resurser och vilken concurrency modulen behöver äga och skydda.

Den insikten syns i alla tre modulerna:

- NVS-modulen äger wrapperns namespace och lagringspolicy.
- `board_i2c` äger den enda delade fysiska bussen och serialiserar åtkomsten.
- Miljömodulen äger polling-tasken och senaste publicerade mätdata, men delar bara ut kopior.

## AI användning, stora möjligheter stora risker
HÄR BERÄTTAR JAG OM HUR JAG ANVÄNT AI.

### Det jag behöver komplettera personligt

- [DU BEHÖVER KOMPLETTERA MED VILKEN DEL SOM VAR SVÅRAST OCH VARFÖR HÄR]
- [DU BEHÖVER KOMPLETTERA MED ETT KONKRET PROBLEM ELLER FEL DU FASTNADE PÅ HÄR]
- [DU BEHÖVER KOMPLETTERA MED HUR DU FELSÖKTE DET PROBLEMET OCH VAD DU LÄRDE DIG HÄR]
- [DU BEHÖVER KOMPLETTERA MED VAD SOM VAR MEST ÖVERRASKANDE ELLER EN AHA-UPPLEVELSE HÄR]
- [DU BEHÖVER KOMPLETTERA MED VILKEN DEL DU KÄNNER DIG STARKAST RESPEKTIVE SVAGAST PÅ EFTER ARBETET HÄR]

## Modulernas typ, ägarskap och exekveringsmodell

Innan anropsflödena beskrivs är det viktigt att skilja mellan en modul, ett objekt och en task. En modul behöver inte ha en egen task. En modul kan också vara single-instance i den färdiga produkten även om den internt använder vanliga instansierbara C++-objekt.

| Modul | Instansmodell | Äger task? | Äger tillstånd och resurser | Hur data delas |
|---|---|---:|---|---|
| `nvs` | Single-instance service-modul i C | Nej | Wrapperns init-status, en kopia av valt namespace och lifecycle-lock. ESP-IDF äger den underliggande flashimplementationen. | Copy-in vid skrivning och copy-out till anroparägda värden eller buffertar vid läsning. |
| `board_i2c` | Single-instance service-modul i C för kortets enda delade buss | Nej | Busshandle, init-status, kortets busskonfiguration och lifecycle-lock. Device-handles som skapas av `board_i2c_add_device()` ägs av anroparen. | Caller-owned buffertar för läsning och skrivning samt opaque device-handles. `board_i2c_get_bus()` är ett medvetet undantag som lämnar ut rått busshandle. |
| `environment_measurements` | Single-instance produktmodul genom sitt publika C-API | Ja, en statiskt allokerad polling-task | Vald sensor och producent, registreringar, manager, latest-value-store, task, stack och synkroniseringsobjekt. | Copy-out av en mutexskyddad snapshot genom `environment_measurements_get_latest()`. Inga pekare till internt muterbart mätdata lämnas ut. |

### Single-instance jämfört med vanliga objekt

`nvs` och `board_i2c` lagrar sitt tillstånd i fil-lokala statiska variabler. Deras publika API saknar ett instansargument, vilket innebär att alla anrop använder samma modulinstans:

```text
rm_nvs_init("app")       -> samma s_is_initialized och s_default_namespace
board_i2c_init()         -> samma s_bus och s_lifecycle_lock
```

Det passar resurser som bara ska finnas en gång i produkten: ett valt applikations-namespace och kortets enda delade I2C-buss.

`environment_measurements` är också single-instance sett från resten av applikationen. `environment_measurements.cpp` komponerar en global produktinstans med `s_indoor_producer`, `s_measurement_store`, `s_producers` och `s_measurements_manager`. Det publika C-API:t arbetar alltid mot denna komposition.

Internt är däremot exempelvis `Bme280Sensor`, `Bme280Producer`, `SimProducer`, `MeasurementStore` och `MeasurementsManager` vanliga C++-objekt. De tar sina beroenden genom konstruktorer och kan i princip instansieras flera gånger. Detta gör den interna designen återanvändbar och testbar även om den nuvarande produkten bara komponerar en miljömodul.

### Taskägarskap och aktiv respektive passiv modul

NVS och `board_i2c` är passiva service-moduler. De kör ingenting själva mellan anrop, utan exekverar i den anropande taskens kontext. Om GUI-tasken anropar `rm_nvs_get_u8()` eller en sensortask anropar `board_i2c_read_reg()`, körs arbetet i just den tasken.

Miljömodulen är en aktiv modul efter `environment_measurements_start()`. Den äger polling-tasken `"environment"`, som självständigt läser producenter och publicerar mätningar med valt intervall. Konsumenter kör fortfarande i sina egna tasks när de hämtar en copy-out-snapshot från modulen.

```mermaid
flowchart LR
    subgraph Passive["Passiva service-moduler"]
        CALLER["Anropande task"]
        NVS["nvs"]
        I2C["board_i2c"]
        CALLER -->|"direkt funktionsanrop"| NVS
        CALLER -->|"direkt funktionsanrop"| I2C
    end

    subgraph Active["Aktiv miljömodul"]
        ENVTASK["environment-task"]
        STORE["MeasurementStore"]
        CONSUMER["Konsumentens task"]
        ENVTASK -->|"publish_batch()"| STORE
        STORE -->|"copy-out snapshot"| CONSUMER
    end
```

### Dataägarskap: kopior i stället för delade datapekare

Den huvudsakliga designstilen är att modulerna behåller sitt interna muterbara tillstånd och kopierar data över API-gränsen:

- NVS tar emot värden eller inputbuffertar vid skrivning och kopierar ut data till anroparens output.
- `board_i2c` använder anroparägda transaktionsbuffertar. Device-handles är opaque referenser, inte direkt åtkomst till objektets interna fält.
- Miljömodulen kopierar först en sammanhängande snapshot under store-mutexen och kopierar sedan den färdiga publika samplen till anroparen.

Copy-out gör ägarskap och livslängd tydliga. När anropet är färdigt behöver konsumenten inte behålla modulens mutex och kan inte ändra modulens interna data genom en pekare. Kostnaden är datakopiering, men datastrukturerna är små och kostnaden är därför rimlig.

Pekare används fortfarande som C-språkets sätt att ange caller-owned input och output, exempelvis `environment_measurements_get_latest(&sample)`. Det är inte samma sak som att dela en pekare till modulens interna lagring. Det tydligaste undantaget är `board_i2c_get_bus()`, som lämnar ut ett rått delat handle för ESP-IDF-integrationer och därför kräver striktare kontroll över bussens livscykel.

## Systemöversikt

De tre modulerna samverkar under uppstart och drift, men de har olika roller:

```mermaid
flowchart LR
    MAIN["app_main()"]
    NVS["nvs<br/>beständig lagring"]
    ENV["environment_measurements<br/>insamling och publicering"]
    I2C["board_i2c<br/>delad buss"]
    BME["BME280"]
    GUI["GUI / UART"]
    FLASH["NVS-partition i flash"]

    MAIN -->|"rm_nvs_init()"| NVS
    MAIN -->|"board_i2c_init()"| I2C
    MAIN -->|"environment_measurements_init()<br/>environment_measurements_start()"| ENV
    NVS -->|"nvs_open(), nvs_get_*(), nvs_set_*()"| FLASH
    ENV -->|"board_i2c_add_device()<br/>board_i2c_read_reg()<br/>board_i2c_write_reg()"| I2C
    I2C -->|"ESP-IDF I2C master driver"| BME
    GUI -->|"environment_measurements_get_latest()"| ENV
    GUI -->|"rm_nvs_get_*() / rm_nvs_set_*()"| NVS
```

Systemets övergripande initieringsordning är också viktig. I2C-bussen måste finnas innan miljösensorn kan registreras, och miljömodulen måste vara initierad innan dess FreeRTOS-task startas.

```mermaid
sequenceDiagram
    participant Main as app_main()
    participant Init as init_single_instance_modules()
    participant Runtime as init_runtime_modules()
    participant Start as start_runtime_modules()
    participant NVS as rm_nvs
    participant I2C as board_i2c
    participant Env as environment_measurements

    Main->>Main: xEventGroupCreate()
    Main->>Init: init_single_instance_modules(&s_event_group)
    Init->>NVS: rm_nvs_init("app")
    Init->>I2C: board_i2c_init()
    Init->>Env: environment_measurements_init()
    Note over Init: Övriga moduler initieras också
    Main->>Runtime: init_runtime_modules()
    Runtime->>NVS: app_gui_bindings_load_saved_appearance()
    Main->>Start: start_runtime_modules()
    Start->>Env: environment_measurements_start()
    Main->>NVS: rm_nvs_get_str("wifi_ssid", ...)
    Main->>NVS: rm_nvs_get_str("wifi_pass", ...)
    loop Applikationens huvudloop
        Main->>Main: app_gui_bindings_sync(&s_gui)
        Main->>Main: task_scheduler_work()
        Main->>Main: vTaskDelay(30 ms)
    end
```

---

# MODUL: `rm_nvs`

### Ansvar och design

NVS-modulen är en förenkling och formalisering av ESP-IDF:s inbyggda NVS-funktionalitet. Jag gjorde denna wrapper avsiktligt så enkel som möjligt: resten av applikationen behöver inte känna till hur partitionen initieras, vilket namespace som används, när data ska committas eller hur handles öppnas och stängs, utan skriver eller läser bara en etikett och ett värde. Detta ger oss centraliserad kontroll samtidigt som modulen enkelt kan utvidgas om vi behöver mer avancerad funktionalitet. Jag kommunicerade detta till teamet tidigt och bad dem återkomma med förslag på funktioner som saknades. Eftersom inga sådana önskemål har uppkommit får modulen förbli enkel.

Modulen äger:

- om wrappern är initierad,
- en egen kopia av valt namespace,
- reglerna för validering av argument,
- återställningsbeteendet om NVS-partitionen inte kan initieras,
- mönstret öppna, använd, commit vid skrivning och stäng.

Det minskar risken att olika delar av applikationen använder NVS på olika eller oförenliga sätt.

### Exakt initieringsflöde

```mermaid
sequenceDiagram
    participant Main as init_single_instance_modules()
    participant API as rm_nvs_init("app")
    participant Lock as s_lifecycle_lock
    participant Flash as ESP-IDF NVS flash

    Main->>API: rm_nvs_init("app")
    API->>API: validera namespace
    API->>Lock: _lock_acquire()
    alt redan initierad med samma namespace
        API->>Lock: _lock_release()
        API-->>Main: ESP_OK
    else redan initierad med annat namespace
        API->>Lock: _lock_release()
        API-->>Main: ESP_ERR_INVALID_STATE
    else inte initierad
        API->>Flash: nvs_flash_init()
        alt ESP_ERR_NVS_NO_FREE_PAGES eller ESP_ERR_NVS_NEW_VERSION_FOUND
            API->>Flash: nvs_flash_erase()
            API->>Flash: nvs_flash_init()
        end
        alt initiering lyckades
            API->>API: memcpy(s_default_namespace, "app", ...)
            API->>API: s_is_initialized = true
        end
        API->>Lock: _lock_release()
        API-->>Main: resultat
    end
```

Återställningsvägen prioriterar att systemet kan starta även om NVS-partitionen har fel format eller saknar lediga sidor. Det är ett medvetet risktagande eftersom `nvs_flash_erase()` raderar all data i standardpartitionen, exempelvis Wi-Fi-inloggning och sparade användarinställningar. Risken är dokumenterad med `@warning` för `rm_nvs_init()`. Konsekvenserna är hanterbara idag, men återställningen bör göras mer robust i framtiden.

### Exakt driftflöde vid läsning

Nedan visas `rm_nvs_get_u8()`. Övriga get-funktioner följer samma struktur med motsvarande `nvs_get_*()`-anrop.

```mermaid
sequenceDiagram
    participant Caller as Anropande modul
    participant Get as rm_nvs_get_u8()
    participant Open as rm_nvs_open()
    participant Lock as s_lifecycle_lock
    participant NVS as ESP-IDF NVS

    Caller->>Get: rm_nvs_get_u8(key, out_value)
    Get->>Get: validera key och out_value
    Get->>Open: rm_nvs_open(NVS_READONLY, &handle)
    Open->>Lock: _lock_acquire()
    Open->>Open: kontrollera s_is_initialized
    Open->>NVS: nvs_open(s_default_namespace, NVS_READONLY, ...)
    Open->>Lock: _lock_release()
    Open-->>Get: handle eller fel
    Get->>NVS: nvs_get_u8(handle, key, out_value)
    Get->>NVS: nvs_close(handle)
    Get-->>Caller: resultat
```

### Exakt driftflöde vid skrivning

```mermaid
sequenceDiagram
    participant Caller as Anropande modul
    participant Set as rm_nvs_set_u8()
    participant Open as rm_nvs_open()
    participant NVS as ESP-IDF NVS

    Caller->>Set: rm_nvs_set_u8(key, value)
    Set->>Set: validera key
    Set->>Open: rm_nvs_open(NVS_READWRITE, &handle)
    Open->>NVS: nvs_open(s_default_namespace, NVS_READWRITE, ...)
    Open-->>Set: handle eller fel
    Set->>NVS: nvs_set_u8(handle, key, value)
    alt nvs_set_u8() lyckades
        Set->>NVS: nvs_commit(handle)
    end
    Set->>NVS: nvs_close(handle)
    Set-->>Caller: resultat
```

`rm_nvs_set_str()` och de övriga set-funktionerna använder samma mönster. `rm_nvs_erase_key()` öppnar med `NVS_READWRITE`, anropar `nvs_erase_key()`, gör `nvs_commit()` och stänger handtaget.

### Synkronisering och dataåtkomst

`s_lifecycle_lock` skyddar wrapperns livscykeltillstånd och namespace medan ett NVS-handle öppnas. Det gör att `rm_nvs_init()`, `rm_nvs_deinit()` och `rm_nvs_open()` inte samtidigt kan ändra eller läsa ett instabilt wrapper-tillstånd.

Låset används främst för att skydda modulens initiering och namespace. När ett NVS-handle väl har öppnats låter jag ESP-IDF hantera trådsäkerheten för själva läsningen eller skrivningen. Jag valde detta eftersom ESP-IDF redan har stöd för det och ett extra lås runt hela operationen därför inte behövs.

### Avvägningar

Varje operation öppnar och stänger ett NVS-handle. Det innebär lite extra arbete jämfört med att behålla ett handle under hela programmets livstid. Fördelen är enklare ägarskap och mindre risk för gamla eller felaktiga handles.

Varje lyckad skrivning committas direkt. Det gör beteendet tydligt och minskar risken att en ändring försvinner vid omstart. Nackdelen är fler flash-skrivningar och därmed potentiellt mer slitage. Därför ska anropande kod, exempelvis GUI-inställningar, endast skriva när värdet faktiskt har ändrats.

Alla anrop är synkrona och väntar tills operationen är klar. Det blockerar den anropande FreeRTOS-tasken, men andra tasks kan normalt fortsätta köras. För den här applikationen är det en rimlig avvägning eftersom NVS används för enstaka inställningar och inte i tidskritisk kod.

`rm_nvs_deinit()` avaktiverar wrappern och rensar dess namespace, men anropar inte `nvs_flash_deinit()`. Det är ett medvetet val eftersom andra ESP-IDF-komponenter kan använda samma standardpartition.

### Test

Unity-testet `"NVS stores reads and erases a u8 value"` provar en enkel men komplett livscykel:

```text
rm_nvs_init("unity_test")
-> rm_nvs_erase_key(test_key)
-> rm_nvs_set_u8(test_key, 42)
-> rm_nvs_get_u8(test_key, &stored_value)
-> rm_nvs_erase_key(test_key)
-> rm_nvs_get_u8(...) förväntas ge ESP_ERR_NVS_NOT_FOUND
-> rm_nvs_deinit()
```

### Reflektion NVS modulen

NVS-modulen kanske kan ses som ett onödigt extra lager framför ESP-IDF:s inbyggda funktionalitet. Samtidigt är NVS en delad resurs som används av flera moduler i projektet. Jag tycker därför att det är viktigt att kontrollera och formalisera hur den används, i stället för att förlita sig på att alla utvecklare alltid använder den på rätt sätt.

---

# MODUL: `board_i2c`

### Ansvar och design

Flera enheter använder samma fysiska I2C-buss, bland annat BME280, IO-expandern och touch-kontrollern. Ett alternativ hade varit att initiera bussen i `main` och sedan skicka runt ESP-IDF:s råa busshandle till alla användare. Det hade fungerat, men ansvaret för konfiguration, synkronisering och nedstängning hade då varit utspritt i systemet. `board_i2c` finns därför för att samla detta ansvar på ett ställe och ge övriga moduler ett gemensamt sätt att använda bussen.

`board_i2c` äger:

- initiering och kontrollerad nedstängning av den gemensamma bussen,
- kortets fasta SDA-, SCL- och portkonfiguration,
- registrering av I2C-enheter,
- serialisering av modulens wrapper-operationer,
- gemensamma hjälpfunktioner för registerläsning och skrivning.

Modulen känner däremot inte till BME280:s registerprotokoll eller andra enheters verksamhetslogik. Sådana detaljer hör hemma i respektive drivrutin.

### Exakt initieringsflöde

```mermaid
sequenceDiagram
    participant Main as init_single_instance_modules()
    participant API as board_i2c_init()
    participant Lock as s_lifecycle_lock
    participant Internal as board_i2c_init_locked()
    participant Driver as ESP-IDF I2C master

    Main->>API: board_i2c_init()
    API->>Lock: _lock_acquire()
    API->>Internal: board_i2c_init_locked()
    alt s_initialized är true
        Internal-->>API: ESP_OK
    else bussen är inte skapad
        Internal->>Internal: skapa i2c_master_bus_config_t
        Note over Internal: I2C_NUM_0, SDA GPIO8,<br/>SCL GPIO9, glitch_ignore_cnt 7
        Internal->>Driver: i2c_new_master_bus(&bus_config, &s_bus)
        alt lyckades
            Internal->>Internal: s_initialized = true
        end
        Internal-->>API: resultat
    end
    API->>Lock: _lock_release()
    API-->>Main: resultat
```

`board_i2c_init_locked()` gör initieringen idempotent. Samma interna funktion används även av exempelvis `board_i2c_add_device()` och `board_i2c_get_bus()`, vilket gör att modulen kan initiera sig själv vid behov utan att skapa flera bussinstanser.

### Exakt registrering av en I2C-enhet

Miljömodulens BME280-driver registrerar sin enhet genom följande kedja:

```mermaid
sequenceDiagram
    participant Sensor as Bme280Sensor::init()
    participant Add as board_i2c_add_device()
    participant Lock as s_lifecycle_lock
    participant Init as board_i2c_init_locked()
    participant Driver as ESP-IDF I2C master

    Sensor->>Add: board_i2c_add_device(address_, BOARD_I2C_DEFAULT_SPEED_HZ, &device_)
    Add->>Add: validera out och address <= 0x7f
    Add->>Add: *out = NULL
    Add->>Lock: _lock_acquire()
    Add->>Init: board_i2c_init_locked()
    Add->>Driver: i2c_master_bus_add_device(s_bus, &dev_config, out)
    Add->>Lock: _lock_release()
    Add-->>Sensor: resultat
```

Som jag ser det finns det tre olika varianter på att hantera enheterna som använder bussen:

- `board_i2c` kan äga alla handles genom ett centralt register, vilket ger stark kontroll men mer komplexitet.
- Drivrutinen kan själv skapa och äga sitt handle, vilket håller bussmodulen enkel men ger svagare kontroll över bussens livscykel.
- `board_i2c` kan skapa handtaget och drivrutinen behåller det, vilket är nuvarande lösning.

Jag valde den nuvarande lösningen eftersom respektive drivrutin känner till sin enhet och alla enheter registreras en gång och lever under hela programmets livstid. Det håller `board_i2c` generell utan att ett centralt register behövs. Om dynamisk registrering eller kontrollerad nedstängning blir ett krav bör modulen även erbjuda borttagning och hålla reda på registrerade handles.

Hastigheten skickas in vid registreringen eftersom ESP-IDF lagrar den per device-handle och ställer om busshastigheten inför varje transaktion. En långsammare enhet kan därför dela bussen med snabbare enheter utan att alla måste använda den lägsta hastigheten. Alla enheter som för närvarande registreras genom `board_i2c` använder 400 kHz, men parametern gör det möjligt att ansluta en framtida eller extern enhet som exempelvis bara stöder 100 kHz. Värdet `0` väljer modulens standardhastighet.

### Exakt driftflöde vid registerläsning

```mermaid
sequenceDiagram
    participant DriverUser as Exempelvis Bme280Sensor::read_raw()
    participant API as board_i2c_read_reg()
    participant Lock as s_lifecycle_lock
    participant Driver as ESP-IDF I2C master
    participant Device as I2C-enhet

    DriverUser->>API: board_i2c_read_reg(dev, reg, data, len)
    API->>API: validera dev, data och len
    API->>Lock: _lock_acquire()
    API->>Driver: i2c_master_transmit_receive(dev, &reg, 1, data, len, 100 ms)
    Driver->>Device: skriv registeradress och läs svar
    Device-->>Driver: data
    Driver-->>API: resultat
    API->>Lock: _lock_release()
    API-->>DriverUser: resultat
```

Skrivning av ett register går via:

```text
board_i2c_write_reg(dev, reg, value)
-> skapa data[2] = {registeradress, värde}
-> board_i2c_write(dev, data, sizeof(data))
-> _lock_acquire(&s_lifecycle_lock)
-> i2c_master_transmit(dev, data, 2, timeout)
-> _lock_release(&s_lifecycle_lock)
```

Siffran `2` är buffertens längd i byte: en byte för registeradressen och en byte för värdet. `board_i2c_write_reg()` förbereder bara bufferten och återanvänder den generella skrivfunktionen. Endast `i2c_master_transmit()` utför den fysiska I2C-transaktionen.

### Icke-samtidig åtkomst

Locken hålls under hela varje wrapper-operation. Två anrop genom exempelvis `board_i2c_read_reg()` och `board_i2c_write()` kan därför inte använda bussen samtidigt. Det skyddar också mot att bussen tas bort av `board_i2c_deinit()` medan en wrapper-operation pågår.

Detta betyder inte att flera separata publika anrop tillsammans blir atomiska. Om en drivrutin först skriver och sedan läser genom två separata wrapper-anrop kan en annan uppgift använda bussen mellan dessa anrop. Om en operation måste vara odelbar ska den uttryckas som ett enda driveranrop, såsom `i2c_master_transmit_receive()` i `board_i2c_read_reg()`.

ESP-IDF:s I2C-driver serialiserar även fysiska transaktioner. Det är viktigt eftersom `board_i2c_get_bus()` lämnar ut det råa busshandtaget för integrationer som touch-drivrutinen:

```text
touch_gt911_init()
-> board_i2c_get_bus()
-> esp_lcd_new_panel_io_i2c(...)
```

När det råa handtaget har returnerats är användningen utanför `board_i2c`-modulens lifecycle-lock. Därför får `board_i2c_deinit()` endast köras under en kontrollerad nedstängning när inga råa användare finns kvar.

### Avvägningar och begränsningar

Designen är enkel och passar nuvarande system, men har några tydliga begränsningar:

- Det finns ingen central registry som hindrar två anropare från att registrera samma adress flera gånger.
- Anropare behåller sina device-handles, men modulen saknar ännu ett wrapper-anrop för att lämna tillbaka och ta bort dem.
- Det råa handtaget behövs för vissa ESP-IDF-integrationer, men försvagar modulens fullständiga kontroll över bussens livscykel.
- Serialisering ger korrekthet, men det finns ingen prioritering mellan långsamma och tidskritiska bussanvändare.
- Locken skyddar enskilda wrapper-anrop, inte en serie av flera anrop.

I2C-anropen är synkrona och blockerar tasken som använder bussen tills transaktionen är klar eller dess timeout på 100 ms nås. Andra FreeRTOS-tasks kan fortfarande köras, så detta är acceptabelt för nuvarande korta sensortransaktioner.

Om fler enheter eller hårdare realtidskrav tillkommer kan en dedikerad bus manager med köade transaktioner vara ett nästa steg. För det nuvarande systemet skulle det däremot tillföra onödig komplexitet.

### Test

Unity-testet `"board I2C rejects invalid transaction arguments"` provar att modulen avvisar:

- läsning med null-handle,
- skrivning med null-handle,
- registerläsning med null-handle,
- registrering av den ogiltiga sjubitarsadressen `0x80`.

Testet är medvetet hårdvaruoberoende och enkelt att köra. Det bevisar argumentkontraktet, men inte fysisk kommunikation, timeout-hantering eller samtidiga anrop. Sådana egenskaper kräver integrationstest på målplattformen.

### Reflektion

Min viktigaste insikt från I2C-modulen är att bussen är en delad resurs, inte bara en samling hjälpfunktioner. När flera enheter och tasks kan använda samma fysiska ledningar behöver åtkomsten kontrolleras på ett gemensamt ställe. Genom att låta `board_i2c` äga bussen men ej enheterna tycker jag det blir en bra balans och gränsen blir också tydlig: `board_i2c` hanterar transporten, medan drivrutinerna hanterar sina egna protokoll.

Så här i efterhand är jag glad att jag höll modulen så enkel som möjligt, särskilt eftersom `environment_measurements` krävde mycket arbete och flera designiterationer. Det viktigaste här är att förhindra samtidig användning som kan göra busskommunikationen felaktig. Det finns en frestelse att bygga en mer avancerad device manager liknande den i ett större operativsystem, men det skulle snabbt öka komplexiteten utan att lösa något aktuellt behov. Arbetet har lärt mig att en bra modul inte behöver göra allt den skulle kunna göra, utan ska göra det systemet faktiskt behöver på ett tydligt sätt.

---

# MODUL: `environment_measurements`

### Första lösningen

Jag började med att bygga miljömätningarna som ett gäng c moduler:  `bme280`, `local_sensor_service` och `sensor_data`. Målet var först bara att läsa temperatur, luftfuktighet och tryck från en BME280 och göra värdena tillgängliga för resten av systemet. Den första lösningen fungerade, men efterhand började jag däremot se problem med designen. Samma tre mätvärden kopierades mellan flera fasta strukturer, och hela kedjan från fysisk sensor till applikation hade BME280:s form inbyggd i koden. Uppdelningen i tre moduler såg flexibel ut, men i praktiken var de så beroende av samma sensor och datastruktur att de nästan lika gärna kunde ha varit en enda modul, exempelvis `bme280_measurements`.

Den första lösningen såg ut så här:

- BME280-lagret producerade en fast struktur med temperatur, luftfuktighet och tryck.
- `local_sensor_service` kopierade manuellt samma tre fält.
- `sensor_data` lagrade ännu en fast struktur med samma form.
- Simulatorbackend använde dynamisk allokering med `calloc()` och `free()`.
- Simulering eller verklig sensor valdes med en kompilatorflagga.

Begränsningen med de fasta strukturerna var inte främst att samma data kopierades flera gånger, utan att varje förändring av mätmodellen behövde göras på flera ställen. Om exempelvis en separat utomhustemperatur skulle läggas till behövde strukturerna, kopieringskoden och konsumenterna ändras tillsammans. Det skapade stark koppling mellan lager som egentligen skulle ha olika ansvar.

Den dynamiska allokeringen i simulatorbackend gav dessutom en livscykel som resten av lösningen behövde hantera korrekt. Varje allokering kunde misslyckas och varje skapad instans behövde frigöras exakt en gång. För en liten inbyggd produkt med ett känt antal sensorkällor gav detta mer felhantering och risk för minnesläckor eller heapfragmentering, utan att systemet faktiskt behövde kunna skapa producenter dynamiskt under drift.

Kompilatorflaggan gjorde det möjligt att testa utan fysisk sensor, men valet blev globalt och låst vid kompilering. Samma firmware kunde därför inte använda en verklig och en simulerad källa samtidigt, och simulatorn kunde inte enkelt injiceras i tester av enskilda delar. Flaggan bytte implementation, men skapade ingen gemensam abstraktion som flera producenter kunde implementera.

### Nuvarande lösning

När jag började om utgick jag från ansvarsområdet i stället för implementationsdetaljerna. Jag frågade mig vad denna del av systemet faktiskt gör, och svaret var att den mäter miljön. Miljömätningar fick därför bli modulens ansvarsområde, snarare än en viss sensor och en viss datastruktur. Problemet var inte egentligen hur BME280 skulle läsas, utan hur applikationen skulle hantera miljömätningar oberoende av vilken sensor som producerade dem.

Samtidigt upptäckte jag att jag hade missförstått kurskraven och att C++ behövde vara en del av lösningen. Jag skrev därför om lösningen till `environment_measurements`, där C++ används internt för tydliga typer, interface och objektrelationer, medan resten av applikationen fortfarande använder ett stabilt C-API. Omskrivningen uppfyllde därmed kurskravet, men blev också en möjlighet att förbättra designen i stället för att bara byta språk.

Produktkompositionen väljs fortfarande vid kompilering via menuconfig, men både `Bme280Producer` och `SimProducer` följer samma interface och hanteras av samma manager.

```mermaid
flowchart LR

    subgraph Current["Nuvarande environment_measurements"]
        SENSOR["Bme280Sensor<br/>hårdvaruprotokoll"]
        PRODUCER["Bme280Producer<br/>mätning till kanaler"]
        MANAGER["MeasurementsManager<br/>pollning och felhantering"]
        STORE["MeasurementStore<br/>senaste värde per kanal"]
        CAPI["C-API<br/>urval och sammansättning"]
        SENSOR --> PRODUCER --> MANAGER --> STORE --> CAPI
    end
```

### Ansvarsuppdelning

Modulen är uppdelad efter logiskt ansvar:

- `Bme280Sensor` känner till BME280:s register, kalibrering, konfiguration och konverteringsformler.
- `Bme280Producer` anpassar en fysisk BME280-avläsning till logiska `MeasurementChannel`-värden.
- `SimProducer` kan producera samma logiska kanaler utan fysisk hårdvara.
- `MeasurementsManager` initierar producenter, pollar dem sekventiellt och hanterar fel samt återhämtning.
- `MeasurementStore` lagrar senaste värde per kanal och skyddar sammanhängande publicering och kopiering med mutex.
- C-API:t väljer de kanaler som ska ingå i den publika mätningen, kontrollerar färskhet och konverterar enheter.


### Nuvarande produktkomposition

Vid kompilering väljs exakt en inomhusproducent: en riktig `Bme280Producer` eller en `SimProducer`. Den registrerade producenten äger just nu tre kanaler:

```text
MeasurementChannel::IndoorAmbientTemperature
MeasurementChannel::IndoorRelativeHumidity
MeasurementChannel::IndoorPressure
```

```mermaid
flowchart LR
    CONFIG["Kconfig"]
    REAL["Bme280Sensor<br/>+ Bme280Producer"]
    SIM["SimProducer"]
    REG["s_producers<br/>ProducerRegistration"]
    MANAGER["s_measurements_manager"]
    STORE["s_measurement_store"]
    API["environment_measurements_get_latest()"]

    CONFIG -->|"riktig källa"| REAL
    CONFIG -->|"simulerad källa"| SIM
    REAL --> REG
    SIM --> REG
    REG --> MANAGER
    MANAGER --> STORE
    STORE --> API
```

### Exakt initieringsflöde

Följande diagram visar initieringen med riktig BME280 vald:

```mermaid
sequenceDiagram
    participant Main as init_single_instance_modules()
    participant API as environment_measurements_init()
    participant Manager as MeasurementsManager::init()
    participant Store as MeasurementStore::init()
    participant Producer as Bme280Producer::init()
    participant Sensor as Bme280Sensor::init()
    participant I2C as board_i2c

    Main->>API: environment_measurements_init()
    API->>Manager: s_measurements_manager.init()
    Manager->>Manager: registrations_are_valid()
    Manager->>Store: s_measurement_store.init()
    Store->>Store: xSemaphoreCreateMutexStatic()
    Manager->>Manager: xSemaphoreCreateBinaryStatic(&stopped_storage_)
    Manager->>Producer: Bme280Producer::init()
    Producer->>Sensor: Bme280Sensor::init()
    Sensor->>I2C: board_i2c_add_device(address_, ..., &device_)
    Sensor->>Sensor: check_chip_id()
    Sensor->>I2C: board_i2c_probe_address(address_)
    Sensor->>I2C: board_i2c_read_reg(... BME280_REG_CHIP_ID ...)
    Sensor->>Sensor: settings_are_valid(settings_)
    Sensor->>Sensor: configure()
    Sensor->>Sensor: reset()
    Sensor->>I2C: board_i2c_write_reg(... BME280_REG_RESET ...)
    Sensor->>Sensor: vTaskDelay(kBme280ResetDelayMs)
    Sensor->>Sensor: wait_until_ready()
    Sensor->>Sensor: read_status()
    Sensor->>I2C: board_i2c_read_reg(... BME280_REG_STATUS ...)
    Sensor->>Sensor: load_calibration()
    Sensor->>I2C: board_i2c_read_reg(... BME280_REG_CALIB_00 ...)
    Sensor->>I2C: board_i2c_read_reg(... BME280_REG_CALIB_26 ...)
    Sensor->>Sensor: apply_settings()
    Sensor->>Sensor: write_ctrl_meas(Bme280Mode::Sleep)
    Sensor->>I2C: board_i2c_write_reg(... BME280_REG_CTRL_MEAS ...)
    Sensor->>Sensor: wait_until_ready()
    Sensor->>I2C: board_i2c_write_reg(... BME280_REG_CTRL_HUM ...)
    Sensor->>I2C: board_i2c_write_reg(... BME280_REG_CONFIG ...)
    Sensor->>Sensor: write_ctrl_meas(configured mode)
    Sensor->>I2C: board_i2c_write_reg(... BME280_REG_CTRL_MEAS ...)
    Sensor->>Sensor: read_settings()
    Sensor->>I2C: board_i2c_read_reg() för CTRL_HUM, CTRL_MEAS och CONFIG
    Sensor->>Sensor: settings_match(settings_, readback)
    Sensor->>Sensor: ready_for_reads_ = true
    alt producentinitiering misslyckas
        Manager->>Manager: mark_failed(index, result)
        Note over Manager: Manager-initiering fortsätter<br/>så sensorn kan återhämta sig senare
    end
    Manager-->>API: ESP_OK om manager-resurser skapades
    API-->>Main: resultat
```

Om simulatorn är vald ersätts sensorkedjan av `SimProducer::init()`, som återställer sitt `sample_index_`.

### Exakt start och task-livscykel

Initiering och start är separerade. Initieringen skapar modulens synkroniseringsobjekt och försöker initiera producenten. Start skapar eller återupptar tasken.

```mermaid
sequenceDiagram
    participant Main as start_runtime_modules()
    participant API as environment_measurements_start()
    participant Manager as MeasurementsManager::start()
    participant RTOS as FreeRTOS
    participant Task as MeasurementsManager::task_loop()

    Main->>API: environment_measurements_start()
    API->>Manager: s_measurements_manager.start()
    Manager->>Manager: stop_requested_.store(false)
    alt task_ == nullptr
        Manager->>RTOS: xTaskCreateStatic(task_entry, "environment", ...)
        RTOS->>Task: MeasurementsManager::task_entry(this)
        Task->>Task: task_loop()
    else tasken finns men är stoppad
        Manager->>RTOS: xTaskNotifyGive(task_)
    end
    Manager->>Manager: running_ = true
    Manager-->>Main: ESP_OK
```

Task, stack, control block, mutex och stop-semafor har statisk lagring. Det gör minnesanvändningen förutsägbar och undviker dynamisk allokering i mätflödet.

### Exakt driftflöde: från sensor till store

Tasken pollar producenter sekventiellt med ett konfigurerbart intervall på 1 till 60 sekunder. En enda task räcker eftersom sensorerna är långsamma och inte behöver varsin exekveringskontext.

```mermaid
sequenceDiagram
    participant Task as MeasurementsManager::task_loop()
    participant Manager as MeasurementsManager::poll_once()
    participant Producer as Bme280Producer::read()
    participant Sensor as Bme280Sensor::read()
    participant I2C as board_i2c_read_reg()/write_reg()
    participant Store as MeasurementStore
    participant RTOS as FreeRTOS

    loop så länge modulen kör
        Task->>Task: kontrollera stop_requested_
        Task->>Manager: poll_once()
        Manager->>Producer: producer.read(batch)
        Producer->>Sensor: sensor_.read(reading)
        Sensor->>Sensor: out = {}
        alt sensorn är inte redo
            Sensor->>Sensor: init()
        end
        opt forced mode
            Sensor->>Sensor: write_ctrl_meas()
            Sensor->>I2C: board_i2c_write_reg(...)
        end
        Sensor->>Sensor: read_raw()
        Sensor->>Sensor: wait_until_ready()
        Sensor->>I2C: board_i2c_read_reg(... BME280_REG_DATA ...)
        Sensor->>Sensor: convert(raw, reading)
        Sensor-->>Producer: Bme280Reading
        Producer->>Producer: mappa reading till tre MeasurementChannel
        Producer->>Producer: batch.count = 3
        Producer-->>Manager: MeasurementBatch
        Manager->>Manager: batch_is_valid_for_registration()
        alt lyckad och giltig batch
            Manager->>Store: publish_batch(batch, now_ms())
            Store->>Store: batch_is_valid(batch)
            Store->>Store: xSemaphoreTake(mutex_)
            Store->>Store: skriv värden med samma timestamp och publication_version
            Store->>Store: xSemaphoreGive(mutex_)
            Manager->>Manager: mark_recovered(index)
        else fel
            Manager->>Store: invalidate_channels(...)
            Manager->>Manager: mark_failed(index, result)
        end
        Task->>RTOS: ulTaskNotifyTake(pdTRUE, intervall)
    end
```

Alla värden från samma fysiska avläsning publiceras i en batch med samma tidsstämpel och `publication_version`. Store-mutexen hålls under hela uppdateringen. En konsument kan därför inte läsa en blandning där exempelvis temperaturen är ny men luftfuktigheten fortfarande kommer från föregående batch.

Om en producent misslyckas ogiltigförklaras dess kanaler direkt. Modulen fortsätter sedan försöka vid kommande pollningar. Det förhindrar att konsumenter använder gammal data som om den fortfarande vore giltig och gör samtidigt frånkopplade sensorer återhämtningsbara.

### Exakt driftflöde: från store till publikt C-API

Det är här urval och sammansättning sker. Nuvarande C-API väljer de tre registrerade inomhuskanalerna och bygger en `environment_measurement_sample_t`.

```mermaid
sequenceDiagram
    participant GUI as app_gui_bindings_sync()
    participant API as environment_measurements_get_latest()
    participant Copy as copy_indoor_measurements()
    participant Store as MeasurementStore::copy_channels()
    participant Time as now_ms()

    GUI->>API: environment_measurements_get_latest(&sample)
    API->>API: *out = {}
    API->>Copy: copy_indoor_measurements(stored)
    Copy->>Store: copy_channels(kIndoorBme280Channels, 3, out)
    Store->>Store: xSemaphoreTake(mutex_)
    Store->>Store: kopiera alla tre MeasurementRecord
    Store->>Store: xSemaphoreGive(mutex_)
    Store-->>API: sammanhängande snapshot
    API->>Time: now_ms()
    API->>API: sample_is_fresh() för varje kanal, max 5 sekunder
    API->>API: välj äldsta timestamp för hela samplet
    API->>API: convert_to_int32() för temperatur, fukt och tryck
    API->>API: out->valid = true
    API-->>GUI: true och färdig sample
```

De interna värdena behåller högre precision. Enhetskonvertering och avrundning sker först vid C-API-gränsen:

- milli-Celsius till tiondels grader Celsius,
- milli-procent till tiondels procent,
- Pascal till tiondels hektopascal.

Det gör att framtida konsumenter kan använda den interna precisionen utan att producenterna behöver ändras.

### Tidsstämplar och begränsningar

Mätningarnas tidsstämplar baseras på monoton tid sedan systemets uppstart. De används för att beräkna hur gammal en mätning är och avgöra om den fortfarande är färsk, men representerar inte verkligt datum eller klockslag.

Jag hann inte implementera synkronisering mot exempelvis NTP eller en extern realtidsklocka. Tidsstämplarna kan därför inte användas för att avgöra när en mätning gjordes i kalendertid eller för att jämföra mätningar från olika uppstarter. Vid varje omstart börjar tidsbasen om från noll.

### Stopp och återstart

```mermaid
sequenceDiagram
    participant Caller as Anropare
    participant API as environment_measurements_stop()
    participant Manager as MeasurementsManager::stop()
    participant Task as MeasurementsManager::task_loop()
    participant RTOS as FreeRTOS

    Caller->>API: environment_measurements_stop()
    API->>Manager: s_measurements_manager.stop()
    Manager->>Manager: stop_requested_.store(true)
    Manager->>RTOS: xTaskNotifyGive(task_)
    Manager->>RTOS: xSemaphoreTake(stopped_, portMAX_DELAY)
    Task->>Task: upptäcker stop_requested_
    Task->>RTOS: xSemaphoreGive(stopped_)
    Task->>RTOS: ulTaskNotifyTake(pdTRUE, portMAX_DELAY)
    Manager->>Manager: running_ = false
```

Stoppet är kooperativt. Tasken väcks direkt om den väntar mellan mätningar, bekräftar stoppet och väntar sedan på en ny notifiering från `MeasurementsManager::start()`. Taskens minne skapas alltså bara en gång.

### Kanaler och framtida utveckling

Kanalerna är ett billigt sätt att minska kopplingen mellan hårdvara och konsumenter. De vanligaste tänkbara utvecklingsvägarna är:

1. En separat, noggrannare temperatursensor producerar en egen temperaturkanal. C-API:t kan då välja den temperaturen tillsammans med luftfuktighet och tryck från BME280.
2. En andra BME280 utomhus producerar separata utomhuskanaler.
3. BME280 ersätts av annan hårdvara, men en ny producent fortsätter publicera samma logiska kanaler.

Detta är designmöjligheter, inte nuvarande funktionalitet. Nuvarande `MeasurementsManager::registrations_are_valid()` förbjuder att två producenter äger samma kanal. För en alternativ temperaturkälla behövs därför en egen kanal, och det framtida urvalet skulle ske i C-API:t.

Batchar och kanaler gör det även enklare att i framtiden skicka mätdata till en kö för samlad SD-kortsskrivning eller API-uppladdning. I nuvarande implementation exponeras dock endast senaste värdet genom C-API:t.

### Samtidighet och dataintegritet

Miljömodulen använder flera mekanismer med olika ansvar:

- En enda manager-task pollar alla producenter sekventiellt. Producenterna behöver därför inte skydda sig mot samtidig pollning från manager-tasken.
- `MeasurementStore` använder mutex för atomisk publicering, invalidering och kopiering av flera kanaler.
- `stop_requested_` är atomisk och kommunicerar stoppbegäran mellan anropande task och manager-task.
- Task notifications används både för tidsstyrd väntan och för att väcka tasken vid stopp eller återstart.
- En binär semafor bekräftar att stoppbegäran har observerats.
- Den underliggande I2C-modulen serialiserar varje busstransaktion.

Sensorläsningarna blockerar miljömodulens egen polling-task medan de pågår. Det påverkar därför normalt inte andra tasks, och tasken väntar utan att använda CPU mellan mätningarna. Publika anrop som hämtar senaste mätningen behöver bara vänta kort på store-mutexen medan en snapshot kopieras.

### Test

Unity-testet `"sim producer returns three measurements"` initierar `SimProducer`, läser en batch och kontrollerar att:

- anropet lyckas,
- batchen innehåller exakt tre mätningar.

Testet är litet och hårdvaruoberoende. Det bevisar producentkontraktets grundform, men kontrollerar ännu inte kanalernas identiteter eller värden. Det täcker inte heller manager-taskens beteende, store-mutexen, färskhetskontrollen, BME280-konverteringen eller återhämtning efter fel.

### Reflektion

Den största förbättringen jämfört med den tidigare lösningen är att modulen inte längre antar att alla sensorer producerar exakt samma fasta tre fält. Sensor, produktion, lagring och publik representation är separata beslut.

Samtidigt har flexibiliteten en kostnad. Fler lager och typer gör koden större och kräver tydligare dokumentation. Mitt mål var därför att ta minsta möjliga steg från en helt rigid modell, styrd av BME280-sensorns form, till en lösning som är enklare att bygga ut och testa. Kanalmodellen och producentinterfacet ger denna flexibilitet till en billig kostnad.

Det var svårt att så sent i kursen ersätta en fungerande C-lösning och gå över till C++. På tidigare lärares råd hade jag främst fokuserat på C, så övergången till C++ blev ganska brutal för mig. Jag har arbetat lite med språket i min ungdom, men uppförsbacken var ändå brant och jag känner mig fortfarande inte helt säker på all C++-syntax som jag har använt. Det kändes riskabelt att börja om när tiden var begränsad, särskilt eftersom jag förstod koncepten bakom interface och klasser bättre än jag behärskade syntaxen. Utan återkommande hjälp från AI med syntax och språkdetaljer hade jag sannolikt inte hunnit slutföra omskrivningen.

I efterhand tycker jag ändå att bytet förbättrade lösningen. Producentinterfacet gjorde beroendena tydligare och gjorde det möjligt att behandla verkliga och simulerade sensorer på samma sätt. Abstraktionen har en viss kostnad, men jag bedömer den som försumbar i detta fall jämfört med I2C-bussens hastighet och det långa intervallet mellan mätningarna.

Det designbeslut jag är mest nöjd med är balansen som kanalmodellen ger. Producenterna översätter sensorspecifika avläsningar till generella kanaler. Därför kan modulens centrala delar, som `MeasurementsManager` och `MeasurementStore`, i stor utsträckning förbli oförändrade när nya sensorer eller mätvärden tillkommer. Förändringarna hamnar främst vid modulens kanter: i hårdvarudrivrutiner och producenter som skapar kanalerna samt i C-API-funktioner som väljer och kombinerar dem för olika konsumenter.

---

## Slutsats

De tre modulerna jag har byggt visar samma grundidé på olika nivåer: en modul ska äga sitt tillstånd, skydda sina resurser och exponera ett begränsat kontrakt.

NVS-modulen centraliserar lagringspolicy. `board_i2c` centraliserar ägarskap och icke-samtidig åtkomst till den fysiska bussen. Miljömodulen separerar hårdvaruprotokoll, datakällor, schemaläggning, lagring och publik representation.

Den viktigaste lärdomen från arbetet är att hitta en balans mellan framtidssäkring och att först få något fungerande. Ett arbetssätt som fungerade bra för mig var att först lösa det aktuella problemet och därefter fråga vilka små och billiga generaliseringar som kunde förbättra testbarheten eller stödja sannolika nästa utvecklingssteg. På så sätt behövde jag inte försöka utforma hela lösningen i förväg, utan designen kunde växa fram iterativt medan jag själv lärde mig området. Kanalerna, producentgränssnittet och C-API:ts urvalspunkt är exempel på gränser som växte fram genom detta arbetssätt.

Arbetet har även gjort skillnaden mellan datadelning och dataägarskap tydligare för mig. Genom copy-out kan modulen behålla ägarskapet, skydda en snapshot kortvarigt och sedan låta konsumenten arbeta självständigt med sin kopia.

Jag har också lärt mig att synkronisering måste placeras där den delade resursen ägs. I2C-bussen och senaste mätdata är två olika delade resurser och behöver därför olika skydd. Ett enda globalt lock hade varit enklare att beskriva, men hade gett sämre ansvarsfördelning och onödig blockering.

Jag gick in i kursen utan tidigare kunskap om inbyggda system och har byggt upp en betydligt större förståelse för området. Att i ett verkligt projekt få designa en relativt avancerad C++-modul som hanterar hårdvara, tasks, synkronisering och fel, men samtidigt erbjuder ett enkelt API till resten av programmet, var särskilt utvecklande. Till nästa projekt tar jag framför allt med mig vikten av att tidigt identifiera ansvar och ägarskap, men också att låta designen utvecklas stegvis utifrån verkliga behov i stället för att försöka förutse allt från början.
