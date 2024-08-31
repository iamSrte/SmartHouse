#include <Arduino.h>
#include <SoftwareSerial.h>


const uint8_t masterNode = 0;
const uint8_t gardenerNode = 1;
const uint8_t lightControlNode = 2;
const uint8_t airConditionerNode = 3;
const uint8_t deviceID = airConditionerNode;

const uint8_t rxPin = 10;
const uint8_t txPin = 11;
const uint8_t cnPin = 12;
const uint8_t pompEnablePin = 5;
const uint8_t motorSlowPin = 6;
const uint8_t motorFastPin = 7;
const uint8_t temperatureSensorPin = A1;
const uint8_t humiditySensorPin = A5;

int16_t tempSensor;
uint8_t humiditySensor;
uint8_t motorCondition;
uint8_t pompCondition;
String buffer;


SoftwareSerial bus(rxPin, txPin);


void response(String request);

void set_ac(uint8_t pomp, uint8_t motor);


void update_sensor_data(int16_t *temperature, uint8_t *humidity);

void setup() {
    // Sensor Configuration
    pinMode(temperatureSensorPin, INPUT);
    pinMode(humiditySensorPin, INPUT);

    // AC Control Configuration
    pinMode(pompEnablePin, OUTPUT);
    pinMode(motorSlowPin, OUTPUT);
    pinMode(motorFastPin, OUTPUT);

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


void update_sensor_data(int16_t *temperature, uint8_t *humidity) {
    int32_t average = 0;
    for (int i = 0; i <= 20; i++) {
        average += analogRead(temperatureSensorPin);
    }
    average = average / 20;
    *temperature = average * 0.488;
    *humidity = map(analogRead(humiditySensorPin), 0, 1024, 100, 0);
}


void send_response(const char response[]) {
    digitalWrite(cnPin, HIGH);
    delay(5);
    bus.write(response);
    digitalWrite(cnPin, LOW);
}


void set_ac(uint8_t pomp, uint8_t motor) {
    if (pomp == 1) {
        digitalWrite(pompEnablePin, HIGH);
        pompCondition = 1;
    } else {
        digitalWrite(pompEnablePin, LOW);
        pompCondition = 0;
    }

    if (motor == 1) {
        digitalWrite(motorSlowPin, HIGH);
        digitalWrite(motorFastPin, LOW);
        motorCondition = 1;
    } else if (motor == 2) {
        digitalWrite(motorSlowPin, HIGH);
        digitalWrite(motorFastPin, HIGH);
        motorCondition = 2;
    } else {
        digitalWrite(motorSlowPin, LOW);
        digitalWrite(motorFastPin, LOW);
        motorCondition = 0;
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
            update_sensor_data(&tempSensor, &humiditySensor);
            strcat(response, "RSP");
            strcat(response, " ");
            strcat(response, itoa(tempSensor, temp, 10));
            strcat(response, " ");
            strcat(response, itoa(humiditySensor, temp, 10));
            strcat(response, " ");
            strcat(response, itoa(pompCondition, temp, 10));
            strcat(response, " ");
            strcat(response, itoa(motorCondition, temp, 10));
        } else if (mode == "SET") {
            uint8_t pomp, motor;
            idx_start = idx_end + 1;
            idx_end = buffer.indexOf(' ', idx_start);
            pomp = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
            idx_start = idx_end + 1;
            idx_end = buffer.indexOf(' ', idx_start);
            motor = strtoul(buffer.substring(idx_start, idx_end).c_str(), nullptr, 10);
            set_ac(pomp, motor);
            strcat(response, "OK");
        }
        strcat(response, " \n");
        send_response(response);
    }
}
