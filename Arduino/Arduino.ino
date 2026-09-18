#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// ============================================================
// LoRaWAN OTAA credentials
// ============================================================
//
// IMPORTANT:
// Never publish your AppKey.
// These values must correspond to the device registered
// in your LoRaWAN network/server.

// ============================================================

static const u1_t PROGMEM APPEUI[8] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01
};

static const u1_t PROGMEM DEVEUI[8] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01
};

static const u1_t PROGMEM APPKEY[16] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01
};


// ============================================================
// LMIC callbacks
// ============================================================

void os_getArtEui(u1_t* buf)
{
    memcpy_P(buf, APPEUI, 8);
}

void os_getDevEui(u1_t* buf)
{
    memcpy_P(buf, DEVEUI, 8);
}

void os_getDevKey(u1_t* buf)
{
    memcpy_P(buf, APPKEY, 16);
}


// ============================================================
// LoRa module pin configuration
// ============================================================
//
// THIS IS AN EXAMPLE.
//
// You MUST change these pins according to your ESP32 +
// LoRa module/board.
//
// For example, some boards use:
//   NSS  = 5
//   RST  = 14
//   DIO0 = 26
//   DIO1 = 33
//   DIO2 = 32
//
// Check your board documentation.
// ============================================================

const lmic_pinmap lmic_pins = {
    .nss = 5,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 14,
    .dio = {26, 33, 32}
};


// ============================================================
// Application configuration
// ============================================================

const unsigned TX_INTERVAL = 30; // Send one message every 30 seconds

static osjob_t sendjob;


// ============================================================
// Send a LoRaWAN packet
// ============================================================

void do_send(osjob_t* j)
{
    // Check whether another transmission is currently pending.
    if (LMIC.opmode & OP_TXRXPEND)
    {
        Serial.println("Transmission still pending...");
    }
    else
    {
        // Message to transmit.
        static uint8_t payload[] = "Hello LoRaWAN!";

        // Queue the packet.
        //
        // Port 1 is used by the application.
        // The last parameter is 0 because we are not asking
        // LMIC to confirm the packet at the application level.
        LMIC_setTxData2(
            1,
            payload,
            sizeof(payload) - 1,
            0
        );

        Serial.println("Packet queued.");
    }

    // The next transmission will be scheduled after
    // the current LoRaWAN transmission has completed.
}


// ============================================================
// LMIC event handler
// ============================================================

void onEvent(ev_t ev)
{
    Serial.print("LMIC event: ");

    switch (ev)
    {
        case EV_JOINING:
            Serial.println("EV_JOINING");
            break;

        case EV_JOINED:
            Serial.println("EV_JOINED");

            // Disable link check validation because some
            // LoRaWAN networks do not provide it in the way
            // LMIC expects.
            LMIC_setLinkCheckMode(0);

            // Send the first packet.
            do_send(&sendjob);
            break;

        case EV_JOIN_FAILED:
            Serial.println("EV_JOIN_FAILED");
            break;

        case EV_REJOIN_FAILED:
            Serial.println("EV_REJOIN_FAILED");
            break;

        case EV_TXCOMPLETE:
            Serial.println("EV_TXCOMPLETE");

            // Check whether we received a downlink packet.
            if (LMIC.txrxFlags & TXRX_ACK)
            {
                Serial.println("Received an ACK.");
            }

            if (LMIC.dataLen > 0)
            {
                Serial.print("Received downlink: ");

                for (int i = 0; i < LMIC.dataLen; i++)
                {
                    Serial.printf(
                        "%02X ",
                        LMIC.frame[LMIC.dataBeg + i]
                    );
                }

                Serial.println();
            }

            // Schedule the next transmission.
            os_setTimedCallback(
                &sendjob,
                os_getTime() + sec2osticks(TX_INTERVAL),
                do_send
            );

            break;

        default:
            Serial.println("Other event");
            break;
    }
}


// ============================================================
// Setup
// ============================================================

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println("ESP32 LoRaWAN example");
    Serial.println("================================");

    // Initialize LMIC.
    os_init();

    // Reset the LMIC state.
    LMIC_reset();

    // Configure the LoRaWAN region.
    //
    // For France, the usual regional configuration is EU868.
    //
    // The exact configuration depends on your LMIC version.
    // For the MCCI LMIC library, EU868 is normally selected
    // through the library's project configuration.
    //
    // Do not blindly copy regional settings from another
    // country.
}


// ============================================================
// Main loop
// ============================================================

void loop()
{
    // LMIC is event-driven.
    //
    // This function processes pending LoRaWAN events such as:
    // - joining
    // - transmission
    // - reception
    // - acknowledgements
    os_runloop_once();
}