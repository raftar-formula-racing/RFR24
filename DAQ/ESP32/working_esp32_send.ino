#include "driver/twai.h"
#include<stdlib.h>

// Pins used to connect to CAN bus transceiver:
#define RX_PIN 21
#define TX_PIN 22
const int analogInPin0 = 13;
const int analogInPin1 = 15;

// Interval:
#define TRANSMIT_RATE_MS 100
#define POLLING_RATE_MS 100

static bool driver_installed = false;
unsigned long previousMillis = 0;  // will store last time a message was sent

int sensorValue1 = 0;
int sensorValue0 = 0;

void send_message(uint32_t identifier, String hexString1, String hexString2, String hexString3, String hexString4, uint8_t data_length) {
  // Extract the first two characters of each hexString
  uint8_t data[4];
  data[0] = strtol(hexString1.c_str(), NULL, 16);
  data[1] = strtol(hexString2.c_str(), NULL, 16);
  data[2] = strtol(hexString3.c_str(), NULL, 16);
  data[3] = strtol(hexString4.c_str(), NULL, 16);

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

void setup() {
  // Start Serial:
  Serial.begin(115200);

  // Initialize configuration structures using macro initializers
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)TX_PIN, (gpio_num_t)RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();  // Look in the api-reference for other speed sets.
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

  // TWAI driver is now successfully installed and started
  driver_installed = true;
}

void loop() {
  if (!driver_installed) {
    // Driver not installed
    delay(1000);
    return;
  }

  // Check if alert happened (unchanged)

  sensorValue0 = analogRead(analogInPin0);
  sensorValue1 = analogRead(analogInPin1);

  // Send variable CAN messages
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= TRANSMIT_RATE_MS) {
    previousMillis = currentMillis;

    // String hexString1 = String(sensorValue0, HEX);
    // String hexString1WithZero = "0" + hexString1;
    // String substring1 = hexString1WithZero.substring(0, 2);
    // String substring2 = hexString1WithZero.substring(2);

    // String hexString2 = String(sensorValue1, HEX);
    // String hexString2WithZero = "0" + hexString2;
    // String substring3 = hexString2WithZero.substring(0, 2);
    // String substring4 = hexString2WithZero.substring(2);

String hexString1 = String(sensorValue0, HEX);
String hexString1WithZero;

if(hexString1.length() == 2) {
  hexString1WithZero = "00" + hexString1;
} else {
  hexString1WithZero = "0" + hexString1;
}

String substring1 = hexString1WithZero.substring(0, 2);
String substring2 = hexString1WithZero.substring(2);

String hexString2 = String(sensorValue1, HEX);
String hexString2WithZero;
Serial.println(hexString2.length());
if(hexString2.length() == 2) {
  hexString2WithZero = "00" + hexString2;
} else {
  hexString2WithZero = "0" + hexString2;
}

String substring3 = hexString2WithZero.substring(0, 2);
String substring4 = hexString2WithZero.substring(2);


   // Serial.println(substring1);
   // delay(10);
    // Serial.println(substring2);
    // delay(10);
    // Serial.println(substring3);
    // delay(10);
    // Serial.println(substring4);

    // Example of sending a variable CAN message with all hexStrings
    send_message(0x7DC, substring1, substring2, substring3, substring4, sizeof("ff") * 2);
    delay(50);
  }
}