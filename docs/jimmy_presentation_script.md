# Manus till Jimmy-presentationen

## Inledning

Jag har arbetat med tre moduler: `rm_nvs`, `board_i2c` och
`environment_measurements`.

Min tanke är att varje modul ska äga en resurs eller ett tydligt
ansvarsområde. Sedan försöker jag dölja implementationen bakom ett så litet
interface som möjligt.

Då får vi EN tydlig plats för kontroll, logging, ändringar och felsökning
och applikationslagret kan bara anropa enkla funktioner för det den behöver.

## Slide 1: `rm_nvs`

Om varje modul använder ESP-IDF:s API direkt får vi
snabbt flera olika sätt att öppna handles, välja namespace, göra commit och
hantera fel. Därför samlar vår `rm_nvs` wrapper den hanteringen på ett ställe.

Namespace är centralt definierat och API:t är uppdelat i typade
funktioner.  Det gör att vi kan validera
argument.

Vi skriver ganska sällan till NVS, så vi behöver inte optimera med batching nu.
Det viktiga är att alla skrivningar går genom samma modul. Om vi senare vill
lägga till batching, loggning eller statistik över flash-skrivningar finns det
då en tydlig och plats att göra det på.

## Slide 2: `board_i2c`

`board_i2c` äger åtkomsten till kortets gemensamma I2C-buss.

Modulen äger inte I2C-enheterna. Den äger projektets gemensamma gräns mot
bussen och ser till att åtkomsten sker på ett kontrollerat sätt.

Poängen är framför allt att varje modul inte ska initiera och konfigurera samma
fysiska buss själv. Det ska finnas en gemensam bus-instans och en gemensam
policy för hur devices på bussen hanteras.

Modulen initierar bussen, registrerar devices, validerar anrop och
håller livscykel och hjälpfunktioner samlade. Själva transaktionen lämnas sedan
till ESP-IDF-drivern.

Samma ägarskapsidé finns här: en delad fysisk resurs får en tydlig
ägare, och framtida utökning eller kontroll kan läggas på ett ställe.

## Slide 3: `environment_measurements`

Den största modulen är `environment_measurements`. Den äger flödet från lokal
miljösensor till användbar mätdata.

Resten av applikationen ska inte behöva känna till något om hårdvara eller polling.
Den ska bara kunna fråga efter senaste giltiga mätdata via modulens smala C-API.

Internt finns en `MeasurementsManager` som äger polling-tasken. Den läser med
intervallet inställningarna från menuconfig och anropar en producent. Just nu är producenten
`Bme280Producer`, som i sin tur använder `Bme280Sensor`-drivern.

Drivern hanterar det hårdvaruspecifika och 
Producern delar upp mätvärdena till en `MeasurementBatch` med separata mätkanaler. För bme280 blir det: temperatur, luftfuktighet och tryck.

Vid C-API-gränsen väljer vi sedan vilka kanaler som ska kombineras till den
datastruktur som konsumenten behöver. Just nu är det ett indoor snapshot med
temperatur, luftfuktighet och tryck. 

Detta är inte tänkt som en stor generell abstraktion. Det är tänkt som minsta
rimliga steget efter att bara "hårdkoda" hela systemet runt BME280 sensorn.

Så fort vi vill kunna simulera sensorn, byta sensor, lägga till en mer exakt separat temperatursensor eller flera olika sensorer behöver vi ändå separera fysisk källa från logisk mätdata.

Jag tycker att kanalerna gör det på ett billigt sätt.

Alla producenter följer samma C++ `MeasurementProducer`-interface. Det är så 
simulering och implementering av nya sensorer blir enkel.

DEt är även enkelt att i C API:et exponera nya kombinationer av kanaler till applagret.
