# QATRA

## Smart Solar Irrigation & Crop Intelligence

### Projet développé par l'équipe Solitech --- Hackathon KIZUNA Japon--Tunisie 2026

------------------------------------------------------------------------

## 1. Présentation du projet

**Qatra**, développé par l'équipe **Solitech**, est une solution
d'agriculture intelligente qui combine l'Internet des objets (IoT),
l'automatisation et l'énergie solaire afin d'optimiser la gestion de
l'eau dans les exploitations agricoles. Dans ses évolutions futures, le
projet pourra également intégrer l'intelligence artificielle pour
analyser les données et proposer des recommandations.

Le système collecte des informations sur les conditions agricoles grâce
à plusieurs capteurs : humidité du sol, température et humidité de
l'air, niveau d'eau du réservoir et débit d'irrigation. À partir de ces
données, il peut commander automatiquement une pompe afin d'irriguer les
cultures lorsque cela est nécessaire.

Qatra vise à proposer une solution adaptable aux réalités agricoles de
la Tunisie et du Japon. Son ambition est de contribuer à une agriculture
plus économe en ressources, plus autonome et plus facile à surveiller.

**Notre objectif : apporter la bonne quantité d'eau au bon moment, tout
en maîtrisant l'énergie nécessaire à l'irrigation.**

## 2. Problématique

L'agriculture dépend fortement de la disponibilité de l'eau et de la
capacité à gérer efficacement les ressources. Pourtant, dans certaines
exploitations, l'irrigation est encore déclenchée manuellement ou selon
des horaires fixes, sans tenir suffisamment compte de l'humidité réelle
du sol et des conditions climatiques.

Cette situation peut entraîner plusieurs difficultés :

-   Une consommation excessive d'eau lorsque l'irrigation continue alors
    que le sol est déjà suffisamment humide.
-   Un apport insuffisant en eau lorsque les besoins des cultures ne
    sont pas détectés à temps.
-   Des dépenses énergétiques importantes liées au fonctionnement des
    pompes.
-   Un manque de visibilité sur le niveau d'eau disponible et le volume
    réellement utilisé.
-   Une détection tardive des fuites, des blocages ou des
    dysfonctionnements.
-   Une dépendance à la présence humaine pour surveiller et gérer
    l'irrigation.

**Problématique centrale :** comment concevoir un système d'irrigation
intelligent, autonome et accessible, capable d'optimiser la consommation
d'eau, de mieux gérer l'énergie et de faciliter la surveillance des
exploitations agricoles en Tunisie et au Japon ?

## 3. Solutions existantes et limites

Plusieurs technologies permettent déjà d'améliorer la gestion de
l'irrigation. Elles répondent à une partie du problème, mais leurs
fonctionnalités et leurs coûts varient selon les installations.

  -----------------------------------------------------------------------
  Solution existante      Fonctionnement          Limites possibles
  ----------------------- ----------------------- -----------------------
  Irrigation manuelle     L'agriculteur déclenche Demande une présence
                          et arrête l'arrosage    régulière et peut
                                                  provoquer un arrosage
                                                  excessif ou insuffisant

  Irrigation              Distribue l'eau         Ne détermine pas
  goutte-à-goutte         directement au niveau   nécessairement le
                          des plantes             moment optimal
                                                  d'irrigation

  Programmateur           Déclenche l'irrigation  Les horaires ne
  d'arrosage              selon des horaires      reflètent pas toujours
                          définis                 les besoins réels des
                                                  cultures

  Système basé sur des    Mesure certains         Les capacités de
  capteurs                paramètres, comme       supervision et
                          l'humidité du sol       d'analyse dépendent du
                                                  système installé

  Pompage solaire         Utilise l'énergie       Ne garantit pas, à lui
                          solaire pour faire      seul, une irrigation
                          fonctionner une pompe   adaptée aux besoins des
                                                  plantes

  Plateforme agricole     Permet de consulter des Peut nécessiter une
  connectée               données à distance      connexion, une
                                                  infrastructure et un
                                                  investissement
                                                  supplémentaires
  -----------------------------------------------------------------------

Qatra propose de réunir plusieurs fonctions dans une même plateforme :
surveillance des conditions agricoles, automatisation de l'irrigation,
mesure de la consommation d'eau, sécurisation de la pompe, alimentation
solaire et supervision connectée.

L'objectif n'est pas de prétendre remplacer toutes les solutions
existantes, mais de proposer une architecture modulaire, évolutive et
adaptable aux besoins de différentes exploitations.

## 4. Notre solution proposée : Qatra

Qatra est organisé autour de quatre fonctions complémentaires.

### 4.1. Surveillance des conditions agricoles

Des capteurs installés dans la zone de culture collectent les
informations nécessaires au suivi de l'exploitation.

  -----------------------------------------------------------------------
  Composant                           Rôle dans Qatra
  ----------------------------------- -----------------------------------
  Capteur capacitif d'humidité du sol Mesurer l'humidité du sol afin
                                      d'identifier les périodes où une
                                      irrigation peut être nécessaire

  Capteur SHT21                       Mesurer la température et
                                      l'humidité de l'air

  Débitmètre YF-S201                  Mesurer le débit et estimer le
                                      volume d'eau distribué

  Capteur de niveau d'eau             Surveiller la quantité d'eau
                                      disponible dans le réservoir

  Capteur LDR --- extension possible  Mesurer la luminosité ambiante afin
                                      d'enrichir les données de suivi
  -----------------------------------------------------------------------

### 4.2. Irrigation automatique et sécurisée

Lorsque le capteur détecte que le sol est trop sec, le système vérifie
d'abord que le réservoir contient suffisamment d'eau. Si les conditions
de sécurité sont respectées, le contrôleur active la pompe.

Pendant l'irrigation, le débitmètre mesure le passage de l'eau. La pompe
est arrêtée lorsque le niveau d'humidité souhaité est atteint ou
lorsqu'une condition de sécurité est détectée.

Le système prévoit notamment : - L'interdiction de démarrer la pompe
lorsque le réservoir est vide ou trop bas. - L'arrêt de la pompe lorsque
l'humidité cible est atteinte. - La détection d'un débit nul lorsque la
pompe fonctionne. - La détection de débits anormaux, selon des seuils
définis après calibration. - Une durée maximale de fonctionnement. - Un
arrêt sécurisé en cas de perte de communication entre les unités.

### 4.3. Alimentation par énergie solaire

Qatra prévoit une alimentation reposant sur un panneau solaire, un
circuit de recharge, une batterie et une régulation de tension adaptée
aux composants électroniques.

Cette architecture vise à réduire la dépendance au réseau électrique et
à favoriser l'autonomie énergétique de l'installation. Le
dimensionnement du panneau, de la batterie et des convertisseurs devra
tenir compte de la consommation réelle du système. La pompe peut
demander beaucoup plus de puissance que les capteurs et les cartes de
commande ; son alimentation doit donc être dimensionnée en conséquence.

### 4.4. Analyse des données et intelligence artificielle

Dans sa première version, Qatra s'appuie sur des règles de décision et
des seuils configurables pour automatiser l'irrigation. Cette approche
permet d'obtenir un fonctionnement local simple et vérifiable.

Une évolution intégrant l'intelligence artificielle pourra exploiter
l'historique des mesures afin de : - Analyser les habitudes de
consommation d'eau. - Identifier les périodes où les besoins en
irrigation sont élevés. - Détecter des comportements inhabituels dans le
débit ou le fonctionnement de la pompe. - Croiser les données du sol
avec les conditions climatiques. - Proposer des recommandations adaptées
aux conditions observées.

L'IA sera développée à partir de données collectées et évaluées. Elle ne
doit pas être présentée comme déjà opérationnelle si aucun modèle n'a
encore été entraîné et testé.

## 5. Pertinence du projet pour la Tunisie

La Tunisie est confrontée à une pression importante sur ses ressources
en eau, accentuée par les périodes de sécheresse et l'irrégularité des
précipitations. L'agriculture a donc besoin de solutions permettant de
mieux contrôler l'irrigation et de limiter les pertes.

Qatra peut être appliqué à différents types de cultures, notamment les
oliveraies, les cultures maraîchères et les serres agricoles.

**Exemple d'utilisation :** dans une oliveraie ou une parcelle
maraîchère, les capteurs mesurent l'humidité du sol. Lorsque celui-ci
devient trop sec, Qatra vérifie le niveau d'eau du réservoir puis
déclenche la pompe si les conditions sont réunies. Le débitmètre
enregistre la quantité d'eau distribuée. L'agriculteur peut ensuite
consulter les données et suivre l'irrigation de son exploitation.

**Impact recherché en Tunisie :** contribuer à une utilisation plus
raisonnée de l'eau, améliorer le suivi de l'irrigation et limiter les
dépenses liées au pompage.

## 6. Pertinence du projet pour le Japon

Le Japon fait face à des défis agricoles liés notamment au
vieillissement de la population agricole et à la diminution de la
main-d'œuvre disponible. La surveillance des parcelles et la réalisation
de tâches répétitives représentent donc des enjeux importants.

Qatra peut contribuer à automatiser certaines opérations et à faciliter
le suivi des exploitations à distance.

**Exemple d'utilisation :** dans une serre ou une exploitation
maraîchère, les capteurs transmettent régulièrement les informations sur
le sol et l'environnement. L'irrigation peut être déclenchée
automatiquement selon les conditions mesurées. Depuis un bureau,
l'agriculteur peut consulter les données, vérifier l'état de la pompe et
recevoir des alertes en cas d'anomalie.

Pour une application dans les rizières, une adaptation sera nécessaire :
il faudra ajouter un capteur mesurant directement le niveau d'eau dans
la parcelle et prévoir une électrovanne adaptée. Le flotteur utilisé
dans la version actuelle surveille le réservoir ; il ne mesure pas le
niveau d'eau de la rizière.

**Impact recherché au Japon :** réduire certaines interventions
manuelles, faciliter la surveillance des cultures et accompagner
l'automatisation des exploitations.

## 7. Une solution commune, adaptée à deux contextes

La Tunisie et le Japon ne rencontrent pas exactement les mêmes
difficultés agricoles. Qatra propose donc une base technologique
commune, configurable selon les besoins de chaque pays.

-   **En Tunisie :** mettre l'accent sur la maîtrise de l'eau, le suivi
    de la consommation et l'optimisation de l'énergie utilisée pour le
    pompage.
-   **Au Japon :** mettre l'accent sur la surveillance à distance,
    l'automatisation et la réduction des tâches répétitives.
-   **Dans les deux pays :** favoriser une agriculture plus efficace,
    durable et mieux informée grâce aux données.

Cette approche s'inscrit dans l'esprit de KIZUNA : créer un lien entre
les compétences et les besoins des deux pays afin de développer des
solutions utiles, concrètes et adaptées à leurs réalités.

## 8. Architecture technique actuelle et évolution IoT

### 8.1. Prototype actuel avec deux Arduino Uno

Le prototype réalisé dans Proteus repose sur deux cartes Arduino Uno.

**Arduino Uno 1 --- Acquisition des données**

Cette carte collecte les mesures des capteurs : - SHT21 : température et
humidité de l'air. - Capteur capacitif : humidité du sol. - Capteur de
niveau : disponibilité de l'eau dans le réservoir. - YF-S201 : débit
d'eau.

Elle transmet les mesures à la deuxième carte par une liaison série
UART.

**Arduino Uno 2 --- Traitement et commande**

Cette carte reçoit les mesures et gère : - La logique d'irrigation. - La
commande de la pompe à travers un circuit de puissance adapté. -
L'affichage des informations sur l'écran LCD. - Les alertes par buzzer
et LEDs. - L'enregistrement des données sur carte SD, si cette fonction
est intégrée et testée.

### 8.2. Évolution vers une architecture MQTT avec deux ESP32

Une évolution possible consiste à utiliser deux ESP32.

**ESP32 1 --- Unité installée dans la serre ou la parcelle**

Il collecte les mesures des capteurs et assure la commande locale de
l'irrigation. Les fonctions essentielles de sécurité doivent continuer à
fonctionner même en l'absence d'Internet.

**ESP32 2 --- Unité installée au bureau**

Il reçoit les données publiées par l'unité de terrain via MQTT et permet
de les afficher sur un écran ou de les transmettre à un tableau de bord.

MQTT permet d'échanger des messages légers entre les équipements
connectés. Des sujets peuvent être organisés comme suit :

  Sujet MQTT            Informations transmises
  --------------------- ---------------------------------
  `qatra/soil`          Humidité du sol
  `qatra/temperature`   Température de l'air
  `qatra/humidity`      Humidité de l'air
  `qatra/water/level`   Niveau du réservoir
  `qatra/water/flow`    Débit et volume d'eau
  `qatra/pump/status`   État de la pompe
  `qatra/energy`        Informations sur l'alimentation

Cette architecture est une perspective d'évolution. Elle ne doit pas
être confondue avec le prototype actuel à deux Arduino Uno tant que la
migration vers les ESP32 n'a pas été réalisée.

## 9. Tableau de bord et supervision à distance

Une plateforme Web ou mobile pourra présenter les principales
informations de l'exploitation : - Humidité du sol. - Température et
humidité de l'air. - Niveau d'eau du réservoir. - Débit instantané et
volume d'eau consommé. - État de la pompe. - État de l'alimentation et
de la batterie. - Historique des mesures. - Alertes et recommandations.

L'agriculteur pourra ainsi surveiller l'installation sans devoir se
déplacer constamment jusqu'à la zone de culture.

## 10. Perspectives d'évolution

Plusieurs améliorations peuvent être envisagées après la validation du
prototype initial.

1.  **Écran OLED local :** afficher les mesures et l'état du système
    directement dans la serre.
2.  **Communication MQTT :** connecter plusieurs unités de terrain à une
    supervision centralisée.
3.  **Suivi énergétique :** ajouter des capteurs de tension et de
    courant pour mesurer la production solaire et la consommation.
4.  **Station météo :** intégrer un capteur de pluie, un LDR et
    éventuellement d'autres capteurs météorologiques.
5.  **Analyse par IA :** exploiter les données historiques pour proposer
    des recommandations d'irrigation.
6.  **Irrigation multi-zones :** contrôler indépendamment plusieurs
    parcelles grâce à des capteurs et des électrovannes.
7.  **Adaptation aux rizières :** ajouter une mesure directe du niveau
    d'eau dans la parcelle et une commande adaptée des vannes.

## 11. Résultats attendus et indicateurs d'évaluation

Les performances de Qatra devront être évaluées à partir de mesures
réalisées sur le prototype et, si possible, comparées à une méthode
d'irrigation de référence.

  -----------------------------------------------------------------------
  Indicateur                          Objectif de l'évaluation
  ----------------------------------- -----------------------------------
  Volume d'eau utilisé                Mesurer la quantité d'eau
                                      distribuée

  Durée de fonctionnement de la pompe Suivre le temps consacré à
                                      l'irrigation

  Consommation énergétique            Évaluer l'énergie nécessaire au
                                      système

  Autonomie sur batterie              Mesurer la durée de fonctionnement
                                      sans apport solaire

  Détection des anomalies             Vérifier la réaction face à un
                                      débit nul, une fuite ou un
                                      réservoir bas

  Temps de surveillance               Évaluer la réduction des
                                      interventions manuelles

  Fiabilité de la communication       Vérifier la transmission et la
                                      réception des mesures
  -----------------------------------------------------------------------

Les économies d'eau, d'énergie et de temps ne doivent pas être annoncées
comme des résultats acquis avant les essais. Elles constituent des
objectifs à mesurer.

## 12. Plan de réalisation pour le hackathon

Pour assurer une démonstration fiable, le développement peut être
organisé en trois étapes.

### Étape 1 --- Prototype fonctionnel

-   Lecture des capteurs.
-   Communication entre les deux Arduino.
-   Affichage LCD.
-   Commande automatique de la pompe.
-   Alertes par LEDs et buzzer.
-   Sécurités contre le réservoir vide et les anomalies de débit.

### Étape 2 --- Démonstration connectée

-   Intégration d'un ESP32.
-   Transmission des mesures via MQTT.
-   Création d'un tableau de bord.
-   Affichage des alertes à distance.
-   Démonstration de l'alimentation solaire.

### Étape 3 --- Évolutions avancées

-   Deuxième ESP32 au bureau.
-   Analyse des historiques par IA.
-   Recommandations d'irrigation.
-   Gestion de plusieurs zones.
-   Adaptation aux besoins de cultures spécifiques en Tunisie et au
    Japon.

## 13. Pitch de présentation

### Qatra --- Une agriculture intelligente, un lien entre la Tunisie et le Japon

Aujourd'hui, l'agriculture doit faire face à des défis majeurs : la
gestion de ressources en eau limitées, les besoins énergétiques de
l'irrigation et la nécessité de faciliter le travail des agriculteurs.

En Tunisie, la maîtrise de l'eau est essentielle pour préserver les
ressources et assurer la continuité de la production agricole. Au Japon,
le vieillissement de la population agricole renforce le besoin
d'automatiser certaines tâches et de faciliter la surveillance des
exploitations.

Face à ces enjeux, **l'équipe Solitech propose Qatra**, une solution
d'irrigation intelligente qui combine l'IoT, l'automatisation et
l'énergie solaire, avec une évolution possible vers l'intelligence
artificielle.

Grâce à plusieurs capteurs, Qatra surveille l'humidité du sol, les
conditions de l'air, le niveau d'eau et le débit d'irrigation. Le
système peut alors commander automatiquement la pompe, mesurer l'eau
distribuée et détecter certaines anomalies.

La connexion à distance, notamment grâce à MQTT dans une future
évolution, permettra à l'agriculteur de surveiller son exploitation
depuis un bureau ou un smartphone.

Notre approche s'adapte aux deux contextes : mieux gérer l'eau et
l'énergie en Tunisie, et faciliter la surveillance et l'automatisation
agricoles au Japon.

**Avec Qatra, Solitech souhaite contribuer à une agriculture plus
intelligente, plus autonome et plus durable.**

## 14. Message clé

**Qatra, développé par Solitech, transforme les données agricoles en
décisions d'irrigation plus adaptées. En associant gestion intelligente
de l'eau, énergie solaire et technologies connectées, le projet propose
une réponse évolutive à des défis agricoles partagés par la Tunisie et
le Japon.**
