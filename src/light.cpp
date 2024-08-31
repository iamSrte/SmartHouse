#include <Arduino.h>
#include <SoftwareSerial.h>


const uint8_t masterNode = 0;
const uint8_t gardenerNode = 1;
const uint8_t lightControlNode = 2;
const uint8_t airConditionerNode = 3;
const uint8_t deviceID = lightControlNode;

const uint8_t rxPin = 10;
const uint8_t txPin = 11;
const uint8_t cnPin = 12;
const uint8_t motorEnablePin = 4;
const uint8_t motorClk = 5;
const uint8_t motorCClk = 6;
const uint8_t lightSensorPin = A0;
const uint8_t openKeyPin = A1;
const uint8_t closeKeyPin = A2;

bool curtainChanging;
uint8_t curtainCondition;
uint16_t lightSensor;
String buffer;


SoftwareSerial bus(rxPin, txPin);


void response(String request);


void setup() {
    // Sensor Configuration
    pinMode(lightSensorPin, INPUT);
    pinMode(closeKeyPin, INPUT);
    pinMode(openKeyPin, INPUT);

    // AC Control Configuration
    pinMode(motorEnablePin, OUTPUT);
    pinMode(motorCClk, OUTPUT);
    pinMode(motorClk, OUTPUT);

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
    if (curtainChanging) {
        if (digitalRead(closeKeyPin)) {
            curtainCondition = 0;
            curtainChanging = false;
            digitalWrite(motorEnablePin, LOW);
            digitalWrite(motorClk, LOW);
            digitalWrite(motorCClk, LOW);
        }
        if (digitalRead(openKeyPin)) {
            curtainCondition = 1;
            curtainChanging = false;
            digitalWrite(motorEnablePin, LOW);
            digitalWrite(motorClk, LOW);
            digitalWrite(motorCClk, LOW);
        }
    }
}


void send_response(const char response[]) {
    digitalWrite(cnPin, HIGH);
    delay(5);
    bus.write(response);
    digitalWrite(cnPin, LOW);
}


void update_sensor_data(uint16_t *light) {
    int32_t average = 0;
    for (int i = 0; i <= 20; i++) {
        average += analogRead(lightSensorPin);
    }
    average = average / 20;
    *light = map(average, 0, 1023, 0, 100);
}


void set_curtain(uint8_t condition) {
    curtainChanging = true;
    if (condition == 0) {
        digitalWrite(motorEnablePin, HIGH);
        digitalWrite(motorClk, HIGH);
        digitalWrite(motorCClk, LOW);
        curtainCondition = 2;
    } else if (condition == 1) {
        digitalWrite(motorEnablePin, HIGH);
        digitalWrite(motorClk, LOW);
        digitalWrite(motorCClk, HIGH);
        curtainCondition = 2;
    } else {
        curtainChanging = false;
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
            update_sensor_data(&lightSensor);
            strcat(response, "RSP");
            strcat(response, " ");
            strcat(response, itoa(lightSensor, temp, 10));
            strcat(response, " ");
            strcat(response, itoa(curtainCondition, temp, 10));
        } else if (mode == "SET") {
            uint8_t condition;
            idx_start = idx_end + 1;
            idx_end = buffer.indexOf(' ', idx_start);
            condition = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
            set_curtain(condition);
            strcat(response, "OK");
        }
        strcat(response, " \n");
        send_response(response);
    }
}
