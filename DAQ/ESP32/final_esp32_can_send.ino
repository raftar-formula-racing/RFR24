#include "driver/twai.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Pins used to connect to CAN bus transceiver:
#define RX_PIN 21
#define TX_PIN 22
const int analogInPin0 = 25;
const int analogInPin1 = 32;
const int analogInPin2 = 34;

// Interval:
#define TRANSMIT_RATE_MS 100
#define POLLING_RATE_MS 10

static bool driver_installed = false;
SemaphoreHandle_t adcSemaphore;

int sensorValue1 = 0;
int sensorValue0 = 0;
int sensorValue2 = 0;

void send_message(uint32_t identifier, uint8_t *data, uint8_t data_length) {
  // Configure message to transmit
  twai_message_t message;
  message.identifier = identifier;
  message.data_length_code = data_length;

  for (int i = 0; i < data_length; i++) {
    message.data[i] = data[i];
  }

  // Queue message for transmission
  if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
    Serial.println("Message queued for transmission");
  } else {
    Serial.println("Failed to queue message for transmission");
  }
}

void adc_task(void *parameter) {
  for (;;) {
    // Read analog sensor values
    sensorValue0 = analogRead(analogInPin0);
    sensorValue1 = analogRead(analogInPin1);
    sensorValue2 = analogRead(analogInPin2);
    // Release semaphore to indicate ADC reading is complete
    xSemaphoreGive(adcSemaphore);
    vTaskDelay(pdMS_TO_TICKS(POLLING_RATE_MS));
  }
}

void can_task(void *parameter) {
  uint8_t data[9];

  for (;;) {
    // Wait for semaphore to indicate ADC reading is complete
    if (xSemaphoreTake(adcSemaphore, portMAX_DELAY) == pdTRUE) {
      // Convert sensor values to hexadecimal strings
      String hexString1 = String(sensorValue0, HEX);
      String hexString2 = String(sensorValue1, HEX);
      String hexString3 = String(sensorValue2, HEX);

      // Ensure hex strings have leading zeros if necessary
      if (hexString1.length() == 1) {
        hexString1 = "0" + hexString1;
      }
      if (hexString2.length() == 1) {
        hexString2 = "0" + hexString2;
      }
       if (hexString3.length() == 1) {
        hexString3 = "0" + hexString3;
      }

      // Copy hexadecimal strings into data array
      data[0] = strtol(hexString1.substring(0, 2).c_str(), NULL, 16);
      data[1] = strtol(hexString1.substring(2).c_str(), NULL, 16);
      data[2] = strtol(hexString2.substring(0, 2).c_str(), NULL, 16);
      data[3] = strtol(hexString2.substring(2).c_str(), NULL, 16);
      data[4] = strtol(hexString3.substring(0, 2).c_str(), NULL, 16);
      data[5] = strtol(hexString3.substring(2).c_str(), NULL, 16);

      // Send CAN message
      send_message(0x7FF, data, 8);
    }
  }
}

void setup() {
  // Start Serial:
  Serial.begin(115200);

  // Initialize semaphore for ADC task synchronization
  adcSemaphore = xSemaphoreCreateBinary();

  // Initialize configuration structures using macro initializers
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)TX_PIN, (gpio_num_t)RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();  // Look in the api-reference for other speed sets.
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // Install TWAI driver
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    Serial.println("Driver installed");
  } else {
    Serial.println("Failed to install driver");
    return;
  }

  // Start TWAI driver
  if (twai_start() == ESP_OK) {
    Serial.println("Driver started");
  } else {
    Serial.println("Failed to start driver");
    return;
  }

  // Reconfigure alerts to detect TX alerts and Bus-Off errors
  uint32_t alerts_to_enable = TWAI_ALERT_TX_IDLE | TWAI_ALERT_TX_SUCCESS | TWAI_ALERT_TX_FAILED | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR;
  if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK) {
    Serial.println("CAN Alerts reconfigured");
  } else {
    Serial.println("Failed to reconfigure alerts");
    return;
  }

  // Create semaphore with initial count 0, as ADC task should run first
  vSemaphoreCreateBinary(adcSemaphore);
  xSemaphoreTake(adcSemaphore, 0);

  // Create tasks for ADC and CAN
  xTaskCreatePinnedToCore(adc_task, "ADC Task", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(can_task, "CAN Task", 4096, NULL, 1, NULL, 1);

  // TWAI driver is now successfully installed and started
  driver_installed = true;
}

void loop() {
  // No need for any operation here as tasks handle all functionality
}
