import json
import logging
import os
import time
from urllib.parse import urlparse

import requests
from dotenv import load_dotenv

# ============================================================
# CONFIGURATION
# ============================================================

load_dotenv()

API_URL = os.getenv("HEYIOT_API_URL", "").strip()
API_TOKEN = os.getenv("HEYIOT_API_TOKEN", "").strip()
DEVICE_ID = os.getenv("HEYIOT_DEVICE_ID", "").strip()

POLL_INTERVAL = int(os.getenv("POLL_INTERVAL", "10"))
REQUEST_TIMEOUT = 15

# ============================================================
# LOGGING
# ============================================================

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
)

logger = logging.getLogger(__name__)

# ============================================================
# CONFIGURATION VALIDATION
# ============================================================

def validate_configuration():
    """Validate the application configuration."""
    if not API_URL:
        raise ValueError("HEYIOT_API_URL is missing from the .env file.")

    parsed_url = urlparse(API_URL)
    if parsed_url.scheme != "https" or not parsed_url.netloc:
        raise ValueError("HEYIOT_API_URL must be a valid HTTPS URL.")

    if POLL_INTERVAL < 1:
        raise ValueError("POLL_INTERVAL must be at least 1 second.")

    logger.info("Configuration loaded successfully.")

    if DEVICE_ID:
        logger.info("Device ID configured: %s", DEVICE_ID)

    if API_TOKEN:
        logger.info("API token configured.")
    else:
        logger.warning("No API token configured; requests may fail if authentication is required.")

# ============================================================
# API REQUEST
# ============================================================

def fetch_device_data(session):
    """Request data from the configured HeyIoT API endpoint."""
    headers = {
        "Accept": "application/json",
        "User-Agent": "HeyIoT-Python-Client/1.0",
    }

    if API_TOKEN:
        headers["Authorization"] = f"Bearer {API_TOKEN}"

    params = {}
    if DEVICE_ID:
        # Selon l'API, le paramètre peut s'appeler 'device_id', 'dev_eui', etc.
        params["device_id"] = DEVICE_ID

    logger.info("Sending an HTTPS request to the API.")

    response = session.get(
        API_URL,
        headers=headers,
        params=params,
        timeout=REQUEST_TIMEOUT,
    )

    response.raise_for_status()

    try:
        data = response.json()
    except requests.exceptions.JSONDecodeError as exc:
        raise ValueError("The API response is not valid JSON.") from exc

    return response.status_code, data

# ============================================================
# DATA DISPLAY
# ============================================================

def display_data(status_code, data):
    """Display the HTTP status and the returned JSON data."""
    print("\n" + "=" * 60)
    print(f"HTTP status: {status_code}")
    print("Received data:")
    print("=" * 60)
    print(json.dumps(data, indent=4, ensure_ascii=False))
    print("=" * 60)

# ============================================================
# MAIN LOOP
# ============================================================

def main():
    validate_configuration()

    logger.info("Starting the HeyIoT API connectivity test.")
    logger.info("Polling interval: %s seconds", POLL_INTERVAL)

    with requests.Session() as session:
        while True:
            try:
                status_code, data = fetch_device_data(session)
                logger.info("API request successful: HTTP %s", status_code)
                display_data(status_code, data)

            except requests.exceptions.HTTPError as exc:
                status_code = exc.response.status_code if exc.response is not None else "unknown"
                logger.error("HTTP request failed with status %s.", status_code)
                
                if status_code == 401:
                    logger.error("Authentication failed. Check your API token.")
                elif status_code == 403:
                    logger.error("Access forbidden. Check API permissions.")
                elif status_code == 404:
                    logger.error("Endpoint not found. Check the API URL.")

            except requests.exceptions.Timeout:
                logger.error("The API request timed out.")
            except requests.exceptions.ConnectionError:
                logger.error("Connection failed. Check the network and API URL.")
            except requests.exceptions.RequestException as exc:
                logger.error("HTTP request error: %s", type(exc).__name__)
            except ValueError as exc:
                logger.error("%s", exc)

            try:
                time.sleep(POLL_INTERVAL)
            except KeyboardInterrupt:
                logger.info("Stopping the application.")
                break

# ============================================================
# ENTRY POINT
# ============================================================

if __name__ == "__main__":
    try:
        main()
    except ValueError as exc:
        logger.error("%s", exc)
        raise SystemExit(1)
