# Manus till Jimmy-presentationen

## Inledning

Jag har arbetat med tre moduler: `rm_nvs`, `board_i2c` och
`environment_measurements`.

Min röda tråd har varit att varje modul ska äga en resurs eller ett tydligt
ansvarsområde. Sedan försöker jag dölja implementationen bakom ett så litet
interface som möjligt.

Då får vi EN tydlig plats för kontroll, logging, ändringar och felsökning.
Applikationslagret kan bara anropa enkla funktioner för det den behöver.

## Slide 1: `rm_nvs`

`rm_nvs` är en wrapper ovanpå ESP-IDF:s NVS, alltså flash-lagring för
inställningar.

Om varje modul använder ESP-IDF:s API direkt får vi
snabbt flera olika sätt att öppna handles, välja namespace, göra commit och
hantera fel. Därför samlar wrappern den hanteringen på ett ställe.

I vår modul är namespace centralt definierat och API:t är uppdelat i typade
funktioner, till exempel för olika värdetyper. Det gör att vi kan validera
argument innan vi skriver och att resten av applikationen kan lita på
NVS-värdena.

Vi skriver ganska sällan till NVS, så vi behöver inte optimera med batching nu.
Det viktiga är att alla skrivningar går genom samma modul. Om vi senare vill
lägga till batching, loggning eller statistik över flash-skrivningar finns det
då en tydlig och plats att göra det på.

## Slide 2: `board_i2c`

`board_i2c` äger åtkomsten till kortets gemensamma I2C-buss.

På bussen finns flera enheter, till exempel BME280-sensorn och
touch-kontrollern. Modulen äger inte enheterna och den känner inte till deras
protokoll. Den äger bara transporten och ser till att åtkomsten sker på ett
kontrollerat sätt.

En enhetsdriver registrerar sin device på bussen och använder sedan
funktioner för till exempel registerläsning och registerskrivning.

Det viktiga är att bara en transaktion använder bussen åt gången. Om två delar
av systemet försöker prata över I2C samtidigt kan värden bli korrupta eller
transaktioner störa varandra. Därför validerar `board_i2c` anropet, tar ett lås,
låter ESP-IDF-drivern utföra transaktionen och släpper sedan låset.

Samma ägarskapsidé finns här: en delad fysisk resurs får en tydlig
ägare, och framtida loggning eller felsökning kan läggas på ett ställe.

## Slide 3: `environment_measurements`

Den största modulen är `environment_measurements`. Den äger flödet från lokal
miljösensor till användbar mätdata.

Resten av applikationen ska inte behöva känna till BME280-register,
kalibreringsformler eller hur polling fungerar. Den ska bara kunna fråga efter
senaste giltiga mätdata via modulens smala C-API.

Internt finns en `MeasurementsManager` som äger polling-tasken. Den läser med
intervallet inställt från menuconfig och anropar en producent. Just nu är producenten
`Bme280Producer`, som i sin tur använder `Bme280Sensor`-drivern.

`Bme280Sensor` hanterar det hårdvaruspecifika: registerläsning över I2C,
kalibreringsdata och kompensation av råvärden. Efter det översätter
`Bme280Producer` resultatet till en `MeasurementBatch` med logiska mätkanaler,
till exempel temperatur, luftfuktighet och tryck.

Den här designen kom efter en första mer direkt C-lösning. Den fungerade, men
den var hårt formad runt BME280 och en fast sample-struct. Därför flyttade jag
ansvaret från "läs den här sensorn" till "publicera miljömätningar som logiska
kanaler".

Vi lagrar inte en BME280-formad struct i hela systemet. I stället bryts värdena
upp i generiska kanaler.`MeasurementStore` lagrar senaste värdet och metadata per kanal.

Det här är inte tänkt som en stor generell abstraktion. Det är snarare minsta
 rimliga steget efter en helt hårdkodadvBME280-driver. Så fort vi vill kunna
 simulera sensorn, byta sensor, lägga till
en mer exakt temperatursensor eller kombinera intern och extern temperatur
behöver vi ändå separera fysisk källa från logisk mätdata. Kanalerna gör det på
ett billigt sätt.

Alla producenter följer samma C++ `MeasurementProducer`-interface. Det betyder att
den riktiga BME280-producenten och en simulerad producent kan användas genom
samma pipeline. Manager och store behöver inte veta om datan kommer från
hårdvara eller simulering.

När batchen kommer tillbaka validerar managern den, sätter en monoton
tidsstämpel och publicerar den till `MeasurementStore`. Store uppdaterar
kanalerna under samma mutex, så mätvärden från samma avläsning hålls ihop.

Vid C-API-gränsen väljer vi sedan vilka kanaler som ska kombineras till den
datastruktur som konsumenten behöver. Just nu är det ett indoor snapshot med
temperatur, luftfuktighet och tryck. Men internt är lagringen inte hårt låst
till just BME280 eller exakt den publika strukturen.
