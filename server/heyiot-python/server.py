from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs

from chirpstack_api import integration
from google.protobuf.json_format import Parse


class Handler(BaseHTTPRequestHandler):
    # True - JSON marshaler (matches the JSON payload encoding configured in the console)
    # False - Protobuf marshaler (binary)
    json = True

    def do_POST(self):
        """Handle incoming HTTP POST requests (Webhooks) from ChirpStack."""
        self.send_response(200)
        self.end_headers()
        
        # Parse query parameters from the request URL (e.g., ?event=up)
        query_args = parse_qs(urlparse(self.path).query)

        content_len = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_len)

        # Route the request based on the 'event' query parameter
        if "event" in query_args:
            event_type = query_args["event"][0]
            if event_type == "up":
                self.up(body)
            elif event_type == "join":
                self.join(body)
            else:
                print("Handler for event %s is not implemented" % event_type)
        else:
            print("No event query parameter found in request path.")

    def up(self, body):
        """Handle uplink events sent by the device (such as SHTC3 sensor data)[cite: 4]."""
        up_event = self.unmarshal(body, integration.UplinkEvent())
        
        print("\n" + "=" * 50)
        print(f"Uplink received from device: {up_event.device_info.device_name}")
        print(f"Device EUI: {up_event.device_info.dev_eui}")
        
        # If a JavaScript payload codec is configured in the console, 
        # decoded fields (like temperature and humidity) will be available in up_event.object
        if up_event.object:
            print(f"Decoded Data Object: {up_event.object}")
        else:
            print(f"Raw Payload (Hex): {up_event.data.hex()}")
        print("=" * 50)

    def join(self, body):
        """Handle device OTAA join events."""
        join_event = self.unmarshal(body, integration.JoinEvent())
        
        print("\n" + "=" * 50)
        print(f"Device Joined: {join_event.device_info.device_name}")
        print(f"Device EUI: {join_event.device_info.dev_eui}")
        print(f"Assigned DevAddr: {join_event.dev_addr}")
        print("=" * 50)

    def unmarshal(self, body, pl):
        """Unmarshal the request body into the appropriate Protobuf message object."""
        if self.json:
            return Parse(body, pl)
        
        pl.ParseFromString(body)
        return pl


if __name__ == "__main__":
    server_address = ('', 8090)
    httpd = HTTPServer(server_address, Handler)
    print("Starting HTTP server on port 8090 to listen to ChirpStack webhooks...")
    
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down HTTP server.")
        httpd.server_close()