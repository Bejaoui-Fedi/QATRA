# QATRA — Simulation d'irrigation intelligente

**QATRA** est un prototype d'irrigation intelligente développé par l'équipe **Solitech** dans le cadre du hackathon **KIZUNA Japon–Tunisie 2026**. Ce dépôt présente principalement la **simulation Proteus** et son fonctionnement embarqué actuel : deux Arduino Uno échangent les mesures des capteurs pour surveiller une parcelle et commander l'irrigation.

> La connexion MQTT avec des ESP32, le tableau de bord et l'analyse par IA sont des pistes d'évolution. Ils ne font pas partie du prototype Arduino décrit ici.

## La simulation en un coup d'œil

Le montage réunit une unité d'acquisition, une unité de traitement et de commande, ainsi qu'une liaison série UART à **9 600 bauds**.

<p align="center">
  <img src="UART%20%E2%80%93%209600%20bauds.PNG" alt="Vue générale de la simulation Proteus QATRA" width="900">
</p>
<p align="center"><em>Vue d'ensemble : acquisition, liaison UART et commande de l'irrigation.</em></p>

## Fonctionnement du prototype

### 1. Acquisition des mesures — Arduino Uno 1

La première carte relève les paramètres simulés de la parcelle et du circuit hydraulique :

- humidité du sol ;
- température et humidité de l'air avec le capteur SHT21 ;
- niveau d'eau du réservoir ;
- débit d'eau avec le débitmètre YF-S201.

Les mesures sont envoyées périodiquement à la seconde carte au format série `T:...,H:...,S:...,W:...,F:...`.

### 2. Décision et commande — Arduino Uno 2

La seconde carte reçoit et interprète les mesures. Elle les affiche sur l'écran LCD, signale les états et alertes à l'aide de LEDs et d'un buzzer, et commande la pompe via un étage transistor-relais. Le programme peut aussi enregistrer les mesures sur carte SD.

La pompe ne démarre que si des mesures valides sont disponibles, que la liaison fonctionne et que le réservoir n'est pas sous le seuil critique. Elle s'arrête lorsque l'humidité cible est atteinte, si une condition de sécurité apparaît ou si la durée maximale de fonctionnement est dépassée. Les seuils sont définis dans le programme de l'Arduino 2.

<table>
  <tr>
    <td align="center" width="50%">
      <img src="Surveillance%20agricole.PNG" alt="Capteurs et unité d'acquisition de la simulation" width="420">
      <br><strong>Arduino Uno 1 — Acquisition des capteurs</strong>
    </td>
    <td align="center" width="50%">
      <img src="Gestion%20intelligente.PNG" alt="Unité de contrôle, affichage et commande de la pompe" width="420">
      <br><strong>Arduino Uno 2 — Traitement et commande</strong>
    </td>
  </tr>
</table>

### 3. Alimentation — schéma complémentaire

Ce schéma illustre un bloc d'alimentation étudié séparément, avec panneau solaire, batterie et régulation. Il complète la réflexion sur l'autonomie énergétique ; il ne représente pas à lui seul l'ensemble du circuit d'irrigation.

<p align="center">
  <img src="bloc%20Alimenation.PNG" alt="Schéma complémentaire d'alimentation solaire et régulée" width="720">
</p>
<p align="center"><em>Étude séparée du bloc d'alimentation solaire et de la régulation.</em></p>

## Contenu du dépôt

| Fichier | Description |
| --- | --- |
| `Qatra.pdsprj` | Projet de simulation Proteus |
| `ARD1/ARD1.ino` | Acquisition des capteurs et émission série |
| `ARD2/ARD2.ino` | Réception, logique d'irrigation, alertes et affichage |
| `UART – 9600 bauds.PNG` | Vue d'ensemble de la simulation et de la liaison UART |
| `Surveillance agricole.PNG` | Capteurs et Arduino d'acquisition |
| `Gestion intelligente.PNG` | Arduino de commande et périphériques |
| `bloc Alimenation.PNG` | Schéma complémentaire du bloc d'alimentation |
| `Qatra_Solitech_Presentation.md` | Présentation détaillée du contexte et des perspectives |

## Ouvrir et examiner la simulation

1. Ouvrir `Qatra.pdsprj` dans Proteus.
2. Consulter `ARD1/ARD1.ino` et `ARD2/ARD2.ino` pour suivre respectivement l'acquisition et la commande.
3. Lancer la simulation et observer les mesures, l'affichage, les indicateurs d'alerte et la commande de la pompe.
4. Modifier les entrées des capteurs dans Proteus pour examiner les réactions du système aux variations d'humidité, de niveau d'eau et de débit.

Les bibliothèques Arduino utilisées comprennent `Wire`, `SPI`, `SD` et `LiquidCrystal_I2C`.

## Limites et évolutions

Le comportement dépend des seuils et des modèles de capteurs configurés pour la simulation. Les économies d'eau ou d'énergie restent des objectifs à mesurer lors d'essais ; elles ne sont pas présentées ici comme des résultats validés.

Les suites envisagées comprennent une migration vers des ESP32 et MQTT, une supervision à distance, un suivi énergétique et une analyse des historiques. Pour le contexte complet du projet, voir [Qatra_Solitech_Presentation.md](Qatra_Solitech_Presentation.md).


## Suite IoT — ESP32 et MQTT

La branche [`solution-iot`](https://github.com/Bejaoui-Fedi/QATRA/tree/solution-iot) présente la suite du projet, qui relie deux ESP32 au moyen du protocole MQTT pour transmettre les mesures, superviser le système et commander la pompe. Elle comprend son propre README illustré décrivant l'architecture, les sujets MQTT, les protections et la configuration.


## Naviguer dans les branches du projet

Chaque branche rassemble une version ou un volet différent de QATRA. Ouvrez la branche pour parcourir ses fichiers ou son README pour accéder à sa présentation.

| Branche | Contenu | Documentation |
| --- | --- | --- |
| [`main`](https://github.com/Bejaoui-Fedi/QATRA/tree/main) | Simulation Proteus et prototype embarqué Arduino Uno | [README principal](https://github.com/Bejaoui-Fedi/QATRA/blob/main/README.md) |
| [`solution-iot`](https://github.com/Bejaoui-Fedi/QATRA/tree/solution-iot) | Suite connectée avec deux ESP32 et MQTT | [README Solution IoT](https://github.com/Bejaoui-Fedi/QATRA/blob/solution-iot/README.md) |
| [`prototype`](https://github.com/Bejaoui-Fedi/QATRA/tree/prototype) | Essais matériels LoRa et mesure ultrasonique du niveau d'eau | [README Prototype](https://github.com/Bejaoui-Fedi/QATRA/blob/prototype/Prototype/README.md) |
