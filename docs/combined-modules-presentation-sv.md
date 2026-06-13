# Presentation: NVS, I2C och Environment Measurements

Längd: cirka 4-5 minuter  
Antal bilder: 2  
Huvudbudskap: Modulerna visar hur tydligt ägarskap och kontrollerad dataåtkomst
gör ett inbyggt system robust och möjligt att bygga ut.

## Bild 1: Kontrollerad åtkomst till resurser

```text
Applikation
   |
   +--> rm_nvs --> ESP-IDF NVS --> flash
   |
   +--> board_i2c --> delad I2C-buss
                         |
                         +--> touch
                         +--> IO-expander
                         +--> BME280
```

- `rm_nvs` ger ett gemensamt API för beständig lagring.
- `board_i2c` äger den delade bussen.
- Åtkomst och livscykel är synkroniserade.
- Hårdvarudetaljer hålls borta från övriga moduler.

## Bild 2: Environment-modulens datamodell

```text
Fysisk sensor
     |
     v
Sensor driver
     |
     v
Producer --> MeasurementBatch --> Manager --> Store --> Publikt API
                |
                +--> en eller flera logiska channels
```

- Sensorer ansvarar för sin specifika hårdvara.
- Producers översätter hårdvara till generiska channels.
- Manager ansvarar för polling och felhantering.
- Store publicerar en sammanhängande senaste ögonblicksbild.
- Channels gör framtida sensorer, SD-kortslagring och API-uppladdning enklare.

## Svenskt Talmanus

### Inledning

"Jag har arbetat med tre moduler: NVS, board I2C och environment measurements.

De löser olika problem, men de bygger på samma designidé: en modul ska ha ett
tydligt ansvar, äga sin data eller resurs och kontrollera hur andra delar av
systemet får tillgång till den.

Det minskar kopplingar mellan moduler och förhindrar samtidig åtkomst som kan
skapa race conditions eller inkonsekvent data."

### NVS

"NVS-modulen är den enklaste av de tre och består till stor del av boilerplate
runt ESP-IDF:s NVS-funktioner.

I stället för att alla delar av applikationen själva öppnar NVS-handtag och
hanterar namespaces, erbjuder modulen ett gemensamt C-API för heltal, strängar
och binär data.

Modulen äger applikationens namespace, validerar argument och gör commit efter
varje lyckad skrivning eller radering. Ett livscykellås skyddar initiering,
avinitiering och namespace, medan ESP-IDF synkroniserar själva NVS-operationerna.

Poängen är att samla lagringsansvaret bakom ett stabilt interface."

### Board I2C

"Board I2C-modulen äger den delade I2C-bussen. På bussen finns flera enheter,
bland annat touch-kontrollern, IO-expandern och BME280-sensorn.

Eftersom bussen är en gemensam fysisk resurs får inte flera transaktioner ske
samtidigt. Modulen använder därför ett lås runt sina wrapper-operationer och
ESP-IDF:s I2C-driver serialiserar de fysiska busstransaktionerna.

Modulen ansvarar för initiering, adresser, probes samt läs- och
skrivoperationer. Enhetsspecifik logik ligger däremot kvar i respektive driver.

Det ger en tydlig gräns: board I2C äger och skyddar transporten, medan
sensor-drivern äger protokollet för sin hårdvara."

### Environment Measurements

"Environment-modulen är uppdelad eftersom den arbetar med hårdvara,
schemaläggning och delad data.

Före environment-modulen hade vi en helt C-baserad lösning uppdelad i tre
komponenter: en BME280-HAL, en polling-service och ett separat sensor-data-lager.
Den fungerade, men hela kedjan utgick från en BME280-sample med exakt
temperatur, luftfuktighet och tryck. Polling-servicen kopierade dessutom
värdena manuellt mellan två nästan identiska structar.

Jag ersatte därför kedjan med en sammanhållen C++-modul som äger hela
ansvarsområdet och uttrycker sensorer, producers och channels tydligare.

Det handlar inte om att C är olämpligt, utan om att C++ gav bättre verktyg för
den här designen. Modulen använder fortfarande fast lagring utan dynamisk
allokering och exponerar samma lilla C-API till resten av applikationen.

Först finns de faktiska sensor-drivrarna. Bme280Sensor känner till register,
chip-ID, kalibreringsdata och kompensationsformler.

Ovanpå sensorn finns en producer. En producer kan använda vilken hårdvara som
helst. Dess enda kontrakt är att returnera en batch med en eller flera logiska
measurement channels.

En channel beskriver betydelsen av ett värde, exempelvis inomhustemperatur
eller lufttryck. Resten av systemet behöver därför inte vara låst till formen
på en BME280-sensor.

MeasurementsManager pollar alla registrerade producers från en FreeRTOS-task.
Vid en lyckad läsning publiceras hela batchen. Vid ett fel markeras producentens
channels som ogiltiga, så att resten av systemet inte använder gamla värden som
om de fortfarande vore aktuella.

MeasurementStore skyddas av en mutex. En hel batch skrivs och läses atomiskt,
vilket gör att konsumenter får en sammanhängande ögonblicksbild."

### Framtida utbyggnad

"Channels är framför allt ett billigt sätt att förbereda för de vanligaste
utbyggnaderna.

Om vi byter sensorhårdvara skriver vi en ny driver och producer som publicerar
samma channels. Manager, store och publikt API kan fortsätta fungera.

Om vi lägger till en utomhus-BME kan den publicera nya utomhus-channels genom
samma pipeline.

Som en teoretisk framtida utbyggnad kan en separat, mer exakt temperatursensor
publicera sin egen temperatur-channel. Urvalet skulle då ske precis innan data
formas till den publika C-API-samplen. Där kan vi välja den mer exakta
temperaturen tillsammans med luftfuktighet och lufttryck från BME-sensorn.

Det är en viktig del av designen: channels beskriver tillgänglig data, medan
det publika API-lagret bestämmer vilka channels som ska kombineras och exponeras
för en viss konsument. Den urvalslogiken är inte implementerad idag.

I dag exponeras bara det senaste kombinerade mätvärdet. Channels och batches
gör det även enklare att senare skicka data till SD-kort eller ett API."

### Tester och avslutning

"Jag lade även till ett enkelt Unity-test för varje modul.

NVS-testet skriver, läser, verifierar och raderar ett värde i flash.
I2C-testet verifierar att ogiltiga argument och adresser avvisas innan bussen
används. Environment-testet verifierar att den simulerade producenten returnerar
en komplett batch med tre channels.

Min viktigaste lärdom från de här modulerna är att bra embedded-design inte bara
handlar om att få hårdvaran att fungera. Det handlar också om att bestämma vem
som äger resurser och data, kontrollera samtidig åtkomst och skapa gränssnitt
som kan byggas ut utan att resten av systemet måste skrivas om. Omskrivningen
från C till C++ var ett konkret exempel på det."

## Kort Frågeunderlag

**Hur väljs värden från flera sensorer?**  
Det är en teoretisk framtida utbyggnad. Tanken är att varje producer publicerar
sina egna channels och att urval och kombination sker vid C-API-gränsen.

**Varför används en mutex i MeasurementStore?**  
För att en hel batch ska kunna publiceras och kopieras som en sammanhängande
ögonblicksbild utan race conditions.

**Varför ligger inte BME280-logiken i board I2C?**  
Board I2C ansvarar bara för den delade transporten. BME280-drivern ansvarar för
det enhetsspecifika protokollet.

**Varför ersattes den gamla C-lösningen med environment-modulen i C++?**  
Den gamla lösningen delade upp samma ansvar mellan BME280-HAL, polling-service
och sensor-data. Dataformen var fortfarande låst till tre BME280-värden och
kopierades manuellt mellan nästan identiska structar. C++-modulen samlar
ansvaret, uttrycker producers och channels tydligare, behåller ett stabilt
C-API och använder ingen dynamisk allokering i mätflödet.

**Varför är NVS-modulen värdefull om den mest är boilerplate?**  
Den centraliserar lagringspolicy, namespace, validering och commit-beteende
bakom ett stabilt API.
