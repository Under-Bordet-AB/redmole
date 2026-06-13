# Manus till Jimmy-presentationen

## Inledning

Jag kommer att visa tre moduler som jag har arbetat med: `rm_nvs`, `board_i2c`
och `environment_measurements`.

De löser olika problem, men bygger på samma grundidé: varje modul ska ha ett
tydligt ansvar och kontrollera åtkomsten till den resurs eller data som den
äger. Det gör beteendet mer förutsägbart och minskar kopplingen mellan
modulerna.

## Slide 1: `rm_nvs`

`rm_nvs` ger resten av applikationen ett litet och enhetligt API för beständig
lagring.

ESP-IDF erbjuder redan NVS, men anroparen måste själv hantera namespace,
handles, commit och felkoder. Wrappern samlar den hanteringen på ett ställe.

Diagrammet visar en skrivning. Anroparen skickar bara en nyckel och ett värde.
`rm_nvs` öppnar applikationens namespace, skriver värdet, gör commit och stänger
sedan handlet innan resultatet returneras.

Varje operation öppnar och stänger sitt eget handle. Det innebär lite extra
arbete, men modulen används för enstaka inställningar och inte för tidskritisk
data. Designen prioriterar därför tydligt ägarskap och förutsägbart beteende.

## Slide 2: `board_i2c`

`board_i2c` äger kortets gemensamma I2C-buss. Flera enheter använder samma
fysiska buss, bland annat BME280, touch-kontrollern och IO-expandern.

Diagrammet visar en registerläsning från BME280. Enhetsdrivern anropar
`board_i2c_read_reg`. Modulen validerar argumenten och tar ett lås innan
ESP-IDF-drivern utför den fysiska transaktionen. När transaktionen är klar
släpps låset och resultatet returneras.

Låset hindrar flera wrapper-anrop från att använda den delade bussen samtidigt.
`board_i2c` ansvarar för transporten, medan enhetsdrivern fortfarande ansvarar
för sitt eget protokoll och sina register.

## Slide 3: `environment_measurements`

Den här sliden följer datan från sensorn tills den ligger säkert i modulens
store. Hur andra moduler sedan hämtar datan är relativt enkelt och visas därför
inte här.

`MeasurementsManager` äger en FreeRTOS-task som startar varje avläsning med ett
konfigurerbart intervall. Managern anropar `Bme280Producer`, som i sin tur
anropar `Bme280Sensor`.

Sensorklassen hanterar den hårdvaruspecifika delen. Den läser rå temperatur,
luftfuktighet och lufttryck över I2C och använder sensorns kalibreringsdata för
att kompensera värdena.

Producenten översätter sedan den BME280-specifika avläsningen till en
`MeasurementBatch` med tre logiska kanaler. Det skiljer den fysiska sensorn från
resten av mätflödet och gör att samma manager och store även kan användas med en
simulerad eller framtida producent.

Alla producenter följer samma `MeasurementProducer`-interface. Därför kan den
riktiga `Bme280Producer` ersättas med en simulerad producent utan att manager
eller store behöver ändras. Simulatorn returnerar bara logiska kanaler genom
samma interface som den riktiga producenten.

Det viktiga är att de tre värdena kommer från samma fysiska avläsning och
fortsätter genom systemet som en gemensam batch. Managern validerar batchen och
lägger till en monoton tidsstämpel. Därefter publicerar `MeasurementStore` alla
kanaler under samma mutex och ger dem samma publiceringsversion.

På så sätt kan ingen konsument senare få en ny temperatur kombinerad med gammal
luftfuktighet eller gammalt lufttryck.

Store känner inte till en färdig BME280-sample utan lagrar endast senaste värdet
och metadata för varje kanal. Vid C-API-gränsen väljer vi sedan vilka kanaler
som ska kombineras till den struktur som en viss konsument behöver. Det innebär
att den interna lagringen inte behöver ändras om ett framtida API vill kombinera
kanalerna på ett annat sätt.

## Avslutning

De tre modulerna visar samma designprincip på olika nivåer.

`rm_nvs` kontrollerar åtkomsten till beständig lagring. `board_i2c` äger och
skyddar den delade fysiska bussen. `environment_measurements` gör en osäker
sensoravläsning till en sammanhängande och giltig batch i ett trådsäkert store.

Min viktigaste lärdom är att hårdvaruavläsningen bara är en del av problemet.
Det som gör datan pålitlig för resten av systemet är tydligt ägarskap,
synkronisering och genomtänkt felhantering.
