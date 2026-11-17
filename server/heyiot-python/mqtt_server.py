import os
import json
import ssl
import logging

import paho.mqtt.client as mqtt


# ============================================================
# MQTT CONFIGURATION
# ============================================================

MQTT_BROKER = os.getenv("MQTT_BROKER", "helium.disk91.com")
MQTT_PORT = int(os.getenv("MQTT_PORT", "443"))

MQTT_CLIENT_ID = os.getenv("MQTT_CLIENT_ID", "adam_florian_christophe")

MQTT_USERNAME = os.getenv("MQTT_USERNAME", "HelloDisk91")
MQTT_PASSWORD = os.getenv("MQTT_PASSWORD", "123")

MQTT_TOPIC = os.getenv("MQTT_TOPIC", "ChristopheFlorianAdam/{{device_id}}/dw")

MQTT_KEEPALIVE = 60

# TLS configuration
MQTT_USE_TLS = True

# Optional path to a custom Certificate Authority certificate
MQTT_CA_CERT = os.getenv("MQTT_CA_CERT")


# ============================================================
# LOGGING CONFIGURATION
# ============================================================

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s | %(levelname)s | %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S"
)

logger = logging.getLogger(__name__)


# ============================================================
# MQTT CALLBACKS
# ============================================================

def on_connect(client, userdata, flags, reason_code, properties):
    """
    Callback executed when the client connects to the broker.
    """

    if reason_code.is_failure:
        logger.error(f"Connection failed: {reason_code}")
        return

    logger.info("Successfully connected to MQTT broker.")

    # Subscribe to the requested topic
    client.subscribe(MQTT_TOPIC, qos=1)

    logger.info(f"Subscribed to topic: {MQTT_TOPIC}")


def on_message(client, userdata, message):
    """
    Callback executed whenever a new MQTT message is received.
    """

    try:
        # Decode the incoming payload
        payload = message.payload.decode("utf-8")

        logger.info(f"Message received on topic: {message.topic}")

        # Try to interpret the payload as JSON
        try:
            data = json.loads(payload)

            logger.info("Received JSON data:")
            logger.info(json.dumps(data, indent=4, ensure_ascii=False))

            # Data processing can be implemented here

        except json.JSONDecodeError:
            logger.info(f"Received raw payload: {payload}")

    except UnicodeDecodeError:
        logger.error("Unable to decode incoming message.")


def on_disconnect(client, userdata, disconnect_flags, reason_code, properties):
    """
    Callback executed when the client disconnects.
    """

    if reason_code.is_failure:
        logger.warning(f"Unexpected disconnection: {reason_code}")
    else:
        logger.info("Disconnected from MQTT broker.")


# ============================================================
# MQTT CLIENT INITIALIZATION
# ============================================================

def create_mqtt_client():

    # Create MQTT client using the Paho MQTT 2.x callback API
    client = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id=MQTT_CLIENT_ID,
        protocol=mqtt.MQTTv311
    )

    # Configure authentication
    if MQTT_USERNAME and MQTT_PASSWORD:
        client.username_pw_set(
            username=MQTT_USERNAME,
            password=MQTT_PASSWORD
        )

    # Configure TLS encryption
    if MQTT_USE_TLS:

        tls_context = ssl.create_default_context(
            cafile=MQTT_CA_CERT
        )

        tls_context.check_hostname = True
        tls_context.verify_mode = ssl.CERT_REQUIRED

        client.tls_set_context(tls_context)

    # Register callbacks
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_disconnect = on_disconnect

    return client


# ============================================================
# MAIN APPLICATION
# ============================================================

def main():

    logger.info("Starting MQTT receiver...")

    client = create_mqtt_client()

    try:

        logger.info(f"Connecting to {MQTT_BROKER}:{MQTT_PORT}")

        client.connect(
            host=MQTT_BROKER,
            port=MQTT_PORT,
            keepalive=MQTT_KEEPALIVE
        )

        # Start the MQTT network loop
        # This function automatically handles reconnections
        client.loop_forever(retry_first_connection=True)

    except KeyboardInterrupt:

        logger.info("Application interrupted by user.")

    except Exception as error:

        logger.exception(f"MQTT connection error: {error}")

    finally:

        client.disconnect()
        logger.info("MQTT receiver stopped.")


# ============================================================
# ENTRY POINT
# ============================================================

if __name__ == "__main__":
    main()
