#include <Arduino.h>
#include <SoftwareSerial.h>


const uint8_t masterNode = 0;
const uint8_t gardenerNode = 1;
const uint8_t lightControlNode = 2;
const uint8_t airConditionerNode = 3;
const uint8_t deviceID = gardenerNode;

const uint8_t rxPin = 10;
const uint8_t txPin = 11;
const uint8_t cnPin = 12;
const uint8_t motorClk = 4;
const uint8_t motorCClk = 5;
const uint8_t motorEnablePin = 6;
const uint8_t humiditySensorPin = A1;


uint8_t pompCondition;
uint16_t humiditySensor;
String buffer;


SoftwareSerial bus(rxPin, txPin);


void response(String request);


void setup() {
    // Sensor Configuration
    pinMode(humiditySensorPin, INPUT);

    // Pomp Control Configuration
    pinMode(motorEnablePin, OUTPUT);
    pinMode(motorCClk, OUTPUT);
    pinMode(motorClk, OUTPUT);
    digitalWrite(motorClk, HIGH);
    digitalWrite(motorCClk, LOW);

    // Serial Communication Configuration
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    pinMode(cnPin, OUTPUT);
    digitalWrite(cnPin, LOW);
    bus.begin(9600);
    Serial.begin(9600);
}


void loop() {
    if (bus.available()) {
        buffer = bus.readStringUntil('\n');
        response(buffer);
    }
}


void send_response(const char response[]) {
    digitalWrite(cnPin, HIGH);
    delay(5);
    bus.write(response);
    digitalWrite(cnPin, LOW);
}


void update_sensor_data(uint16_t *humidity) {
    *humidity = map(analogRead(humiditySensorPin), 0, 1023, 100, 0);
}


void set_pomp(uint8_t condition) {
    if (condition == 1) {
        digitalWrite(motorEnablePin, HIGH);
        pompCondition = 1;
    } else {
        digitalWrite(motorEnablePin, LOW);
        pompCondition = 0;
    }
}


void response(String request) {
    String mode;
    uint8_t sender, receiver;
    uint8_t idx_start, idx_end;

    idx_start = 0;
    idx_end = request.indexOf(' ', idx_start);
    sender = strtoul(request.substring(idx_start, idx_end).c_str(), nullptr, 10);
    idx_start = idx_end + 1;
    idx_end = request.indexOf(' ', idx_start);
    receiver = strtoul(request.substring(idx_start, idx_end).c_str(), nullptr, 10);
    idx_start = idx_end + 1;
    idx_end = request.indexOf(' ', idx_start);
    mode = request.substring(idx_start, idx_end);
    if (receiver == deviceID) {
        char temp[3];
        char response[21] = "";
        strcat(response, itoa(deviceID, temp, 10));
        strcat(response, " ");
        strcat(response, itoa(sender, temp, 10));
        strcat(response, " ");
        if (mode == "REQ") {
            update_sensor_data(&humiditySensor);
            strcat(response, "RSP");
            strcat(response, " ");
            strcat(response, itoa(humiditySensor, temp, 10));
            strcat(response, " ");
            strcat(response, itoa(pompCondition, temp, 10));
        } else if (mode == "SET") {
            uint8_t condition;
            idx_start = idx_end + 1;
            idx_end = buffer.indexOf(' ', idx_start);
            condition = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
            set_pomp(condition);
            strcat(response, "OK");
        }
        strcat(response, " \n");
        send_response(response);
    }
}
