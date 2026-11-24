# Retimer : comparaison du matériel de la minuterie dédiée

Recherche du 22 novembre 2026. Prix TTC livrés en Belgique, estimés quand marqués « ~ ». Les points non vérifiés sont signalés. La recherche web a atteint sa limite de requêtes : plusieurs prix et stocks restent à confirmer au panier.

## Critères

**Obligatoires (éliminatoires)** : O1 veille profonde avec réveil par minuterie matérielle et par une action ; O2 compilateur, flasheur et débogueur natifs sur Mac Apple Silicon ; O3 bouton d'acquittement et moyen de régler les deux minuteries (encodeurs, boutons +/-, tactile…) ; O4 écran affichant les deux temps, capable de clignoter ; O5 bips ; O6 deux alertes visuelles distinguables ; O7 alimentation autonome ; O8 commandable avec livraison en Belgique par une entreprise fiable (en stock ou avec une date de réassort annoncée) ; O9 moins de 200 € par exemplaire complet.

**Comparaison** : C0 saisie ; C1 piles du commerce ; C2 consommation en veille ; C3 écran ; C4 acquittement sans les mains (gestes, NFC, tape, tactile) ; C5 langage et débogage ; C6 assemblage ; C7 boîtier ; C8 coût (idéal < 100 €) ; C9 pérennité ; C10 volume sonore ; C11 délai de livraison.

## Constats transversaux

- **Rosetta disparaît** : macOS 27 est la dernière version à prendre pleinement en charge les applications Intel ; macOS 28 (vers septembre 2027) ne la garde que pour certains jeux. Un outil qui dépend encore de Rosetta est donc à considérer comme cassé. Sources : https://www.macrumors.com/2026/06/10/macos-golden-gate-last-to-support-intel-apps/
- **Outils natifs arm64** : ESP-IDF (compilateurs, gdb, openocd-esp32 en macos-arm64, https://raw.githubusercontent.com/espressif/esp-idf/master/tools/tools.json), esptool v5 (https://github.com/espressif/esptool/releases), outils Pico SDK en binaires universels (https://github.com/raspberrypi/pico-sdk-tools/releases), picotool Homebrew, J-Link universel, STM32CubeIDE (natif depuis février 2026) et CubeProgrammer ≥ 2.20, probe-rs, bossac Homebrew.
- **Pièges Rosetta** : l'IDE Arduino et arduino-cli utilisent encore ctags x86 pour compiler les .ino (https://forum.arduino.cc/t/apple-silicon-version-of-arduino-ide/1455631) ; cœurs AVR (avr-gcc, avrdude) ; M5Burner (Intel uniquement) ; nrfutil nrf5sdk-tools ; STM32CubeCLT ; certains paquets PlatformIO (chaîne ARM ou ESP32 x86 installée par défaut). Conclusion : développer avec les SDK natifs (ESP-IDF, Pico SDK, nRF Connect SDK/Zephyr, Rust), pas avec l'IDE Arduino.
- **Écran** : le Sharp Memory LCD est le meilleur compromis pour un décompte à la seconde (≈ 4 µA, clignotement gratuit, lisible en lumière ambiante, mais rien hors tension et pas de rétroéclairage). L'e-ink garde l'image hors tension mais supporte mal un rafraîchissement chaque seconde et un clignotement à 1 Hz : il convient avec l'affichage réduit (MAT-04) et des alertes portées par LED. Un TFT doit rester allumé (≈ 100 mA) pour montrer le décompte : autonomie en jours.
- **RP2040 et RP2350** : veille médiocre (dormant 0,76 à 3,3 mA ; P-state du RP2350 0,18 à 0,25 mA), et l'oscillateur de la minuterie dérive de 17 à 25 % : une RTC externe est indispensable. https://forums.pimoroni.com/t/rp2350-low-power-modes/27235
- **Encodeurs** : préférer des encodeurs nus sur GPIO (0 µA) ; les modules I2C (Adafruit seesaw) gardent un micro alimenté, les M5Stack Unit Encoder n'ont pas de broche d'interruption.
- **Acquittement sans les mains** : l'accéléromètre (double tape sur le boîtier, LIS3DH ≈ 2 à 10 µA, ≈ 5 €) est l'option la plus fiable et la moins chère. NFC : seul le ST25R3916 a un mode de détection de carte basse consommation (≈ 5 µA en capacitif, note AN4985) ; le PN532 et le RC522 consomment trop pour rester à l'écoute. Gestes : un capteur de distance à temps de vol (VL53L1X) permet le retour visuel progressif d'ACQ-07 ; l'APDS-9960 est sensible au soleil direct. Le tactile capacitif fonctionne mal avec des doigts mouillés ou gras.
- **Boîtiers** : boîtiers portatifs du commerce avec compartiment à piles (Hammond 1593, Takachi LC, OKW Soft-Case avec fenêtre d'affichage, IP54 à IP65, chez RS et Farnell) ; il ne reste qu'à percer. Impression 3D : Materialise (Louvain), Protolabs Network. Matière PETG ou ASA, pas PLA.
- **The Pi Hut** : vers l'UE, uniquement DHL Express (30 £), droits et frais réglés à la livraison : limite pour O8. Kiwi Electronics : port 7,95 € vers la Belgique.

## Table de comparaison (solutions qui passent O1 à O9)

| Solution | Catégorie | Prix TTC | C0 Saisie | C3 Écran | C2 Veille | C1 Piles | C4 Sans les mains | C7 Boîtier | C5 Outils | Points faibles |
|---|---|---|---|---|---|---|---|---|---|---|
| J. M5Stack PaperMono (C153) | Packagé | ~70 € (Mouser Belgique), **livraison mi-décembre 2026** (réassort annoncé le 16/12, puis le 06/01/2027) | Tactile 3,97" + boutons A et B + bouton marche | e-paper 3,97" 800×480, rafraîchissement rapide 0,07 s | Non publiée | Batterie 1150 mAh, piles impossibles | **Lecteur NFC ST25R3916** avec détection de carte basse conso (~5 µA), IMU (taper), tactile, micro | **Fourni** (101×62×8 mm) | ESP-IDF, Rust, JTAG par USB | Délai de quelques semaines ; le tactile et les boutons A/B ne réveillent pas depuis l'état le plus économe ; consommation non publiée |
| A. XIAO nRF52840 + Sharp Memory 1,3" + 2 encodeurs | À construire | ~69 € (Kiwi) | 2 encodeurs + bouton (à câbler) | Sharp 1,3" 168×144 | ≈ 10 à 15 µA au total | 2 AA en direct ou 3 AAA + LDO | NFC tag intégré (téléphone), accéléromètre ou VL53L1X en option | À faire (boîtier du commerce à percer) | Zephyr/NCS, Rust, SWD | Écran petit (le 2,7" est en rupture) ; ~15 fils à souder |
| B. XIAO ESP32-S3 + Sharp 1,3" + 2 encodeurs | À construire | ~70 € | 2 encodeurs + bouton, plaque tactile possible | Sharp 1,3" | ≈ 10 à 20 µA, réveils plus coûteux | 3 AAA + régulateur | Tactile capacitif natif ; options en ajout | À faire | ESP-IDF, Rust, JTAG par USB | Chaque réveil redémarre le micro |
| C. Raspberry Pi Pico 2 + e-paper 2,9" + RTC | À construire | ~60 € | 2 encodeurs + bouton (4 broches de réveil max.) | e-ink 2,9" 296×128 | ≈ 180 à 250 µA | 2-3 AA en direct | En ajout | À faire | Pico SDK, Rust, SWD | Veille la plus gourmande ; e-ink mal adapté à la seconde |
| D. Nucleo-L432KC + Sharp 1,3" | À construire | ~80 € | 2 encodeurs + bouton | Sharp 1,3" | ≈ 21 µA (après modification de la carte) | 2 AA en direct | En ajout | À faire | STM32 HAL, Zephyr, Rust, ST-LINK intégré | Carte encombrante ; prix et stock non vérifiés |
| E. Pimoroni Badger 2350 + buzzer Qwiic | Hybride | ~70 à 100 € (Mouser, Pimoroni) | 5 boutons (+/- ×2 et acquittement) | e-ink 2,7" 264×176 | 167 µA mesurés | LiPo fournie ; piles à vérifier | Port Qwiic ; LED en 4 zones pour les alertes | **Fourni** | MicroPython officiel ; C possible mais à porter | Sous réserve pour O5 (buzzer à ajouter) ; veille moyenne |
| F. M5Stack CoreInk | Packagé | 51 € (RS Belgique, en stock) | Molette 3 positions + bouton | e-ink 1,54" 200×200 | Non publiée à l'époque ; calculée le 24/11/2026 d'après le schéma : ~230 µA en veille profonde, dont ~170 µA pour le pont de mesure de la batterie, et quelques µA éteint (non mesuré) | LiPo 390 mAh, piles impossibles | Port Grove | **Fourni** | ESP-IDF ; pas de débogueur sans sonde | Écran petit ; buzzer faible ; produit de 2020 ; écran GDEW0154M09 arrêté (remplaçant GDEY0154D67, autres commandes) ; fonctionnement 0 à 50 °C |
| G. M5Stack PaperS3 | Packagé | ~60 à 75 € (DigiKey, à confirmer) | Tactile 4,7" + 1 bouton | e-ink 4,7" 960×540 | 9 µA éteint | LiPo 1800 mAh, piles impossibles | IMU (taper, soulever), tactile | **Fourni** | ESP-IDF, Rust, JTAG par USB | Une seule LED ; buzzer faible ; le tactile ne réveille pas depuis l'état éteint |
| H. M5Stack Dial v1.1 | Packagé | ~40 € (RS Belgique) + LiPo | Encodeur + écran tactile pressable | TFT rond 1,28" | 6 µA éteint, mais l'écran s'éteint | LiPo à ajouter, piles risquées | Lecteur RFID intégré | **Fourni** (51×51 mm) | ESP-IDF, Rust, JTAG par USB | Le décompte n'est visible qu'écran allumé (≈ 100 mA) : autonomie en jours |
| I. Soldered Inkplate 4TEMPERA | Packagé | 159,95 € avec verre (réf. 333308, 85 en stock chez Soldered) ; la version sans verre (333307) est épuisée chez Soldered | Tactile + bouton | e-paper 3,8" 600×600 avec éclairage | 18 µA | Li-ion 1200 mAh, piles impossibles | **Capteur de gestes APDS-9960 intégré**, IMU | **Fourni** (boîtier imprimé 3D, 90×83×24 mm, dans les deux versions) | ESP-IDF ; pas de débogueur USB | Cher ; fabricant européen sérieux |

## En rupture sans date annoncée

- **Adafruit MagTag** : e-ink 2,9", 4 boutons, haut-parleur, accéléromètre, 4 LED ; en rupture chez Adafruit et The Pi Hut, sans date de réassort trouvée ; le kit boîtier est en stock. À surveiller.

## Solutions écartées

- BBC micro:bit v2, Calliope mini 3 : O4 (matrice 5×5 LED).
- Arduino UNO R3, Nano, Nano Every : O2 (chaîne AVR x86) ; UNO R4 : O2 probable.
- Pimoroni Badger 2040 W : O8 (arrêté).
- LilyGO T-Embed : O8 (pas de revendeur fiable).
- Seeed XIAO nRF54L15 : O8 (rupture, date de réassort non trouvée).
- Seeed SenseCAP Indicator : O7 (USB seulement).
- Adafruit Feather nRF52840 : plus de 1 mA en veille à cause du régulateur.
- M5StickC Plus2, Core2, CoreS3, Tufty 2350 : écran TFT, même limite que le Dial, non chiffrés.

## Non évalués faute de requêtes

Seeed reTerminal E1001/E1002 (e-paper 7,5", boutons, buzzer, LED, batterie, boîtier), Adafruit PyBadge et CLUE, Watchy.

## Incertitudes principales

- Prix et stock : PaperS3, XIAO ESP32-S3, Nucleo-L432KC, écran Waveshare 2,9", Badger 2350 chez Pimoroni UE.
- Badger 2350 : broche pour le buzzer, réveil par les boutons, alimentation par piles.
- Inkplate 4TEMPERA : tactile capacitif 2 points confirmé (https://docs.soldered.com/inkplate/4tempera/hardware/touchscreen/) ; fonctionnement du tactile à travers le verre, réveil par le tactile et modèle de RTC non documentés.
- Kits équipés d'un Sharp Memory LCD : aucun trouvé qui remplisse les critères. Bangle.js 2 (montre nRF52840 à écran mémoire et tactile, Pimoroni) n'a qu'un vibreur, pas de buzzer.
- Volume sonore : aucun buzzer mesuré ; un piézo de 60 dB est insuffisant, viser 85 dB.
- Consommations des capteurs reprises de mémoire des fiches techniques, à mesurer.
- E-ink, chaîne de fabrication (24/11/2026, général, non relu dans une source) : le film électrophorétique vient d'un seul fournisseur, E Ink ; Good Display (GDEW, GDEY) assemble les dalles et modules ; les contrôleurs viennent d'autres entreprises (Fitipower, Solomon). Le Sharp Memory LCD est une autre technologie, sans film électrophorétique. Durée de vie annoncée du GDEY0154D67 : 1 000 000 de cycles ou 5 ans, avec un rafraîchissement au moins toutes les 24 h (fiche Good Display rév. 1.2) ; rien de tel trouvé pour le GDEW0154M09. Détail : `docs/coreink.md`.
