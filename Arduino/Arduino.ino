#include <Arduino.h>
#include <LoRaWan-RAK4630.h>
#include <Wire.h>
#include "SparkFun_SHTC3.h"


// =======================================================================
// SHTC3 TEMPERATURE / HUMIDITY SENSOR
// =======================================================================

SHTC3 mySHTC3;


// =======================================================================
// LORAWAN CONFIGURATION
// =======================================================================
//
// This example uses OTAA (Over-The-Air Activation).
//
// You MUST replace the following values with the credentials
// configured for your device in your LoRaWAN network / HeyIoT Console.
//
// IMPORTANT:
// - DevEUI identifies the LoRaWAN device.
// - AppEUI / JoinEUI identifies the application.
// - AppKey is a secret cryptographic key.
//
// NEVER publish your real AppKey.
//
// The values below are examples only.
// =======================================================================

uint8_t nodeDeviceEUI[8] =
{
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01
};


uint8_t nodeAppEUI[8] =
{
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01
};


uint8_t nodeAppKey[16] =
{
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01
};


// =======================================================================
// LORAWAN PARAMETERS
// =======================================================================
//
// France uses the EU868 LoRaWAN regional band.
//
// Class A is the standard low-power class for battery-powered sensors.
// =======================================================================

DeviceClass_t g_CurrentClass = CLASS_A;

LoRaMacRegion_t g_CurrentRegion = LORAMAC_REGION_EU868;


// =======================================================================
// APPLICATION SETTINGS
// =======================================================================
//
// The device will send one packet every 30 seconds.
//
// 30000 ms = 30 seconds
//
// This short interval is useful for testing.
// For a battery-powered production device, a longer interval would
// normally be preferable.
// =======================================================================

#define LORAWAN_APP_INTERVAL 30000


// =======================================================================
// LORAWAN PAYLOAD
// =======================================================================
//
// Maximum application payload buffer.
//
// We only use 4 bytes:
//
// Byte 0-1 -> Temperature
// Byte 2-3 -> Humidity
// =======================================================================

uint8_t m_lora_app_data_buffer[64];

lmh_app_data_t m_lora_app_data =
{
    m_lora_app_data_buffer,
    0,
    0,
    0,
    0
};


// =======================================================================
// TIMER
// =======================================================================

TimerEvent_t appTimer;


// =======================================================================
// FUNCTION PROTOTYPES
// =======================================================================

void send_lora_frame(void);

void on_lorawan_has_joined_cb(void);

void on_lorawan_rx_data_cb(lmh_app_data_t *app_data);

void on_lorawan_confirm_class_cb(DeviceClass_t Class);

void on_lorawan_join_failed_cb(void);


// =======================================================================
// LORAWAN CALLBACK STRUCTURE
// =======================================================================
//
// These callbacks are called by the LoRaWAN stack when important
// LoRaWAN events occur.
// =======================================================================

static lmh_callback_t lora_callbacks =
{
    BoardGetBatteryLevel,
    BoardGetUniqueId,
    BoardGetRandomSeed,
    on_lorawan_rx_data_cb,
    on_lorawan_has_joined_cb,
    on_lorawan_confirm_class_cb,
    on_lorawan_join_failed_cb
};


// =======================================================================
// SETUP
// =======================================================================

void setup()
{
    // -------------------------------------------------------------------
    // Initialize Serial communication
    // -------------------------------------------------------------------

    Serial.begin(115200);

    unsigned long timeout = millis();

    while (!Serial && (millis() - timeout < 5000))
    {
        delay(100);
    }

    Serial.println();
    Serial.println("======================================");
    Serial.println("WisBlock RAK11300 LoRaWAN Node");
    Serial.println("Temperature + Humidity");
    Serial.println("======================================");


    // -------------------------------------------------------------------
    // Initialize I2C
    // -------------------------------------------------------------------

    Wire.begin();

    Serial.println("Initializing SHTC3 sensor...");


    // -------------------------------------------------------------------
    // Initialize the RAK1901 / SHTC3 sensor
    // -------------------------------------------------------------------

    if (mySHTC3.begin() != SHTC3_Status_Nominal)
    {
        Serial.println("ERROR: SHTC3 sensor initialization failed!");

        // Stop the program if the sensor cannot be initialized.
        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("SHTC3 sensor initialized successfully.");


    // -------------------------------------------------------------------
    // Initialize the RAK LoRaWAN hardware
    // -------------------------------------------------------------------

    Serial.println("Initializing LoRaWAN hardware...");

    uint32_t err_code = lora_rak4630_init();

    if (err_code != 0)
    {
        Serial.printf(
            "ERROR: LoRaWAN hardware initialization failed: %lu\n",
            err_code
        );

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("LoRaWAN hardware initialized successfully.");


    // -------------------------------------------------------------------
    // Configure LoRaWAN parameters
    // -------------------------------------------------------------------
    //
    // ADR:
    // Adaptive Data Rate is enabled.
    //
    // DR_3:
    // Initial data rate.
    //
    // PUBLIC NETWORK:
    // Standard LoRaWAN public network.
    //
    // TX POWER:
    // Use the default transmit power.
    // -------------------------------------------------------------------

    lmh_param_t lora_param_init =
    {
        LORAWAN_ADR_ON,
        DR_3,
        LORAWAN_PUBLIC_NETWORK,
        8,
        LORAWAN_DEFAULT_TX_POWER,
        LORAWAN_DUTYCYCLE_OFF
    };


    // -------------------------------------------------------------------
    // Initialize the LoRaWAN stack
    // -------------------------------------------------------------------

    err_code = lmh_init(
        &lora_callbacks,
        lora_param_init,
        true,
        g_CurrentClass,
        g_CurrentRegion
    );

    if (err_code != 0)
    {
        Serial.printf(
            "ERROR: LoRaWAN stack initialization failed: %lu\n",
            err_code
        );

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("LoRaWAN stack initialized successfully.");


    // -------------------------------------------------------------------
    // Configure OTAA credentials
    // -------------------------------------------------------------------

    lmh_setDevEui(nodeDeviceEUI);

    lmh_setAppEui(nodeAppEUI);

    lmh_setAppKey(nodeAppKey);


    // -------------------------------------------------------------------
    // Initialize the application timer
    // -------------------------------------------------------------------

    TimerInit(
        &appTimer,
        send_lora_frame
    );


    // -------------------------------------------------------------------
    // Start the LoRaWAN OTAA join procedure
    // -------------------------------------------------------------------

    Serial.println();
    Serial.println("Starting LoRaWAN OTAA join...");
    Serial.println();

    lmh_join();
}


// =======================================================================
// MAIN LOOP
// =======================================================================
//
// The RAK LoRaWAN stack handles the LoRaWAN events and timers.
//
// Therefore, the application loop does not need to perform the
// transmission itself.
// =======================================================================

void loop()
{
    // Do not put blocking operations here.

    delay(10);
}


// =======================================================================
// CALLBACK: LORAWAN JOIN SUCCESS
// =======================================================================
//
// This function is called when the device successfully joins the
// LoRaWAN network using OTAA.
// =======================================================================

void on_lorawan_has_joined_cb(void)
{
    Serial.println();
    Serial.println("======================================");
    Serial.println("LoRaWAN JOIN SUCCESSFUL!");
    Serial.println("======================================");

    // Send the first measurement immediately.
    send_lora_frame();
}


// =======================================================================
// CALLBACK: LORAWAN JOIN FAILURE
// =======================================================================

void on_lorawan_join_failed_cb(void)
{
    Serial.println();
    Serial.println("LoRaWAN JOIN FAILED.");
    Serial.println("Retrying...");
    Serial.println();

    // Try to join the network again.
    lmh_join();
}


// =======================================================================
// CALLBACK: DOWNLINK RECEIVED
// =======================================================================
//
// This function is called when the LoRaWAN network sends data back
// to the device.
// =======================================================================

void on_lorawan_rx_data_cb(lmh_app_data_t *app_data)
{
    Serial.println();
    Serial.println("Downlink received!");

    Serial.printf(
        "Port: %d\n",
        app_data->port
    );

    Serial.printf(
        "Payload size: %d bytes\n",
        app_data->buffsize
    );

    Serial.print("Payload: ");

    for (uint8_t i = 0; i < app_data->buffsize; i++)
    {
        Serial.printf(
            "%02X ",
            app_data->buffer[i]
        );
    }

    Serial.println();
}


// =======================================================================
// CALLBACK: LORAWAN CLASS CONFIRMATION
// =======================================================================

void on_lorawan_confirm_class_cb(DeviceClass_t Class)
{
    Serial.printf(
        "LoRaWAN class confirmed: %d\n",
        Class
    );
}


// =======================================================================
// FUNCTION: READ SENSOR AND SEND LORAWAN PACKET
// =======================================================================

void send_lora_frame(void)
{
    // -------------------------------------------------------------------
    // Make sure the device has successfully joined the network.
    // -------------------------------------------------------------------

    if (lmh_join_status_get() != LMH_SET)
    {
        Serial.println(
            "Cannot send packet: device is not joined."
        );

        return;
    }


    // -------------------------------------------------------------------
    // Read the SHTC3 sensor
    // -------------------------------------------------------------------

    mySHTC3.update();

    float temperature = mySHTC3.toDegC();

    float humidity = mySHTC3.toPercent();


    // -------------------------------------------------------------------
    // Display the measurements on the Serial Monitor
    // -------------------------------------------------------------------

    Serial.println();
    Serial.println("--------------------------------------");

    Serial.printf(
        "Temperature: %.2f C\n",
        temperature
    );

    Serial.printf(
        "Humidity: %.2f %%\n",
        humidity
    );


    // -------------------------------------------------------------------
    // Encode the measurements
    // -------------------------------------------------------------------
    //
    // We multiply the values by 100.
    //
    // Example:
    //
    // 23.57 C
    //
    // becomes:
    //
    // 2357
    //
    // This allows us to transmit the value as an integer instead of
    // sending a floating-point number.
    //
    // Temperature uses int16_t because it must support negative
    // temperatures.
    //
    // Humidity uses uint16_t because humidity cannot normally be
    // negative.
    // -------------------------------------------------------------------

    int16_t temperature_encoded =
        (int16_t)(temperature * 100.0f);

    uint16_t humidity_encoded =
        (uint16_t)(humidity * 100.0f);


    // -------------------------------------------------------------------
    // Prepare the LoRaWAN application payload
    // -------------------------------------------------------------------

    m_lora_app_data.buffsize = 0;

    // Application port.
    //
    // Port 2 is used by this application.
    m_lora_app_data.port = 2;


    // -------------------------------------------------------------------
    // Encode temperature
    // -------------------------------------------------------------------
    //
    // Temperature occupies bytes 0 and 1.
    //
    // Big-endian format:
    //
    // Byte 0 = most significant byte
    // Byte 1 = least significant byte
    // -------------------------------------------------------------------

    m_lora_app_data.buffer[
        m_lora_app_data.buffsize++
    ] = (temperature_encoded >> 8) & 0xFF;

    m_lora_app_data.buffer[
        m_lora_app_data.buffsize++
    ] = temperature_encoded & 0xFF;


    // -------------------------------------------------------------------
    // Encode humidity
    // -------------------------------------------------------------------
    //
    // Humidity occupies bytes 2 and 3.
    // -------------------------------------------------------------------

    m_lora_app_data.buffer[
        m_lora_app_data.buffsize++
    ] = (humidity_encoded >> 8) & 0xFF;

    m_lora_app_data.buffer[
        m_lora_app_data.buffsize++
    ] = humidity_encoded & 0xFF;


    // -------------------------------------------------------------------
    // Display the payload before transmission
    // -------------------------------------------------------------------

    Serial.print("Payload: ");

    for (uint8_t i = 0; i < m_lora_app_data.buffsize; i++)
    {
        Serial.printf(
            "%02X ",
            m_lora_app_data.buffer[i]
        );
    }

    Serial.println();


    // -------------------------------------------------------------------
    // Send the LoRaWAN uplink
    // -------------------------------------------------------------------
    //
    // LMH_UNCONFIRMED_MSG means that the application does not request
    // a LoRaWAN acknowledgement for this packet.
    // -------------------------------------------------------------------

    lmh_error_status error =
        lmh_send(
            &m_lora_app_data,
            LMH_UNCONFIRMED_MSG
        );


    // -------------------------------------------------------------------
    // Check transmission status
    // -------------------------------------------------------------------

    if (error == LMH_SUCCESS)
    {
        Serial.println(
            "LoRaWAN packet queued successfully."
        );
    }
    else
    {
        Serial.printf(
            "ERROR: LoRaWAN packet transmission failed: %d\n",
            error
        );
    }


    // -------------------------------------------------------------------
    // Schedule the next transmission
    // -------------------------------------------------------------------

    TimerSetValue(
        &appTimer,
        LORAWAN_APP_INTERVAL
    );

    TimerStart(
        &appTimer
    );

    Serial.println("--------------------------------------");
}
