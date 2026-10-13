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

# Load environment variables from the .env file.
load_dotenv()

API_URL = os.getenv("HEYIOT_API_URL", "").strip()
API_TOKEN = os.getenv("HEYIOT_API_TOKEN", "").strip()
DEVICE_ID = os.getenv("HEYIOT_DEVICE_ID", "").strip()

POLL_INTERVAL = int(os.getenv("POLL_INTERVAL", "10"))

# HTTP request timeout in seconds.
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
        raise ValueError(
            "HEYIOT_API_URL is missing from the .env file."
        )

    parsed_url = urlparse(API_URL)

    if parsed_url.scheme != "https" or not parsed_url.netloc:
        raise ValueError(
            "HEYIOT_API_URL must be a valid HTTPS URL."
        )

    if POLL_INTERVAL < 1:
        raise ValueError(
            "POLL_INTERVAL must be at least 1 second."
        )

    logger.info("Configuration loaded successfully.")

    if DEVICE_ID:
        logger.info("Device ID configured.")

    if API_TOKEN:
        logger.info("API token configured.")
    else:
        logger.info(
            "No API token configured; checking whether "
            "the endpoint permits unauthenticated access."
        )


# ============================================================
# API REQUEST
# ============================================================

def fetch_device_data(session):
    """Request data from the configured HeyIoT API endpoint."""

    headers = {
        "Accept": "application/json",
        "User-Agent": "HeyIoT-Python-Client/1.0",
    }

    # Use Bearer authentication when an API token is provided.
    if API_TOKEN:
        headers["Authorization"] = f"Bearer {API_TOKEN}"

    # Add the device ID as a query parameter if provided.
    # Change this parameter if the API documentation specifies
    # a different name or requires the ID in the URL path.
    params = {}

    if DEVICE_ID:
        params["device_id"] = DEVICE_ID

    logger.info("Sending an HTTPS request to the API.")

    response = session.get(
        API_URL,
        headers=headers,
        params=params,
        timeout=REQUEST_TIMEOUT,
    )

    # Raise an exception for HTTP errors such as 401, 403 or 404.
    response.raise_for_status()

    # Parse the response as JSON.
    try:
        data = response.json()
    except requests.exceptions.JSONDecodeError as exc:
        raise ValueError(
            "The API response is not valid JSON."
        ) from exc

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

    print(
        json.dumps(
            data,
            indent=4,
            ensure_ascii=False,
        )
    )

    print("=" * 60)


# ============================================================
# MAIN LOOP
# ============================================================

def main():
    """Run the API connectivity test."""

    validate_configuration()

    logger.info("Starting the HeyIoT API connectivity test.")
    logger.info(
        "Polling interval: %s seconds",
        POLL_INTERVAL,
    )

    # Reuse the HTTP session between requests.
    with requests.Session() as session:

        while True:
            try:
                status_code, data = fetch_device_data(session)

                logger.info(
                    "API request successful: HTTP %s",
                    status_code,
                )

                display_data(status_code, data)

            except requests.exceptions.HTTPError as exc:
                status_code = (
                    exc.response.status_code
                    if exc.response is not None
                    else "unknown"
                )

                logger.error(
                    "HTTP request failed with status %s.",
                    status_code,
                )

                # Do not print request headers or credentials.
                if status_code == 401:
                    logger.error(
                        "Authentication failed. Check the API token."
                    )

                elif status_code == 403:
                    logger.error(
                        "Access forbidden. Check API permissions."
                    )

                elif status_code == 404:
                    logger.error(
                        "Endpoint not found. Check the API URL."
                    )

            except requests.exceptions.Timeout:
                logger.error(
                    "The API request timed out."
                )

            except requests.exceptions.ConnectionError:
                logger.error(
                    "Connection failed. Check the network and API URL."
                )

            except requests.exceptions.RequestException as exc:
                logger.error(
                    "HTTP request error: %s",
                    type(exc).__name__,
                )

            except ValueError as exc:
                logger.error("%s", exc)

            except KeyboardInterrupt:
                logger.info("Stopping the application.")
                break

            # Wait before requesting the next update.
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
