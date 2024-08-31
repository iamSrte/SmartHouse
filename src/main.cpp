#include <Arduino.h>
#include <Wire.h>
#include "Keypad.h"
#include <SoftwareSerial.h>
#include "LiquidCrystal_I2C.h"


const uint8_t masterNode = 0;
const uint8_t gardenerNode = 1;
const uint8_t lightControlNode = 2;
const uint8_t airConditionerNode = 3;
const uint8_t deviceID = masterNode;

const uint8_t rxPin = 10;
const uint8_t txPin = 11;
const uint8_t cnPin = 12;
const uint8_t backKeyPin = 13;

int32_t tempSensor;
uint8_t humiditySensor;
uint8_t motorCondition;
uint8_t pompCondition;

uint16_t lightSensor;
uint8_t curtainsCondition;

uint16_t gardnerHumiditySensor;
uint8_t gardnerPompCondition;


uint8_t fetchError;
bool clearFirst = true;
bool clearError = true;
bool justUpdate = false;
bool inMainMenu = false;


const int ROW_NUM = 4;
const int COLUMN_NUM = 4;
char keys[4][4] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
};

byte pin_rows[ROW_NUM] = {9, 8, 7, 6};
byte pin_column[COLUMN_NUM] = {5, 4, 3, 2};
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial bus(rxPin, txPin);


void showMainMenu();

void showACMenu(bool clear, bool update);

void showLightMenu(bool clear, bool update);

void showGardenerMenu(bool clear, bool update);

void showConnectionLost(const char prompt[], bool clear);

bool setACData(uint8_t pomp, uint8_t motor);

bool fetchACData(int32_t *temperature, uint8_t *humidity, uint8_t *pomp, uint8_t *motor);

bool setLightData(uint8_t curtain);

bool fetchLightData(uint16_t *light, uint8_t *curtain);

bool setGardenerData(uint8_t pomp);

bool fetchGardenerData(uint16_t *gardnerHumidity, uint8_t *gardnerPomp);


void setup() {
    // LCD Configuration
    lcd.init();
    lcd.backlight();

    // Serial Communication Configuration
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    pinMode(cnPin, OUTPUT);
    pinMode(backKeyPin, INPUT);
    digitalWrite(cnPin, HIGH);
    bus.begin(9600);
    Serial.begin(9600);
}


void loop() {
    if (!inMainMenu) {
        showMainMenu();
        fetchError = 0;
        clearFirst = true;
        clearError = true;
        inMainMenu = true;
        justUpdate = false;
    }
    switch (keypad.getKey()) {
        case '1':
            inMainMenu = false;
            while (digitalRead(backKeyPin) == 0) {
                if (fetchACData(&tempSensor, &humiditySensor, &pompCondition, &motorCondition)) {
                    fetchError = 0;
                    showACMenu(clearFirst, justUpdate);
                    clearFirst = false;
                    clearError = true;
                    justUpdate = true;
                } else {
                    fetchError += 1;
                    if (fetchError == 4) {
                        showConnectionLost("AC Controller:", clearError);
                        clearFirst = true;
                        clearError = false;
                        justUpdate = false;
                    }
                }
                switch (keypad.getKey()) {
                    case '1':
                        setACData(pompCondition, motorCondition + 1);
                        break;

                    case '2':
                        setACData(pompCondition + 1, motorCondition);
                        break;
                }
            }
            break;
        case '2':
            inMainMenu = false;
            while (digitalRead(backKeyPin) == 0) {
                if (fetchLightData(&lightSensor, &curtainsCondition)) {
                    fetchError = 0;
                    showLightMenu(clearFirst, justUpdate);
                    clearFirst = false;
                    clearError = true;
                    justUpdate = true;
                } else {
                    fetchError += 1;
                    if (fetchError == 4) {
                        showConnectionLost("Light Controller", clearFirst);
                        clearFirst = true;
                        clearError = false;
                        justUpdate = false;
                    }
                }
                switch (keypad.getKey()) {
                    case '1':
                        if (curtainsCondition == 0) setLightData(1);
                        else if (curtainsCondition == 1) setLightData(0);
                        break;
                }
            }
            break;
        case '3':
            inMainMenu = false;
            while (digitalRead(backKeyPin) == 0) {
                if (fetchGardenerData(&gardnerHumiditySensor, &gardnerPompCondition)) {
                    fetchError = 0;
                    showGardenerMenu(clearFirst, justUpdate);
                    clearFirst = false;
                    clearError = true;
                    justUpdate = true;
                } else {
                    fetchError += 1;
                    if (fetchError == 4) {
                        showConnectionLost("GRDNR Controller", clearFirst);
                        clearFirst = true;
                        clearError = false;
                        justUpdate = false;
                    }
                }
                switch (keypad.getKey()) {
                    case '1':
                        setGardenerData(gardnerPompCondition + 1);
                        break;
                }
            }
            break;
    }
}


bool sendRequest(const char request[], String *result) {
    digitalWrite(cnPin, HIGH);
    delay(5);
    bus.write(request);
    digitalWrite(cnPin, LOW);
    bus.listen();
    delay(20);
    if (bus.available()) {
        *result = bus.readStringUntil('\n');
        return true;
    }
    return false;
}


bool setACData(uint8_t pomp, uint8_t motor) {
    char temp[2];
    char request[20] = "";
    String buffer, mode;
    uint8_t sender, receiver;
    uint8_t idx_start, idx_end;
    strcat(request, itoa(deviceID, temp, 10));
    strcat(request, " ");
    strcat(request, itoa(airConditionerNode, temp, 10));
    strcat(request, " ");
    strcat(request, "SET");
    strcat(request, " ");
    strcat(request, itoa(pomp, temp, 10));
    strcat(request, " ");
    strcat(request, itoa(motor, temp, 10));
    strcat(request, " \n");
    if (sendRequest(request, &buffer)) {
        idx_start = 0;
        idx_end = buffer.indexOf(' ', idx_start);
        sender = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
        idx_start = idx_end + 1;
        idx_end = buffer.indexOf(' ', idx_start);
        receiver = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
        idx_start = idx_end + 1;
        idx_end = buffer.indexOf(' ', idx_start);
        mode = buffer.substring(idx_start, idx_end);
        if ((sender == airConditionerNode) && (receiver == deviceID) && (mode == "OK")) {
            return true;
        }
    }
    return false;
}


bool fetchACData(int32_t *temperature, uint8_t *humidity, uint8_t *pomp, uint8_t *motor) {
    char temp[2];
    char request[20] = "";
    strcat(request, itoa(deviceID, temp, 10));
    strcat(request, " ");
    strcat(request, itoa(airConditionerNode, temp, 10));
    strcat(request, " ");
    strcat(request, "REQ");
    strcat(request, "\n");

    String buffer, mode;
    uint8_t sender, receiver;
    uint8_t idx_start, idx_end;
    if (sendRequest(request, &buffer)) {
        idx_start = 0;
        idx_end = buffer.indexOf(' ', idx_start);
        sender = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
        idx_start = idx_end + 1;
        idx_end = buffer.indexOf(' ', idx_start);
        receiver = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
        idx_start = idx_end + 1;
        idx_end = buffer.indexOf(' ', idx_start);
        mode = buffer.substring(idx_start, idx_end);
        if ((sender == airConditionerNode) && (receiver == deviceID) && (mode == "RSP")) {
            idx_start = idx_end + 1;
            idx_end = buffer.indexOf(' ', idx_start);
            *temperature = strtol(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
            idx_start = idx_end + 1;
            idx_end = buffer.indexOf(' ', idx_start);
            *humidity = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
            idx_start = idx_end + 1;
            idx_end = buffer.indexOf(' ', idx_start);
            *pomp = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
            idx_start = idx_end + 1;
            idx_end = buffer.indexOf(' ', idx_start);
            *motor = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
            return true;
        }
    }
    return false;
}


bool setLightData(uint8_t curtain) {
    char temp[2];
    char request[20] = "";
    String buffer, mode;
    uint8_t sender, receiver;
    uint8_t idx_start, idx_end;
    strcat(request, itoa(deviceID, temp, 10));
    strcat(request, " ");
    strcat(request, itoa(lightControlNode, temp, 10));
    strcat(request, " ");
    strcat(request, "SET");
    strcat(request, " ");
    strcat(request, itoa(curtain, temp, 10));
    strcat(request, " \n");
    if (sendRequest(request, &buffer)) {
        idx_start = 0;
        idx_end = buffer.indexOf(' ', idx_start);
        sender = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
        idx_start = idx_end + 1;
        idx_end = buffer.indexOf(' ', idx_start);
        receiver = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
        idx_start = idx_end + 1;
        idx_end = buffer.indexOf(' ', idx_start);
        mode = buffer.substring(idx_start, idx_end);
        if ((sender == lightControlNode) && (receiver == deviceID) && (mode == "OK")) {
            return true;
        }
    }
    return false;
}


bool fetchLightData(uint16_t *light, uint8_t *curtain) {
    char temp[2];
    char request[20] = "";
    strcat(request, itoa(deviceID, temp, 10));
    strcat(request, " ");
    strcat(request, itoa(lightControlNode, temp, 10));
    strcat(request, " ");
    strcat(request, "REQ");
    strcat(request, "\n");

    String buffer, mode;
    uint8_t sender, receiver;
    uint8_t idx_start, idx_end;
    if (sendRequest(request, &buffer)) {
        idx_start = 0;
        idx_end = buffer.indexOf(' ', idx_start);
        sender = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
        idx_start = idx_end + 1;
        idx_end = buffer.indexOf(' ', idx_start);
        receiver = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
        idx_start = idx_end + 1;
        idx_end = buffer.indexOf(' ', idx_start);
        mode = buffer.substring(idx_start, idx_end);
        if ((sender == lightControlNode) && (receiver == deviceID) && (mode == "RSP")) {
            idx_start = idx_end + 1;
            idx_end = buffer.indexOf(' ', idx_start);
            *light = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
            idx_start = idx_end + 1;
            idx_end = buffer.indexOf(' ', idx_start);
            *curtain = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
            return true;
        }
    }
    return false;
}


bool setGardenerData(uint8_t pomp) {
    char temp[2];
    char request[20] = "";
    String buffer, mode;
    uint8_t sender, receiver;
    uint8_t idx_start, idx_end;
    strcat(request, itoa(deviceID, temp, 10));
    strcat(request, " ");
    strcat(request, itoa(gardenerNode, temp, 10));
    strcat(request, " ");
    strcat(request, "SET");
    strcat(request, " ");
    strcat(request, itoa(pomp, temp, 10));
    strcat(request, " \n");
    if (sendRequest(request, &buffer)) {
        idx_start = 0;
        idx_end = buffer.indexOf(' ', idx_start);
        sender = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
        idx_start = idx_end + 1;
        idx_end = buffer.indexOf(' ', idx_start);
        receiver = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
        idx_start = idx_end + 1;
        idx_end = buffer.indexOf(' ', idx_start);
        mode = buffer.substring(idx_start, idx_end);
        if ((sender == lightControlNode) && (receiver == deviceID) && (mode == "OK")) {
            return true;
        }
    }
    return false;
}


bool fetchGardenerData(uint16_t *gardnerHumidity, uint8_t *gardnerPomp) {
    char temp[2];
    char request[20] = "";
    strcat(request, itoa(deviceID, temp, 10));
    strcat(request, " ");
    strcat(request, itoa(gardenerNode, temp, 10));
    strcat(request, " ");
    strcat(request, "REQ");
    strcat(request, "\n");

    String buffer, mode;
    uint8_t sender, receiver;
    uint8_t idx_start, idx_end;
    if (sendRequest(request, &buffer)) {
        idx_start = 0;
        idx_end = buffer.indexOf(' ', idx_start);
        sender = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
        idx_start = idx_end + 1;
        idx_end = buffer.indexOf(' ', idx_start);
        receiver = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
        idx_start = idx_end + 1;
        idx_end = buffer.indexOf(' ', idx_start);
        mode = buffer.substring(idx_start, idx_end);
        if ((sender == gardenerNode) && (receiver == deviceID) && (mode == "RSP")) {
            idx_start = idx_end + 1;
            idx_end = buffer.indexOf(' ', idx_start);
            *gardnerHumidity = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
            idx_start = idx_end + 1;
            idx_end = buffer.indexOf(' ', idx_start);
            *gardnerPomp = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
            return true;
        }
    }
    return false;
}


void showMainMenu() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("1.AC");
    lcd.setCursor(0, 1);
    lcd.print("2.Light");
    lcd.setCursor(8, 0);
    lcd.print("3.GRDNR");
    lcd.setCursor(8, 1);
    lcd.print("4._____");
}


void showACMenu(bool clear, bool update) {
    uint8_t tempPos;
    uint8_t humidityPos;
    if (clear) {
        lcd.clear();
    }
    if (!update) {
        lcd.setCursor(0, 0);
        lcd.print("1.MTR");
        lcd.setCursor(0, 1);
        lcd.print("2.PMP");
        lcd.setCursor(11, 0);
        lcd.print("T:");
        lcd.setCursor(11, 1);
        lcd.print("H:");
    }

    lcd.setCursor(6, 0);
    if (motorCondition == 0) {
        lcd.print("OFF");
    } else if (motorCondition == 1) {
        lcd.print("SLW");
    } else {
        lcd.print("FST");
    }

    lcd.setCursor(6, 1);
    if (pompCondition == 0) {
        lcd.print("OFF");
    } else {
        lcd.print("ON ");
    }

    lcd.setCursor(13, 0);
    lcd.print(tempSensor);
    if (tempSensor < 10) {
        tempPos = 14;
    } else if (tempSensor < 100) {
        tempPos = 15;
    } else {
        tempPos = 16;
    }
    lcd.setCursor(tempPos, 0);
    lcd.print((char) 223);
    lcd.print("  ");

    lcd.setCursor(13, 1);
    lcd.print(humiditySensor);
    if (humiditySensor < 10) {
        humidityPos = 14;
    } else if (humiditySensor < 100) {
        humidityPos = 15;
    } else {
        humidityPos = 16;
    }
    lcd.setCursor(humidityPos, 1);
    lcd.print("%  ");
}


void showLightMenu(bool clear, bool update) {
    uint8_t lightPos;
    if (clear) {
        lcd.clear();
    }
    if (!update) {
        lcd.setCursor(0, 0);
        lcd.print("Light LVL:");
        lcd.setCursor(0, 1);
        lcd.print("1.Curtain:");
    }

    lcd.setCursor(11, 0);
    lcd.print(lightSensor);
    if (lightSensor < 10) {
        lightPos = 12;
    } else if (lightSensor < 100) {
        lightPos = 13;
    } else {
        lightPos = 14;
    }
    lcd.setCursor(lightPos, 0);
    lcd.print("%  ");

    lcd.setCursor(11, 1);
    if (curtainsCondition == 0) {
        lcd.print("CLOSE");
    } else if (curtainsCondition == 1) {
        lcd.print("OPEN ");
    } else {
        lcd.print("PRGRS");
    }
}


void showGardenerMenu(bool clear, bool update) {
    uint8_t humidityPos;
    if (clear) {
        lcd.clear();
    }
    if (!update) {
        lcd.setCursor(0, 0);
        lcd.print("Humidity:");
        lcd.setCursor(0, 1);
        lcd.print("1.Pomp:");
    }
    lcd.setCursor(10, 0);
    lcd.print(gardnerHumiditySensor);
    if (gardnerHumiditySensor < 10) {
        humidityPos = 11;
    } else if (gardnerHumiditySensor < 100) {
        humidityPos = 12;
    } else {
        humidityPos = 13;
    }
    lcd.setCursor(humidityPos, 0);
    lcd.print("%  ");

    lcd.setCursor(8, 1);
    if (gardnerPompCondition == 0) {
        lcd.print("OFF");
    } else {
        lcd.print("ON ");
    }
}


void showConnectionLost(const char prompt[], bool clear) {
    if (clear) {
        lcd.clear();
    }
    lcd.setCursor(0, 0);
    lcd.print(prompt);
    lcd.setCursor(0, 1);
    lcd.print("Connection Lost!");
}
