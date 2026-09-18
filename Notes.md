# Projet

## Configuration du projet

1. Ajouter l'URL du gestionnaire de cartes RAKwireless

   1. Installer et/ou ouvrir **Arduino IDE**.
   2. Aller dans **Fichier > Préférences**.
   3. Dans le champ **URL de gestionnaire de cartes supplémentaires**, coller le lien `https://raw.githubusercontent.com/RAKWireless/RAKwireless-Arduino-BSP-Index/main/package_rakwireless_index.json` et cliquer sur **OK**.
2. Installer le support de la carte

   1. Aller dans **Outils > Type de carte > Gestionnaire de cartes**.
   2. Dans la barre de recherche, taper `RAKwireless`.
   3. Installer le paquet **RAKwireless nRF Boards**.
3. Sélectionner la carte

   1. Aller **dans Outils > Type de carte** et sélectionner **WisBlock RAK4631**.
4. Installer les bibliothèques nécessaires

   1. Aller dans **Croquis > Inclure une bibliothèque > Gérer les bibliothèques**.
   2. Chercher et installer la bibliothèque pour le LoRaWAN : **SX126x-Arduino**.
   3. Chercher et installer la bibliothèque pour ton capteur de température. Si tu utilises le module standard RAK1901, cherche **SparkFun SHTC3 Humidity and Temperature Sensor**.

## Configuration de HeyloT Console

1. Se connecter

   1. Aller sur le site [HeyloT Console](https://console.helium-iot.xyz).
2. Créer un Device Profile

   1. Aller dans **Device Profiles**.
   2. Créer un nouveau *device profile* en cliquant sur **Add device profile**.
   3. Valider tous les paramètres nécessaires. S'assurer que la région affiche bien `EU868`.
3. Créer une Application

   1. Aller dans **Applications**.
   2. Créer une nouvelle *application* en cliquant sur **Add application**.
   3. Une fois fait, cliquer sur le nom de l'application.
4. Créer un Device

   1. Créer un nouveau *device* en cliquant sur **Add device**.
   2. Lors de la création, donner un nom, générez un **Device EUI** et un **Join EUI**. Mettre le *device Profile* créé précédemment.
   3. Une fois fait, cliquer dessus et allez sur **OTAA keys**.
   4. Générer un **Application key**.
5. Intégrer les clés dans le code

   1. Retourner sur le code d'Arduino.
   2. Dans les variables `nodeDeviceEUI`, `nodeAppEUI` et `nodeAppKey`, remplacer les valeurs respectives par les clés générées sur le site.