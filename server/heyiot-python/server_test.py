import os
import requests

from requests.exceptions import RequestException

# À définir dans les variables d'environnement.
API_URL = "https://console.helium-iot.xyz/api/applications/4EBF1384C8A21D30DFA77718F0DCDD9F/integrations"
API_TOKEN = "cdb16764-f770-45fd-9a00-864fe466028a"

HEADERS = {
    "Authorization": f"Bearer {API_TOKEN}",
    "Accept": "application/json",
}

def tester_api():
    try:
        response = requests.get(
            API_URL,
            headers=HEADERS,
            timeout=(5, 15),
        )

        print("Code HTTP :", response.status_code)
        response.raise_for_status()

        data = response.json()
        print("Réponse JSON :", data)

        return data

    except requests.exceptions.HTTPError as exc:
        print("Erreur HTTP :", exc.response.status_code)

    except requests.exceptions.Timeout:
        print("Le serveur n'a pas répondu dans le délai prévu.")

    except requests.exceptions.JSONDecodeError:
        print("La réponse ne contient pas de JSON valide.")

    except RequestException as exc:
        print("Erreur de communication :", exc)

    return None


if __name__ == "__main__":
    tester_api()
