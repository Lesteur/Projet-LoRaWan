#include <Arduino.h>
#include <LoRaWan-RAK4630.h> // Library for LoRaWAN (adjust if using a different core)
#include <SPI.h>
#include <Wire.h>
#include "SparkFun_SHTC3.h"  // Library for the RAK1901 temperature sensor

SHTC3 mySHTC3; // Create an instance of the sensor

// =======================================================================
// LORAWAN CONFIGURATION
// You MUST replace these arrays with the keys from your HeyIoT Console!
// The format is MSB (Most Significant Bit first).
// =======================================================================
uint8_t nodeDeviceEUI[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t nodeAppEUI[8]    = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t nodeAppKey[16]   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Set the region for LoRaWAN (EU868 for Europe, US915 for North America)
DeviceClass_t  g_CurrentClass = CLASS_A;
LoRaMacRegion_t g_CurrentRegion = LORAMAC_REGION_EU868;

// Time between two transmissions (in milliseconds) - e.g., 60 seconds
#define LORAWAN_APP_INTERVAL 30000 

// Buffer to hold the payload data to send
uint8_t m_lora_app_data_buffer[64];
lmh_app_data_t m_lora_app_data = {m_lora_app_data_buffer, 0, 0, 0, 0};

// Timer for scheduling the next transmission
TimerEvent_t appTimer;

// Function prototypes
void send_lora_frame(void);
void on_lorawan_has_joined_cb(void);
void on_lorawan_rx_data_cb(lmh_app_data_t *app_data);
void on_lorawan_confirm_class_cb(DeviceClass_t Class);
void on_lorawan_join_failed_cb(void);

// LoRaWAN event callbacks structure
static lmh_callback_t lora_callbacks = {
  BoardGetBatteryLevel,
  BoardGetUniqueId,
  BoardGetRandomSeed,
  on_lorawan_rx_data_cb,
  on_lorawan_has_joined_cb,
  on_lorawan_confirm_class_cb,
  on_lorawan_join_failed_cb
};

// =======================================================================
// SETUP: Initializes serial, sensor, and LoRaWAN
// =======================================================================
void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  time_t timeout = millis();
  while (!Serial && (millis() - timeout < 5000)) delay(100); 

  Serial.println("=====================================");
  Serial.println("WisBlock LoRaWAN Temperature Node");
  Serial.println("=====================================");

  // Initialize the I2C bus and the temperature sensor
  Wire.begin();
  if (mySHTC3.begin() != SHTC3_Status_Nominal) {
    Serial.println("SHTC3 sensor initialization failed!");
    while (1) delay(100); 
  }
  Serial.println("SHTC3 sensor initialized successfully.");

  // Initialize LoRaWAN setup
  uint32_t err_code = lora_rak4630_init();
  if (err_code != 0) {
    Serial.printf("LoRaWAN init failed - Error: %d\n", err_code);
    return;
  }

  // Setup LoRaWAN parameters
  lmh_param_t lora_param_init = {
    LORAWAN_ADR_ON,
    DR_3,
    LORAWAN_PUBLIC_NETWORK,
    8,
    LORAWAN_DEFAULT_TX_POWER,
    LORAWAN_DUTYCYCLE_OFF
  };

  // Initialize the LoRaWAN stack
  err_code = lmh_init(&lora_callbacks, lora_param_init, true, g_CurrentClass, g_CurrentRegion);
  if (err_code != 0) {
    Serial.printf("lmh_init failed - Error: %d\n", err_code);
    return;
  }

  // Set the keys for OTAA activation
  lmh_setDevEui(nodeDeviceEUI);
  lmh_setAppEui(nodeAppEUI);
  lmh_setAppKey(nodeAppKey);

  // Initialize the timer for periodic transmission
  TimerInit(&appTimer, send_lora_frame);
  
  Serial.println("Starting LoRaWAN join process...");
  lmh_join();
}

void loop() {
  // Handle LoRaWAN events and timers
  // Do NOT put heavy blocking code in loop() when using LoRaWAN
}

// =======================================================================
// CALLBACK: Triggered when successfully connected to the network
// =======================================================================
void on_lorawan_has_joined_cb(void) {
  Serial.println("Network Joined Successfully!");
  send_lora_frame(); // Start sending data now that we are connected
}

void on_lorawan_join_failed_cb(void) {
  Serial.println("Network Join Failed. Will retry...");
  lmh_join();
}

void on_lorawan_rx_data_cb(lmh_app_data_t *app_data) {
  Serial.printf("DReceived data on port %d", app_data->port);
}

void on_lorawan_confirm_class_cb(DeviceClass_t Class) {
  Serial.printf("Class LoRaWAN Successful : %d\n", Class);
}

// =======================================================================
// FUNCTION: Reads temperature and sends the LoRaWAN packet
// =======================================================================
void send_lora_frame(void) {
  if (lmh_join_status_get() != LMH_SET) return; // Not joined, do nothing

  // 1. Read temperature and humidity from the sensor
  mySHTC3.update();
  float temp_c = mySHTC3.toDegC();
  float humidity = mySHTC3.toPercent();

  Serial.printf("Temperature: %.2f °C, Humidity: %.2f %%\n", temp_c, humidity);

  // 2. Prepare the payload (data to send)
  // We multiply by 100 and send as integers to avoid sending heavy floating point numbers
  uint16_t temp_int = (uint16_t)(temp_c * 100);
  uint16_t hum_int = (uint16_t)(humidity * 100);

  // Reset payload size
  m_lora_app_data.buffsize = 0;
  m_lora_app_data.port = 2; // Arbitrary application port

  // Add temperature data to payload (2 bytes)
  m_lora_app_data.buffer[m_lora_app_data.buffsize++] = (temp_int >> 8) & 0xFF; // High byte
  m_lora_app_data.buffer[m_lora_app_data.buffsize++] = temp_int & 0xFF;        // Low byte

  // Add humidity data to payload (2 bytes)
  m_lora_app_data.buffer[m_lora_app_data.buffsize++] = (hum_int >> 8) & 0xFF;  // High byte
  m_lora_app_data.buffer[m_lora_app_data.buffsize++] = hum_int & 0xFF;         // Low byte

  // 3. Send the packet
  lmh_error_status error = lmh_send(&m_lora_app_data, LMH_UNCONFIRMED_MSG);
  
  if (error == LMH_SUCCESS) {
    Serial.println("Packet sent to network.");
  } else {
    Serial.printf("Failed to send packet - Error: %d\n", error);
  }

  // 4. Schedule the next transmission
  TimerSetValue(&appTimer, LORAWAN_APP_INTERVAL);
  TimerStart(&appTimer);
}