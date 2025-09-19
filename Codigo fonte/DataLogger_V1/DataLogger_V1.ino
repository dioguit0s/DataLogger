#include <LiquidCrystal_I2C.h>  // Biblioteca para LCD I2C
#include <RTClib.h>             // Biblioteca para Relógio em Tempo Real
#include <Wire.h>               // Biblioteca para comunicação I2C
#include <EEPROM.h>             // Biblioteca para memoria EEPROM
#include "DHT.h"                // Biblioteca para o sensor DHT11
#include "CMBMenu.hpp" // ** menu **

// --- Global Hardware and Settings ---
#define UTC_OFFSET -3   // Correct offset for Brazil (BRT)
#define DHTPIN 2
#define DHTTYPE DHT11
#define BUZZER_PIN 8   
DHT dht(DHTPIN, DHTTYPE);

// Digital Pins
int btnEntra = 3;
int btnVolta = 4;
int ledOk = 5;
int ledFailure = 6;
// Analog Pins
const int joystickXPin = A0;
const int lightSensorPin = A2;

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS1307 RTC;

// --- EEPROM Configuration ---
const int maxRecords = 120;
const int recordSize = 10; // 4 (time) + 2 (temp) + 2 (hum) + 2 (lum) = 10 bytes
int startAddress = 0;
int endAddress = maxRecords * recordSize;
int currentAddress = 0;
int lastLoggedMinute = -1;

// Monitoring Triggers
float trigger_t_min = 15.0, trigger_t_max = 25.0;
float trigger_u_min = 30.0, trigger_u_max = 80.0;
float trigger_l_min = 0, trigger_l_max = 30.0; // Adjustable

// --- CMBMenu Setup ---

// ** menu **
// Define menu text
const char g_menuDataHora[] PROGMEM = {"1. Data e Hora"};
const char g_menuSensores[] PROGMEM = {"2. Sensores"};
const char g_menuSensorHum[] PROGMEM = {"2.1 Ver sensores"};
const char g_menuSensorLum[] PROGMEM = {"2.2 Ajuste Luz"};
const char g_menuViewLog[] PROGMEM = {"3. Ver Log"};

// ** menu **
// Define function IDs
enum MenuFID {
    MenuDummy, // A dummy ID for parent menus that do nothing
    MenuDataHora,
    MenuSensores,
    MenuSensorHum,
    MenuSensorLum,
    MenuViewLog
};

// Define key types
enum KeyType {
    keyNone,
    keyLeft,
    keyRight,
    keyEnter,
    keyExit
};

// ** menu **
// Create global CMBMenu instance
CMBMenu<100> g_Menu;

// --- Forward Declarations ---
void printMenuEntry(const char* f_Info);
KeyType getKey();
void showDataHora();
void showHumETemp();
void adjustLumSettings();
void showLog();
void getNextAddress();
void checkAndLogSensors();
void handleAlarm(bool isActive); 

// --- CARACTERES CUSTOMIZADOS (APENAS FRAMES A, B e C) ---

// --- FRAME A (Imagem 1) ---
byte frameA_0_0[] = { B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000 };
byte frameA_0_1[] = { B00011, B00100, B01000, B01110, B10011, B10011, B01100, B00110 };
byte frameA_0_2[] = { B11110, B00001, B00000, B00101, B00100, B00010, B00111, B00000 };
byte frameA_0_3[] = { B01110, B10001, B01001, B11010, B00010, B00010, B11100, B00100 };
byte frameA_1_0[] = { B00000, B00000, B00001, B00001, B00000, B00001, B00001, B00000 };
byte frameA_1_1[] = { B01000, B11100, B00010, B00010, B11111, B00010, B00100, B11011 };
byte frameA_1_2[] = { B00001, B00000, B01000, B10000, B00000, B00110, B01100, B10000 };
byte frameA_1_3[] = { B01000, B11110, B01001, B10001, B10010, B10010, B01100, B00000 };

// --- FRAME B (Imagem 2) ---
byte frameB_0_1[] = { B00000, B00001, B00010, B00011, B00100, B00100, B00011, B00011 };
byte frameB_0_2[] = { B11111, B00000, B00000, B10010, B11010, B11001, B00011, B10000 };
byte frameB_0_3[] = { B00000, B11100, B00010, B11100, B00010, B00010, B11100, B01000 };
byte frameB_1_1[] = { B01100, B10000, B10000, B01001, B00110, B01001, B01000, B01111 };
byte frameB_1_2[] = { B00000, B00000, B01000, B10000, B00000, B01101, B10010, B10011 };
byte frameB_1_3[] = { B11100, B01010, B01001, B01101, B10010, B00010, B00100, B11100 };

// --- FRAME C (Imagem 3) ---
byte frameC_0_2[] = { B11110, B00001, B00000, B01011, B01000, B00100, B01111, B00001 };
byte frameC_0_3[] = { B00000, B10000, B01000, B10000, B01000, B01000, B10000, B00000 };
byte frameC_1_1[] = { B01001, B01000, B10000, B10011, B01100, B01000, B01111, B00000 };
byte frameC_1_2[] = { B00010, B10001, B10001, B00001, B01000, B11000, B11111, B00000 };
byte frameC_1_3[] = { B10000, B01000, B01000, B10000, B10000, B10000, B10000, B00000 };

// ********************************************
// SETUP
// ********************************************
void setup() {
    pinMode(btnEntra, INPUT_PULLUP);
    pinMode(btnVolta, INPUT_PULLUP);
    pinMode(ledOk, OUTPUT);
    pinMode(ledFailure, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT); 

    lcd.init();
    lcd.backlight();
    dht.begin();
    Serial.begin(9600);
    RTC.begin();
    //RTC.adjust(DateTime(F(__DATE__), F(__TIME__))); // Set time once

    lcd.clear();
    desenhaMario();
    delay(1000);

    // ** menu **
    // Add nodes to menu (layer, string, function ID)
    g_Menu.addNode(0, g_menuDataHora, MenuDataHora);
    g_Menu.addNode(0, g_menuSensores, MenuSensores); // Parent menu, use dummy ID or its own if needed
    g_Menu.addNode(1, g_menuSensorHum, MenuSensorHum);
    g_Menu.addNode(1, g_menuSensorLum, MenuSensorLum);
    g_Menu.addNode(0, g_menuViewLog, MenuViewLog);

    // ** menu **
    // Build menu and get the initial display text
    const char* info;
    g_Menu.buildMenu(info);
    g_Menu.printMenu(); 

    // ** menu **
    // Print the initial menu entry to the LCD
    printMenuEntry(info);
}

// ********************************************
// LOOP
// ********************************************
void loop() {
    // Check sensors once per minute
    checkAndLogSensors();

    // --- Menu Logic  ---
    int fid = 0;
    const char* info;
    bool layerChanged = false;

    KeyType key = getKey();

    // ** menu **
    // Call menu methods regarding pressed key
    switch(key) {
        case keyExit:  g_Menu.exit(); break;
        case keyEnter: g_Menu.enter(layerChanged); break;
        case keyRight: g_Menu.right(); break;
        case keyLeft:  g_Menu.left(); break;
        default: break;
    }

    // ** menu **
    // Update menu on LCD when a key was pressed and get current function ID "fid"
    if (keyNone != key) {
        fid = g_Menu.getInfo(info);
        printMenuEntry(info);
    }

    // ** menu **
    // Do action regarding function ID "fid"
    if ((0 != fid) && (keyEnter == key) && (!layerChanged)) {
        switch (fid) {
            case MenuDataHora:  showDataHora(); break;
            case MenuSensorHum: showHumETemp(); break;
            case MenuSensorLum: adjustLumSettings(); break;
            case MenuViewLog:   showLog(); break;
            default: break; // Parent menus will fall here and do nothing
        }
    }
}

// Variável para controlar qual frame (0, 1 ou 2) será exibida
int frameIndex = 0;

//animacao metodo
void desenhaMario() {
  // O loop controla a posição da imagem na tela, da esquerda para a direita
  for (int posicao = -4; posicao < 16; posicao++) {

    // 1. SELECIONA E CARREGA o frame ANTES de desenhar este passo do movimento
    if (frameIndex == 0)      { loadFrameA(); } 
    else if (frameIndex == 1) { loadFrameB(); } 
    else                      { loadFrameC(); }

    lcd.clear();

    // 2. DESENHA o frame que foi carregado na posição atual
    byte topRowChars[] = {0, 1, 2, 3};
    byte bottomRowChars[] = {4, 5, 6, 7};

    for (int j = 0; j < 4; j++) {
      int colunaNaTela = posicao + j;
      if (colunaNaTela >= 0 && colunaNaTela < 16) {
        lcd.setCursor(colunaNaTela, 0); lcd.write(topRowChars[j]);
        lcd.setCursor(colunaNaTela, 1); lcd.write(bottomRowChars[j]);
      }
    }
    
    // 3. AVANÇA o índice para que o PRÓXIMO PASSO do movimento use a PRÓXIMA IMAGEM
    frameIndex = (frameIndex + 1) % 3;
    
    // Controla a velocidade geral da animação
    delay(200); 
  }
 }

// --- Funções para carregar cada frame na memória do LCD ---
void loadFrameA() {
  lcd.createChar(0, frameA_0_0); lcd.createChar(1, frameA_0_1); lcd.createChar(2, frameA_0_2); lcd.createChar(3, frameA_0_3);
  lcd.createChar(4, frameA_1_0); lcd.createChar(5, frameA_1_1); lcd.createChar(6, frameA_1_2); lcd.createChar(7, frameA_1_3);
}

void loadFrameB() {
  lcd.createChar(0, frameA_0_0); lcd.createChar(1, frameB_0_1); lcd.createChar(2, frameB_0_2); lcd.createChar(3, frameB_0_3);
  lcd.createChar(4, frameA_1_0); lcd.createChar(5, frameB_1_1); lcd.createChar(6, frameB_1_2); lcd.createChar(7, frameB_1_3);
}

void loadFrameC() {
  lcd.createChar(0, frameA_0_0); lcd.createChar(1, frameA_0_1); lcd.createChar(2, frameC_0_2); lcd.createChar(3, frameC_0_3);
  lcd.createChar(4, frameA_1_0); lcd.createChar(5, frameC_1_1); lcd.createChar(6, frameC_1_2); lcd.createChar(7, frameC_1_3);
}

// ********************************************
// Menu and Action Functions
// ********************************************

/**
 * Prints the current menu item text to the first line of the LCD.
 * This function is called every time a navigation key is pressed.
 */
void printMenuEntry(const char* f_Info) {
    String info_s; // PROGMEM conversion is handled by the library
    MBHelper::stringFromPgm(f_Info, info_s);

    Serial.println("----------------");
    Serial.println(info_s);
    Serial.println("----------------");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(info_s);
    // The second line is now free for action functions to use.
}

/**
 * Reads joystick and buttons to determine which key was pressed.
 */
KeyType getKey() {
    KeyType key = keyNone;
    int xValue = analogRead(joystickXPin);
    
    if (xValue < 200) key = keyLeft;
    else if (xValue > 800) key = keyRight;
    else if (digitalRead(btnEntra) == LOW) key = keyEnter;
    else if (digitalRead(btnVolta) == LOW) key = keyExit;
    
    if (key != keyNone) delay(200); // Simple debounce
    return key;
}

/**
 * Displays the current time on the second line of the LCD.
 */
void showDataHora() {
    DateTime now = RTC.now();
    DateTime adjustedTime = DateTime(now.unixtime() + (UTC_OFFSET * 3600L));
    char timeBuffer[16];
    sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(timeBuffer);
}

/**
 * Displays the current temp/humidity on the second line.
 */
void showHumETemp() {
    int h = dht.readHumidity();
    int t = dht.readTemperature();
    float l = analogRead(lightSensorPin);

    char sensorBuffer[16];
    //sprintf(sensorBuffer, "T: " + String(t) + "C" + "H: " + String(h) + "%");

    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("T:" + String(t) + "C " + "H:" + String(h) + "% " + "L:" + String(l));
}

/**
 * Allows adjustment of the light trigger. This is a special
 * case that needs its own temporary loop.
 */
void adjustLumSettings() {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Ajuste:<- ->Exit");
    
    // This is the one place where a sub-loop is acceptable
    while (digitalRead(btnVolta) == HIGH) { // Exit with back button
        KeyType adjKey = getKey();
        if (adjKey == keyRight) {
            trigger_l_max += 1.0;
            if (trigger_l_max > 1023) trigger_l_max = 1023;
        } else if (adjKey == keyLeft) {
            trigger_l_max -= 1.0;
            if (trigger_l_max < 0) trigger_l_max = 0;
        }
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("2.2 Ajuste Luz  ");
        lcd.setCursor(0, 1);
        lcd.print("Max: ");
        lcd.print(trigger_l_max);
        lcd.print("   ");
    }
    // Redraw the menu after exiting the sub-loop
    const char* info;
    g_Menu.getInfo(info);
    printMenuEntry(info);
}

/**
 * Dumps the EEPROM log to the Serial Monitor.
 */
void showLog() {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Log no Serial...");
    
    Serial.println("\n--- INICIO DO LOG DA EEPROM ---");
    Serial.println("Timestamp\t\tTemp\tHumid\tLumin");
    for (int address = startAddress; address < endAddress; address += recordSize) {
        uint32_t timeStamp;
        EEPROM.get(address, timeStamp); // Read the 4-byte timestamp first

        // Check if the timestamp is valid. 0xFFFFFFFF is the value for erased memory.
        if (timeStamp != 0xFFFFFFFF && timeStamp != 0) {
            
            // --- This is the completed log printing logic ---

            // 1. Declare variables to hold the raw sensor data
            int16_t tempInt, humiInt, lumInt;

            // 2. Read the rest of the data for this record from EEPROM
            EEPROM.get(address + 4, tempInt);
            EEPROM.get(address + 6, humiInt);
            EEPROM.get(address + 8, lumInt);
            
            // 3. Convert the raw data back to human-readable values
            float temperature = tempInt / 100.0;
            float humidity = humiInt / 100.0;
            // Luminosity is already a raw value, no conversion needed

            // 4. Create a DateTime object from the Unix timestamp
            DateTime dt = DateTime(timeStamp);

            // 5. Print the formatted data to the Serial Monitor
            Serial.print(dt.timestamp(DateTime::TIMESTAMP_FULL)); // e.g., "2025-09-13T17:57:00"
            Serial.print("\t");
            Serial.print(temperature);
            Serial.print(" C\t");
            Serial.print(humidity);
            Serial.print(" %\t");
            Serial.println(lumInt); // Use println for the last item to move to the next line
        }
    }
    
    // Print a footer to the Serial Monitor
    Serial.println("--- FIM DO LOG ---");

    // Keep the message on the LCD for a moment so the user can see it
    delay(1000); 
    // The main loop will redraw the menu entry automatically after this.
}

// ********************************************
// Sensor and Logging Helper Functions
// ********************************************

/**
 * --- ADDED ---
 *Activates or deactivates the failure LED and buzzer.
 * True to activate the alarm, false to deactivate.
 */
void handleAlarm(bool isActive) {
    digitalWrite(ledFailure, isActive);
    digitalWrite(ledOk, !isActive);

    if (isActive) {
        tone(BUZZER_PIN, 1000, 250); // Sound the buzzer at 1000 Hz for 250ms
    } else {
        noTone(BUZZER_PIN); // Stop the buzzer
    }
}


/**
 *Reads sensors, checks if values are within triggers, and logs if they are not.
 * Also controls the alarm (LEDs and Buzzer).
 */
void checkAndLogSensors() {
    DateTime now = RTC.now();
    DateTime adjustedTime = DateTime(now.unixtime() + (UTC_OFFSET * 3600L));
    if (adjustedTime.minute() != lastLoggedMinute) {
        lastLoggedMinute = adjustedTime.minute();
        digitalWrite(LED_BUILTIN, HIGH);
        delay(50);
        digitalWrite(LED_BUILTIN, LOW);

        float h = dht.readHumidity();
        float t = dht.readTemperature();
        float l = analogRead(lightSensorPin);

        if (isnan(h) || isnan(t)) {
            handleAlarm(true); // Treat sensor failure as an alarm state
        } else {
            bool outOfBounds = (t < trigger_t_min || t > trigger_t_max ||
                                h < trigger_u_min || h > trigger_u_max ||
                                l < trigger_l_min || l > trigger_l_max);

            handleAlarm(outOfBounds); // Call the new alarm function

            if (outOfBounds) {
                EEPROM.put(currentAddress, now.unixtime());
                EEPROM.put(currentAddress + 4, (int16_t)(t * 100));
                EEPROM.put(currentAddress + 6, (int16_t)(h * 100));
                EEPROM.put(currentAddress + 8, (int16_t)l);
                getNextAddress();
            }
        }
    }
}

void getNextAddress() {
    currentAddress += recordSize;
    if (currentAddress >= endAddress) {
        currentAddress = 0;
    }
}
