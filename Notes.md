# Projet

## Configuration du projet

1. Ajouter l'URL du gestionnaire de cartes RAKwireless
  1. Installer ou/et ouvrir Arduino IDE.
  2. Aller dans **Fichier > Préférences**.
  3. Dans le champ "URL de gestionnaire de cartes supplémentaires", coller le lien `https://raw.githubusercontent.com/RAKWireless/RAKwireless-Arduino-BSP-Index/main/package_rakwireless_index.json` et cliquer sur **OK**.
2. Installer le support de la carte
  1. Aller dans **Outils > Type de carte > Gestionnaire de cartes**.
  2. Dans la barre de recherche, taper `RAKwireless`.
  3. Installer le paquet "RAKwireless nRF Boards".
3. Sélectionner la carte
  1. Aller dans Outils > Type de carte et sélectionner **WisBlock RAK4631**.
4. Installer les bibliothèques nécessaires
  1. Aller dans **Croquis > Inclure une bibliothèque > Gérer les bibliothèques**.
  2. Chercher et installer la bibliothèque pour le LoRaWAN : **SX126x-Arduino**.
  3. Chercher et installer la bibliothèque pour ton capteur de température. Si tu utilises le module standard RAK1901, cherche **SparkFun SHTC3 Humidity and Temperature Sensor**.