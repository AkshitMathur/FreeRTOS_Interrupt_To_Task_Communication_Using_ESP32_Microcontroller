#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "SamAdr";
const char* password = "adrish123";

String apiKey = "61TVJB8Z12UYH2SR";
const char* server = "http://api.thingspeak.com/update";

#define SENSOR_PIN 4

volatile int objectCount = 0;
volatile unsigned long lastInterruptTime = 0;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// ISR
void IRAM_ATTR detectObject() {
    unsigned long now = micros(); // better than millis()

    // debounce: ignore signals within 200ms
    if (now - lastInterruptTime > 200000) {

        portENTER_CRITICAL_ISR(&mux);

        objectCount++;

        portEXIT_CRITICAL_ISR(&mux);

        lastInterruptTime = now;
    }
}

// Upload Task
void uploadTask(void *pvParameters) {
    while (1) {

        int safeCount;

        portENTER_CRITICAL(&mux);

        safeCount = objectCount;

        portEXIT_CRITICAL(&mux);

        if (WiFi.status() == WL_CONNECTED) {

            HTTPClient http;

            String url = String(server) +
                         "?api_key=" + apiKey +
                         "&field1=" + String(safeCount);

            http.begin(url);

            int httpResponseCode = http.GET();

            Serial.print("Uploaded Count: ");
            Serial.print(safeCount);
            Serial.print(" | Response: ");
            Serial.println(httpResponseCode);

            http.end();
        }

        vTaskDelay(15000 / portTICK_PERIOD_MS);
    }
}

// Print Task
void printTask(void *pvParameters) {

    int lastPrinted = -1;

    while (1) {

        int safeCount;

        portENTER_CRITICAL(&mux);

        safeCount = objectCount;

        portEXIT_CRITICAL(&mux);

        // Print only when value changes
        if (safeCount != lastPrinted) {

            Serial.print("Object Count: ");
            Serial.println(safeCount);

            lastPrinted = safeCount;
        }

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

void setup() {

    Serial.begin(115200);

    pinMode(SENSOR_PIN, INPUT);

    attachInterrupt(
        digitalPinToInterrupt(SENSOR_PIN),
        detectObject,
        RISING
    );

    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED) {

        delay(500);

        Serial.print(".");
    }

    Serial.println("\nConnected!");

    xTaskCreatePinnedToCore(
        uploadTask,
        "Upload Task",
        4096,
        NULL,
        1,
        NULL,
        0
    );

    xTaskCreatePinnedToCore(
        printTask,
        "Print Task",
        2048,
        NULL,
        1,
        NULL,
        1
    );
}

void loop() {}
