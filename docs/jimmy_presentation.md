# Manus till gruppresentation

## Slide 1: `rm_nvs`

`rm_nvs` ger resten av applikationen ett gemensamt API för beständig lagring.
ESP-IDF har redan stöd för NVS, men kräver att anroparen själv hanterar
namespace, handles, commit och felkoder. Wrappern samlar den hanteringen på ett
ställe.

Diagrammet visar en skrivning av ett typat värde. Anroparen skickar bara en
nyckel och ett värde. `rm_nvs` öppnar applikationens gemensamma namespace,
skriver värdet, gör commit och stänger sedan handlet innan resultatet
returneras.

Varje operation öppnar och stänger ett eget handle, och varje lyckad skrivning
committas direkt. Det innebär lite mer arbete och fler skrivningar till flash,
men modulen används för enstaka inställningar och inte för tidskritisk data.
Designen prioriterar därför ett enkelt API och ett förutsägbart beteende.

## Slide 2: `board_i2c`

`board_i2c` äger kortets gemensamma I2C-buss. Flera enheter använder samma
fysiska buss, exempelvis BME280, touch-kontrollern och IO-expandern. Genom att
samla bussens konfiguration, livscykel och synkronisering i en modul undviker vi
att varje enhetsdrivrutin försöker kontrollera samma resurs.

Diagrammet visar en registerläsning från BME280. Sensordrivern skickar sitt
device-handle, registeradressen och en mottagarbuffert till
`board_i2c_read_reg`. Funktionen validerar argumenten och tar ett lås innan
ESP-IDF-drivern startar den fysiska transaktionen. När transaktionen är klar
släpps låset och resultatet returneras till sensordrivern.

Låset gör att endast ett publikt wrapper-anrop använder bussen åt gången.
Enhetsspecifika detaljer, som vilka register BME280 använder, ligger däremot
kvar i sensordrivern. `board_i2c` ansvarar alltså för transporten, men inte för
sensorprotokollet.

## Slide 3: `environment_measurements` – producenter och kanaler

Den här sliden visar den viktigaste abstraktionen i miljömodulen: gränsen mellan
en producent och resten av mätflödet.

`Bme280Producer` och `SimProducer` hämtar sina värden på olika sätt, men följer
samma `MeasurementProducer`-kontrakt. Resten av miljömodulen behöver därför
inte veta om datan kommer från riktig hårdvara eller en simulator.

Producentens ansvar är att returnera en `MeasurementBatch` med logiska kanaler.
Manager-tasken pollar producenterna sekventiellt och publicerar en lyckad batch
till `MeasurementStore`. Hela batchen publiceras under samma mutex, så att ingen
konsument kan få en blandning av gamla och nya värden från samma mättillfälle.

Store lagrar samtidigt senaste värdet separat för varje logisk kanal. Det gör
att polling, felhantering och lagring inte behöver vara bundna till en viss
sensor eller en fast datastruktur.

## Slide 4: `environment_measurements` – data ut

Den här sliden visar hur resten av applikationen läser miljödatan utan att
behöva känna till producenter, kanaler eller de interna C++-klasserna.

GUI eller UART anropar `environment_measurements_get_latest` genom modulens
C-API. Funktionen ber `MeasurementStore` att kopiera temperatur-, fukt- och
tryckkanalerna. Kopieringen görs under store-mutexen, vilket ger en
sammanhängande snapshot av de tre senaste kanalvärdena.

C-API:t kontrollerar att värdena är giltiga och fortfarande färska. Därefter
byggs en enkel `environment_measurement_sample_t`, som returneras till
anroparen.

Designen ger en tydlig gräns mellan modulens interna representation och resten
av systemet. Internt kan modulen arbeta med producenter och logiska kanaler,
medan GUI och UART får en liten, stabil C-struktur som är enkel att använda.
