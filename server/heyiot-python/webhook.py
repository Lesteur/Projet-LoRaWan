import json
import base64
import logging

from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse, parse_qs


# Server configuration
HOST = "0.0.0.0"
PORT = 8090

# Logging configuration
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s"
)


class ChirpStackHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        """Handle GET requests for server health checks."""

        if self.path == "/health":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(
                json.dumps({"status": "running"}).encode("utf-8")
            )
        else:
            self.send_error(404, "Endpoint not found")

    def do_POST(self):
        """Handle incoming ChirpStack HTTP integration events."""

        try:
            # Extract event type from URL query parameters
            query_args = parse_qs(urlparse(self.path).query)
            event_type = query_args.get("event", [None])[0]

            if event_type not in ["up", "join"]:
                self.send_error(400, "Unsupported or missing event type")
                return

            # Read HTTP request body
            content_length = int(self.headers.get("Content-Length", 0))

            if content_length <= 0:
                self.send_error(400, "Empty request body")
                return

            body = self.rfile.read(content_length)

            # This first version expects JSON payloads
            content_type = self.headers.get("Content-Type", "")

            if "application/json" not in content_type:
                self.send_error(
                    415,
                    "This test server currently expects JSON data"
                )
                return

            message = json.loads(body.decode("utf-8"))

            # Process the event
            if event_type == "up":
                self.handle_uplink(message)

            elif event_type == "join":
                self.handle_join(message)

            # Return successful HTTP response
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(
                json.dumps({"status": "received"}).encode("utf-8")
            )

        except json.JSONDecodeError:
            logging.exception("Invalid JSON received")
            self.send_error(400, "Invalid JSON")

        except Exception:
            logging.exception("Error while processing request")
            self.send_error(500, "Internal server error")

    def handle_uplink(self, message):
        """Process an uplink event."""

        device_info = message.get("deviceInfo", {})
        dev_eui = device_info.get("devEui", "Unknown")
        device_name = device_info.get("deviceName", "Unknown")

        logging.info("Uplink received")
        logging.info("Device name: %s", device_name)
        logging.info("DevEUI: %s", dev_eui)

        # Extract the Base64 payload
        encoded_data = message.get("data")

        if not encoded_data:
            logging.warning("No payload data found")
            return

        # Decode Base64 into raw bytes
        raw_data = base64.b64decode(encoded_data, validate=True)

        logging.info("Raw payload: %s", raw_data.hex())

        # Decode the sensor measurements
        measurements = self.decode_sensor_data(raw_data)

        if measurements is None:
            logging.warning("Payload format is not valid")
            return

        temperature = measurements["temperature"]
        humidity = measurements["humidity"]

        logging.info("Temperature: %.2f °C", temperature)
        logging.info("Humidity: %.2f %%", humidity)

        # Display decoded object if provided by ChirpStack
        decoded_object = message.get("object")

        if decoded_object:
            logging.info("Decoded object: %s", decoded_object)

        logging.info("-" * 50)

    def handle_join(self, message):
        """Process a device join event."""

        device_info = message.get("deviceInfo", {})
        dev_eui = device_info.get("devEui", "Unknown")
        dev_addr = message.get("devAddr", "Unknown")

        logging.info("Device joined the network")
        logging.info("DevEUI: %s", dev_eui)
        logging.info("DevAddr: %s", dev_addr)
        logging.info("-" * 50)

    @staticmethod
    def decode_sensor_data(data):
        """
        Decode temperature and humidity.

        Expected payload:
        Bytes 0-1: Temperature (signed integer, Big Endian)
        Bytes 2-3: Humidity (unsigned integer, Big Endian)
        Both values are divided by 100.
        """

        if len(data) != 4:
            return None

        # Decode signed temperature
        raw_temp = (data[0] << 8) | data[1]

        if raw_temp & 0x8000:
            raw_temp -= 0x10000

        temperature = raw_temp / 100.0

        # Decode unsigned humidity
        raw_humidity = (data[2] << 8) | data[3]

        humidity = raw_humidity / 100.0

        return {
            "temperature": temperature,
            "humidity": humidity
        }

    def log_message(self, format, *args):
        """Use the configured logger instead of the default HTTP output."""

        logging.info(format % args)


if __name__ == "__main__":

    server = ThreadingHTTPServer((HOST, PORT), ChirpStackHandler)

    logging.info("ChirpStack HTTP server starting")
    logging.info("Listening on http://%s:%s", HOST, PORT)

    try:
        server.serve_forever()

    except KeyboardInterrupt:
        logging.info("Server stopped")

    finally:
        server.server_close()
