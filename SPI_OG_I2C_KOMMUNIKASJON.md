# 🔌 SPI OG I2C KOMMUNIKASJON - Komplett Norsk Kurs

## 📚 INNHOLDSFORTEGNELSE

1. [Grunnleggende Konsepter](#basics)
2. [SPI - Serial Peripheral Interface](#spi)
3. [I2C - Inter-Integrated Circuit](#i2c)
4. [Hvordan Koden Velger Hvem Som Får Snakke](#arbitration)
5. [Interrupt System og Prioriteter](#interrupts)
6. [Praktisk Eksempel: LCD og Sensorer](#practical)
7. [Køsystem og Databuffer](#queues)
8. [Feilsøking](#troubleshooting)

---

## 🌟 1. GRUNNLEGGENDE KONSEPTER {#basics}

### **Hva er SPI og I2C?**

Tenk på det som **telefonlinjer** mellom mikrokontrolleren (MSPM0) og eksterne enheter:

```
Mikrokontroller (MSPM0G3507)
         │
    ┌────┴────┐
    │         │
   SPI       I2C
    │         │
    │         └─── OPT3001 (Lyssensor)
    │              TMP006 (Temperatursensor)
    │
    └─── LCD Display (ST7735)
```

**Forskjellen:**

| Feature | SPI | I2C |
|---------|-----|-----|
| **Hastighet** | Rask (opptil 16 MHz) | Middels (100-400 kHz) |
| **Ledninger** | Mange (4+) | Få (2) |
| **Enheter** | En master → flere slaves | En master → mange slaves |
| **Kompleksitet** | Enkel | Middels |
| **Bruk** | LCD, SD-kort | Sensorer, EEPROM |

---

## 📡 2. SPI - SERIAL PERIPHERAL INTERFACE {#spi}

### **2.1 Hvordan SPI Fungerer**

SPI er som en **rundkjøring** hvor master (MSPM0) kontrollerer trafikken:

```
Master (MSPM0)                   Slave (LCD)
┌──────────┐                    ┌──────────┐
│   MOSI   │──────────────────>│   MOSI   │  Data ut
│   MISO   │<──────────────────│   MISO   │  Data inn
│   SCLK   │──────────────────>│   SCLK   │  Klokke
│   CS     │──────────────────>│   CS     │  Velg slave
└──────────┘                    └──────────┘
```

**Analogi:** Tenk på det som en **telefonsamtale**:

1. **CS (Chip Select)** = Ringer opp enheten ("Hei, jeg vil snakke med deg!")
2. **SCLK (Clock)** = Takt/rytme for samtalen (når ord skal sendes)
3. **MOSI (Master Out Slave In)** = Du snakker
4. **MISO (Master In Slave Out)** = Enheten svarer

### **2.2 SPI Steg-for-Steg**

```c
// Eksempel: Send kommando til LCD

// STEG 1: Velg slave (CS = LOW)
DL_GPIO_clearPins(GPIOA, LCD_CS_PIN);  // "Ring opp" LCD
delayMicroseconds(1);  // Kort pause

// STEG 2: Sett modus (Data eller Command)
DL_GPIO_setPins(GPIOB, LCD_DC_PIN);    // DC=HIGH betyr "data"

// STEG 3: Send data via SPI
uint8_t data = 0xFF;  // Byte å sende
DL_SPI_transmitData8(SPI1_INST, data);

// STEG 4: Vent til sendt
while (DL_SPI_isBusy(SPI1_INST)) {
    // Venter...
}

// STEG 5: Deselect slave (CS = HIGH)
DL_GPIO_setPins(GPIOA, LCD_CS_PIN);    // "Legg på"
```

**Hva skjer internt:**

```
Tid:  0      1      2      3      4      5      6      7
      │      │      │      │      │      │      │      │
CS:   ─┐                                              ┌─
      └──────────────────────────────────────────────┘

SCLK: ─┐  ┌─┐  ┌─┐  ┌─┐  ┌─┐  ┌─┐  ┌─┐  ┌─┐  ┌─
      └──┘ └──┘ └──┘ └──┘ └──┘ └──┘ └──┘ └──┘

MOSI: ──1────1────1────1────1────1────1────1────
       Bit7 Bit6 Bit5 Bit4 Bit3 Bit2 Bit1 Bit0
       (0xFF = 11111111)

Data sendes bit-for-bit, synkronisert med klokken!
```

### **2.3 SPI i Ditt Prosjekt**

```javascript
// Din .syscfg konfigurasjon:
SPI1.$name = "SPI_LCD";
SPI1.targetBitRate = 10000000;  // 10 MHz (faktisk 8 MHz)

// Pins:
SPI1.peripheral.sclkPin = PB16;  // Klokke
SPI1.peripheral.mosiPin = PB15;  // Data ut
SPI1.peripheral.misoPin = PA16;  // Data inn (ikke brukt av LCD)
```

**I koden:**

```c
// Funksjon for å sende en byte til LCD:
void LCD_Send_Byte(uint8_t byte) {
    // 1. Velg LCD (CS low)
    DL_GPIO_clearPins(GPIOA, DL_GPIO_PIN_2);  // CS
    
    // 2. Send data
    DL_SPI_transmitData8(SPI1_INST, byte);
    
    // 3. Vent til ferdig
    while (DL_SPI_isBusy(SPI1_INST));
    
    // 4. Deselect LCD (CS high)
    DL_GPIO_setPins(GPIOA, DL_GPIO_PIN_2);
}

// Bruk:
LCD_Send_Byte(0x2A);  // Send kommando
LCD_Send_Byte(0x00);  // Send parameter
LCD_Send_Byte(0x7F);  // Send parameter
```

### **2.4 Hvorfor LCD Bruker SPI**

LCD trenger å sende **mye data raskt** (128x128 piksler = 16384 piksler!):

```
For å oppdatere hele skjermen:
├─ 128 × 128 piksler = 16384 piksler
├─ Hver piksel = 2 bytes (16-bit farge)
├─ Total = 32768 bytes
└─ Ved 8 MHz SPI: ~33 ms for full oppdatering
```

**SPI er perfekt for dette:** Rask, enkel, én-til-én kommunikasjon.

---

## 🔗 3. I2C - INTER-INTEGRATED CIRCUIT {#i2c}

### **3.1 Hvordan I2C Fungerer**

I2C er som en **buss** hvor alle enheter deler samme ledninger:

```
Master (MSPM0)
     │
     ├─── SDA (Data) ───┬─── OPT3001 (Addr: 0x44)
     │                  │
     ├─── SCL (Clock) ──┴─── TMP006 (Addr: 0x40)
     │
    GND
```

**Analogi:** Tenk på det som en **gruppesamtale** på én telefonlinje:

1. **Master** (MSPM0) er samtaleleder
2. **Slaves** (sensorer) lytter til sin **adresse**
3. Når master sier "0x44!" svarer bare OPT3001
4. Alle deler samme **SDA** (data) og **SCL** (klokke)

### **3.2 I2C Addressing (Adresser)**

Hver enhet har en **unik 7-bit adresse**:

```
OPT3001 Light Sensor:  0x44 (0100 0100)
TMP006 Temp Sensor:    0x40 (0100 0000)
```

**Hvordan master velger slave:**

```
Master sender: [START] [0x44] [R/W bit] [DATA] [STOP]
                        ↑
                  Kun OPT3001 svarer!
```

### **3.3 I2C Protokoll Steg-for-Steg**

```c
// Eksempel: Les lys-verdi fra OPT3001

// STEG 1: Send START condition
// (Master "ringer opp" bussen)
DL_I2C_startControllerTransfer(I2C0_INST, 0x44, ...);

// STEG 2: Send slave adresse + WRITE bit
// "Hei 0x44, jeg vil snakke med deg!"
// OPT3001: "Ja, jeg hører!"

// STEG 3: Send register adresse
// "Jeg vil lese fra register 0x00 (resultat)"
uint8_t reg = 0x00;
DL_I2C_fillControllerTXFIFO(I2C0_INST, &reg, 1);

// STEG 4: Send REPEATED START + READ bit
// "Nå vil jeg høre hva du har å si"
DL_I2C_startControllerTransfer(I2C0_INST, 0x44, DL_I2C_CONTROLLER_DIRECTION_RX, ...);

// STEG 5: Les data
uint8_t data[2];
while (!finished) {
    data[0] = DL_I2C_receiveControllerData(I2C0_INST);
    data[1] = DL_I2C_receiveControllerData(I2C0_INST);
}

// STEG 6: Send STOP condition
// "Takk for nå, legger på!"
DL_I2C_stopControllerTransfer(I2C0_INST);
```

**Hva skjer på bussen:**

```
SDA:  S│0│1│0│0│0│1│0│0│W│A│0│0│0│0│0│0│0│0│A│Sr│...│P
      │└────Addr 0x44────┘│ │└──Reg 0x00──┘│ │  ...
      START              ACK             ACK REPEATED-START  STOP

S  = START condition
Sr = REPEATED START
A  = ACK (acknowledge)
P  = STOP condition
W  = Write bit (0)
R  = Read bit (1)
```

### **3.4 I2C Pull-up Resistorer (VIKTIG!)**

I2C **krever** pull-up resistorer på SDA og SCL:

```
       3.3V
        │
       [R]  Pull-up resistor (4.7 kΩ)
        │
        ├─── SDA ───┬─── Device 1
        │           └─── Device 2
        
       [R]  Pull-up resistor (4.7 kΩ)
        │
        └─── SCL ───┬─── Device 1
                    └─── Device 2
```

**På LaunchPad:** Pull-ups er på J19/J20 jumpers (MÅ være installed!)

**Hvorfor?** I2C bruker **open-drain** outputs:
- Device kan dra linjen LOW (0V)
- Device kan slippe linjen (→ pull-up drar den HIGH til 3.3V)
- Device kan IKKE aktivt drive linjen HIGH

### **3.5 I2C i Ditt Prosjekt**

```javascript
// Din .syscfg konfigurasjon:
I2C1.$name = "I2C_SENSORS";
I2C1.basicEnableController = true;
I2C1.basicControllerBusSpeed = 400000;  // 400 kHz (Fast Mode)

// Pins:
I2C1.peripheral.sdaPin = PA0;  // Data
I2C1.peripheral.sclPin = PA1;  // Clock
```

**Lesing fra OPT3001:**

```c
// Funksjon for å lese lys-intensitet:
uint16_t OPT3001_Read_Light(void) {
    uint8_t reg_addr = 0x00;  // Result register
    uint8_t data[2];
    
    // 1. Skriv register adresse
    DL_I2C_startControllerTransfer(
        I2C0_INST,
        OPT3001_ADDR,  // 0x44
        DL_I2C_CONTROLLER_DIRECTION_TX,
        1  // Send 1 byte
    );
    DL_I2C_fillControllerTXFIFO(I2C0_INST, &reg_addr, 1);
    
    // Vent til sendt
    while (DL_I2C_getControllerStatus(I2C0_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS);
    
    // 2. Les data (2 bytes)
    DL_I2C_startControllerTransfer(
        I2C0_INST,
        OPT3001_ADDR,
        DL_I2C_CONTROLLER_DIRECTION_RX,
        2  // Les 2 bytes
    );
    
    // Les bytes
    data[0] = DL_I2C_receiveControllerData(I2C0_INST);
    data[1] = DL_I2C_receiveControllerData(I2C0_INST);
    
    // 3. Kombiner til 16-bit verdi
    uint16_t result = (data[0] << 8) | data[1];
    return result;
}
```

---

## 🚦 4. HVORDAN KODEN VELGER HVEM SOM FÅR SNAKKE {#arbitration}

### **4.1 SPI: Master Styrer Alt**

Med SPI er det **INGEN kø** - master bestemmer alt!

```c
// Master (MSPM0) bestemmer:

// Tid 0-10 ms: Snakk med LCD
DL_GPIO_clearPins(GPIOA, LCD_CS_PIN);  // Velg LCD
send_lcd_data();
DL_GPIO_setPins(GPIOA, LCD_CS_PIN);    // Deselect LCD

// Tid 10-20 ms: Ikke snakk med noen (pause)
delay_ms(10);

// Tid 20-30 ms: Snakk med LCD igjen
DL_GPIO_clearPins(GPIOA, LCD_CS_PIN);
send_lcd_data();
DL_GPIO_setPins(GPIOA, LCD_CS_PIN);
```

**Diagram:**

```
Tid:    0ms      10ms     20ms     30ms
        │        │        │        │
LCD_CS: └────────┐        └────────┐
        Selected │ Idle   Selected │ Idle
        
SPI:    [DATA]   [IDLE]   [DATA]   [IDLE]
```

**Ingen kø, ingen konkurranse - master er boss!**

### **4.2 I2C: Delt Buss med Adresser**

Med I2C deler alle **samme ledninger**, men kun én snakker om gangen:

```c
// Scenario: Les begge sensorer

// 1. Les OPT3001 (adresse 0x44)
DL_I2C_startControllerTransfer(I2C0_INST, 0x44, ...);
// → Kun OPT3001 svarer!
uint16_t light = read_data();
DL_I2C_stopControllerTransfer(I2C0_INST);

// 2. Kort pause (optional)
delay_us(100);

// 3. Les TMP006 (adresse 0x40)
DL_I2C_startControllerTransfer(I2C0_INST, 0x40, ...);
// → Kun TMP006 svarer!
uint16_t temp = read_data();
DL_I2C_stopControllerTransfer(I2C0_INST);
```

**Diagram:**

```
Tid:    0ms           5ms           10ms
        │             │             │
SDA:    [0x44 DATA]   [0x40 DATA]   [IDLE]
        OPT3001       TMP006        
        
        START──────STOP─START──────STOP
```

**Hvordan enheter "vet" det er dem:**

```
Master sender på buss: [START] [0│1│0│0│0│1│0│0] [W/R]
                                └────0x44────┘

OPT3001 (0x44): "Det er meg! Jeg svarer!"
TMP006  (0x40): "Ikke meg, jeg venter stille..."
```

### **4.3 Praktisk Eksempel: Oppdater LCD og Les Sensorer**

```c
void main_loop(void) {
    while(1) {
        // 1. Les sensorer (I2C)
        uint16_t light = OPT3001_Read_Light();      // I2C til 0x44
        delay_us(100);  // Kort pause mellom I2C transaksjoner
        uint16_t temp = TMP006_Read_Temperature();  // I2C til 0x40
        
        // 2. Oppdater LCD (SPI)
        LCD_Clear();                          // SPI til LCD
        LCD_DrawString(10, 10, "Light:");     // SPI til LCD
        LCD_DrawNumber(70, 10, light);        // SPI til LCD
        LCD_DrawString(10, 30, "Temp:");      // SPI til LCD
        LCD_DrawNumber(70, 30, temp);         // SPI til LCD
        
        // 3. Vent før neste iterasjon
        delay_ms(100);
    }
}
```

**Tid-linje:**

```
0ms:    START I2C → OPT3001 (0x44) → Read light → STOP
5ms:    PAUSE (100 µs)
5.1ms:  START I2C → TMP006 (0x40) → Read temp → STOP
10ms:   SPI CS=LOW → Send LCD commands → CS=HIGH
50ms:   SPI CS=LOW → Send more LCD data → CS=HIGH
100ms:  [Repeat cycle]
```

**INGEN kø eller konkurranse!** Master (koden din) bestemmer rekkefølgen!

---

## 🎯 5. INTERRUPT SYSTEM OG PRIORITETER {#interrupts}

### **5.1 Har SPI og I2C Interrupts?**

**JA!** Både SPI og I2C kan generere interrupts:

```javascript
// SysConfig konfigurasjon:
SPI1.interruptPriority = "2";  // Medium priority
SPI1.enabledInterrupts = ["TX_READY", "RX_READY"];

I2C1.interruptPriority = "3";  // Lower priority
I2C1.enabledInterrupts = ["CONTROLLER_TXFIFO_TRIGGER", "CONTROLLER_RXFIFO_TRIGGER"];
```

### **5.2 Når Brukes Interrupts?**

**Scenario A: Polling (Enkel, Blokkerer)**

```c
// Send data via SPI (blokkerer CPU)
void LCD_Send_Byte_Polling(uint8_t byte) {
    DL_SPI_transmitData8(SPI1_INST, byte);
    
    // Vent til ferdig (blokkerer!)
    while (DL_SPI_isBusy(SPI1_INST)) {
        // CPU venter her... kan ikke gjøre annet!
    }
}
```

**Scenario B: Interrupt-Driven (Avansert, Non-Blokkerende)**

```c
// Send data via SPI (non-blokkerende)
volatile bool spi_busy = false;

void LCD_Send_Byte_Interrupt(uint8_t byte) {
    spi_busy = true;
    DL_SPI_transmitData8(SPI1_INST, byte);
    // CPU kan gjøre annet mens SPI sender!
}

// SPI ISR kalles når byte er sendt
void SPI1_IRQHandler(void) {
    if (DL_SPI_getInterruptStatus(SPI1_INST) & DL_SPI_INTERRUPT_TX_DONE) {
        spi_busy = false;  // Sending ferdig!
    }
}

// Main loop kan gjøre andre ting
void main(void) {
    LCD_Send_Byte_Interrupt(0xFF);
    
    // Gjør annet arbeid mens SPI sender
    process_audio();
    read_buttons();
    
    // Sjekk om SPI ferdig
    if (!spi_busy) {
        // Neste byte...
    }
}
```

### **5.3 Interrupt Prioriteter i Ditt Prosjekt**

```
Priority 0 (Høyest)
    ↓
Priority 1 ← TIMER_SAMPLE (8 kHz audio - MÅ være nøyaktig!)
    ↓
Priority 2 ← ADC, SPI (viktig men ikke kritisk timing)
    ↓
Priority 3 ← I2C, GPIO buttons (lavest prioritet)
    ↓
Priority 7 (Lavest)
```

**Hva betyr dette?**

```c
// Hvis dette skjer:

void SPI1_IRQHandler(void) {  // Priority 2
    // SPI interrupt starter...
    process_spi_data();
    
    // PLUTSELIG: Timer interrupt (Priority 1) triggers!
    // → SPI ISR avbrytes (preempted)
    // → Timer ISR kjører
    // → Når timer ferdig, SPI ISR fortsetter
}

void TIMG7_IRQHandler(void) {  // Priority 1
    // Høyere prioritet - kan avbryte SPI!
    calculate_audio();
    update_pwm();
}
```

**Diagram:**

```
Tid:  0     5    10    15    20    25
      │     │     │     │     │     │
SPI:  ├─────────────────────────┐
      │ SPI ISR                 │
      │         │               │
Timer:│         ├─────┐         │
      │         │TIMER│ (avbryter SPI!)
      │         └─────┘         │
      └─────────────────────────┘
           SPI fortsetter
```

### **5.4 DMA: Enda Mer Effektivt!**

For **store dataoverføringer** (som LCD full-screen update), bruk DMA:

```javascript
// SysConfig DMA konfigurasjon:
DMA1.srcAddr = "&lcd_buffer[0]";
DMA1.destAddr = "SPI1_INST + DATA_REG";
DMA1.transferSize = 1024;  // 1 KB data
```

```c
// Send 1 KB til LCD uten CPU involvement!
void LCD_Update_Screen_DMA(uint8_t *buffer, uint16_t size) {
    // Start DMA transfer
    DL_DMA_startTransfer(DMA_CH0);
    
    // CPU er FRI mens DMA sender data!
    // Kan gjøre audio processing, ADC, etc.
    
    // DMA interrupt når ferdig
}

void DMA_IRQHandler(void) {
    // DMA ferdig - all data sendt!
    lcd_update_complete = true;
}
```

**Fordel:** CPU er 100% fri under data-transfer!

---

## 💼 6. PRAKTISK EKSEMPEL: LCD OG SENSORER {#practical}

### **6.1 Komplett System**

```c
// ============================================
// GLOBAL STATE
// ============================================
typedef struct {
    uint16_t light_lux;
    int16_t temperature_c;
    bool lcd_update_needed;
} System_State_t;

volatile System_State_t system_state = {0};

// ============================================
// SENSOR READING (I2C)
// ============================================
void Read_All_Sensors(void) {
    // 1. Les lyssensor (I2C adresse 0x44)
    uint16_t raw_light = OPT3001_Read_Light();
    system_state.light_lux = convert_to_lux(raw_light);
    
    delay_us(100);  // Kort pause mellom I2C transaksjoner
    
    // 2. Les temperatur (I2C adresse 0x40)
    uint16_t raw_temp = TMP006_Read_Temperature();
    system_state.temperature_c = convert_to_celsius(raw_temp);
    
    // 3. Marker at LCD trenger oppdatering
    system_state.lcd_update_needed = true;
}

// ============================================
// LCD UPDATE (SPI)
// ============================================
void Update_LCD_Display(void) {
    if (!system_state.lcd_update_needed) {
        return;  // Ingenting å gjøre
    }
    
    // Tegn på LCD via SPI
    LCD_Clear();
    
    // Vis lys-nivå
    LCD_DrawString(10, 20, "Lys:");
    LCD_DrawNumber(70, 20, system_state.light_lux);
    LCD_DrawString(120, 20, "lux");
    
    // Vis temperatur
    LCD_DrawString(10, 40, "Temp:");
    LCD_DrawNumber(70, 40, system_state.temperature_c);
    LCD_DrawString(120, 40, "C");
    
    system_state.lcd_update_needed = false;
}

// ============================================
// MAIN LOOP
// ============================================
int main(void) {
    SYSCFG_DL_init();
    
    while(1) {
        // Les sensorer hver 100 ms
        static uint32_t last_sensor_read = 0;
        if (get_time_ms() - last_sensor_read > 100) {
            Read_All_Sensors();  // I2C
            last_sensor_read = get_time_ms();
        }
        
        // Oppdater LCD når nødvendig
        Update_LCD_Display();  // SPI
        
        // Sov til neste event
        __WFI();
    }
}
```

### **6.2 Timing Analyse**

```
Main Loop Cycle (100 ms):

0 ms:    START
├─ Read OPT3001 (I2C):     ~2 ms
├─ Delay:                  0.1 ms
├─ Read TMP006 (I2C):      ~2 ms
├─ Update LCD (SPI):       ~30 ms
└─ Sleep (__WFI):          ~66 ms

Total CPU active time: ~34 ms (34%)
Total CPU idle time:   ~66 ms (66%)
```

**CPU er ledig 66% av tiden!** Perfekt for power saving.

---

## 📦 7. KØSYSTEM OG DATABUFFER {#queues}

### **7.1 Er Det En Kø?**

**NEI, ikke i hardware!** Men du kan lage én i software:

```c
// Software kø for SPI commands
#define QUEUE_SIZE 32

typedef struct {
    uint8_t data[QUEUE_SIZE];
    uint8_t head;  // Hvor skal vi legge til neste?
    uint8_t tail;  // Hvor skal vi hente neste?
    uint8_t count; // Hvor mange i køen?
} SPI_Queue_t;

SPI_Queue_t spi_queue = {0};

// Legg til i kø
void Queue_Add(uint8_t byte) {
    if (spi_queue.count < QUEUE_SIZE) {
        spi_queue.data[spi_queue.head] = byte;
        spi_queue.head = (spi_queue.head + 1) % QUEUE_SIZE;
        spi_queue.count++;
    }
}

// Ta fra kø
uint8_t Queue_Remove(void) {
    if (spi_queue.count > 0) {
        uint8_t byte = spi_queue.data[spi_queue.tail];
        spi_queue.tail = (spi_queue.tail + 1) % QUEUE_SIZE;
        spi_queue.count--;
        return byte;
    }
    return 0;
}

// Send alle i køen
void Process_SPI_Queue(void) {
    while (spi_queue.count > 0) {
        uint8_t byte = Queue_Remove();
        DL_SPI_transmitData8(SPI1_INST, byte);
        while (DL_SPI_isBusy(SPI1_INST));  // Vent
    }
}
```

**Bruk:**

```c
// Legg kommandoer i kø (raskt!)
Queue_Add(0x2A);  // Column address set
Queue_Add(0x00);
Queue_Add(0x00);
Queue_Add(0x00);
Queue_Add(0x7F);

// Prosesser køen senere
Process_SPI_Queue();
```

### **7.2 Hardware FIFO Buffers**

**MSPM0 har innebygde FIFO buffers!**

```
SPI TX FIFO:
┌──────────┐
│  Byte 7  │ ← Nyeste
│  Byte 6  │
│  Byte 5  │
│  Byte 4  │
│  Byte 3  │
│  Byte 2  │
│  Byte 1  │
│  Byte 0  │ ← Eldste (sendes først)
└──────────┘

I2C TX FIFO:
┌──────────┐
│  Byte 3  │
│  Byte 2  │
│  Byte 1  │
│  Byte 0  │
└──────────┘
```

**Fordel:** Du kan fylle FIFO raskt, hardware sender automatisk!

```c
// Fyll SPI FIFO (8 bytes)
DL_SPI_transmitData8(SPI1_INST, 0x01);
DL_SPI_transmitData8(SPI1_INST, 0x02);
DL_SPI_transmitData8(SPI1_INST, 0x03);
// ... opp til 8 bytes

// Hardware sender automatisk i rekkefølge!
// Ingen ekstra kode nødvendig
```

---

## 🔧 8. FEILSØKING {#troubleshooting}

### **8.1 SPI Problemer**

**Problem: LCD viser bare hvitt/svart eller garbage**

```
Sjekk:
├─ [ ] CS pin toggle korrekt? (LOW når send, HIGH når idle)
├─ [ ] SPI clock ikke for rask? (max 15 MHz for ST7735)
├─ [ ] DC pin korrekt? (HIGH for data, LOW for command)
├─ [ ] Riktige pins? (PB15=MOSI, PB16=SCLK)
└─ [ ] Reset pin pulsed ved oppstart?
```

**Debug kode:**

```c
// Test SPI kommunikasjon
void Test_SPI(void) {
    // Send en enkel kommando
    DL_GPIO_clearPins(GPIOA, LCD_CS_PIN);  // CS=LOW
    DL_GPIO_clearPins(GPIOB, LCD_DC_PIN);  // DC=LOW (command)
    
    DL_SPI_transmitData8(SPI1_INST, 0x01);  // Software reset
    while (DL_SPI_isBusy(SPI1_INST));
    
    DL_GPIO_setPins(GPIOA, LCD_CS_PIN);    // CS=HIGH
    delay_ms(120);  // Wait for reset
    
    // Hvis LCD reagerer → SPI virker!
}
```

### **8.2 I2C Problemer**

**Problem: Sensor svarer ikke (NACK error)**

```
Sjekk:
├─ [ ] J19/J20 jumpers installed? (I2C pull-ups!)
├─ [ ] Riktig adresse? (OPT3001=0x44, TMP006=0x40)
├─ [ ] SDA/SCL riktige pins? (PA0=SDA, PA1=SCL)
├─ [ ] Sensor får strøm? (3.3V og GND)
└─ [ ] I2C speed ikke for rask? (bruk 100 kHz for test)
```

**Debug kode:**

```c
// Scan I2C bus for enheter
void Scan_I2C_Bus(void) {
    printf("Scanning I2C bus...
");
    
    for (uint8_t addr = 0x01; addr < 0x7F; addr++) {
        // Prøv å starte transfer
        DL_I2C_startControllerTransfer(
            I2C0_INST,
            addr,
            DL_I2C_CONTROLLER_DIRECTION_TX,
            0
        );
        
        delay_ms(10);
        
        // Sjekk om ACK mottatt
        if (!(DL_I2C_getControllerStatus(I2C0_INST) & DL_I2C_CONTROLLER_STATUS_ERROR)) {
            printf("Found device at 0x%02X
", addr);
        }
        
        DL_I2C_stopControllerTransfer(I2C0_INST);
    }
    
    printf("Scan complete.
");
}
```

### **8.3 Timing Problemer**

**Problem: System "henger" eller er treg**

```c
// Mål hvor lang tid ting tar
void Measure_Timing(void) {
    uint32_t start, end;
    
    // Mål I2C lesing
    start = get_time_us();
    OPT3001_Read_Light();
    end = get_time_us();
    printf("I2C read: %lu us
", end - start);
    
    // Mål LCD oppdatering
    start = get_time_us();
    LCD_Clear();
    end = get_time_us();
    printf("LCD clear: %lu us
", end - start);
}
```

---

## ✅ OPPSUMMERING

### **Nøkkelpunkter:**

1. **SPI = Rask, en-til-en kommunikasjon**
   - Master velger slave med CS pin
   - Perfekt for LCD (mye data, raskt)
   - Ingen kø i hardware - koden bestemmer rekkefølge

2. **I2C = Langsom, delt buss**
   - Alle deler SDA/SCL
   - Enheter valgt med adresse (0x44, 0x40, etc)
   - Krever pull-up resistorer (J19/J20)

3. **Ingen automatisk kø**
   - Koden din bestemmer hvem som snakker når
   - Du kan lage software-kø hvis ønskelig
   - Hardware FIFO buffers hjelper med burst transfers

4. **Interrupts er valgfritt**
   - Polling: Enklere, blokkerer CPU
   - Interrupt: Mer komplekst, CPU kan gjøre annet
   - DMA: Best for store overføringer

5. **Prioriteter**
   - Timer (audio) = høyest (Priority 1)
   - SPI/I2C = medium (Priority 2-3)
   - Lower priority kan avbrytes av higher priority

### **Beste Praksis:**

```c
// Oppdater system i rekkefølge:
void System_Update(void) {
    // 1. Les sensorer (I2C) - sjeldent
    if (time_for_sensor_read()) {
        Read_All_Sensors();
    }
    
    // 2. Oppdater LCD (SPI) - oftere
    if (lcd_needs_update()) {
        Update_LCD_Display();
    }
    
    // 3. Audio i ISR (Timer) - konstant 8 kHz
    // Dette skjer automatisk i bakgrunnen!
}
```

**Lykke til med SPI og I2C!** 🔌📡
