# Vérifications sur la carte

Liste à suivre le soir, avec le Mac et le CoreInk. Chaque vérification dit
quoi faire, ce qu'on attend, et ce qu'il faut noter. Les réponses vont dans la
colonne « Résultat », puis dans la pull request ou le fil de discussion :
c'est ce qui permet de corriger les pilotes le lendemain.

Les pilotes ont été écrits sans la carte, d'après [coreink.md](coreink.md) :
il faut s'attendre à des écarts. Un écart n'est pas un échec, c'est
l'information qui manquait.

Durée estimée : 1 h 30 pour la soirée 1.

## Soirée 1

### A. Chaîne de développement (étape 1 du plan)

| # | Faire | Attendu | Résultat |
|---|---|---|---|
| A1 | Installer ESP-IDF v5.4.1 : `mkdir -p ~/esp && cd ~/esp && git clone -b v5.4.1 --recursive https://github.com/espressif/esp-idf.git && cd esp-idf && ./install.sh esp32` | Installation sans erreur. | |
| A2 | `. ~/esp/esp-idf/export.sh`, puis `file $(which xtensa-esp32-elf-gcc)` | `Mach-O 64-bit executable arm64` (GEN-07). | |
| A3 | `file $(which python3)` et `esptool.py version` | Python `arm64` ; esptool répond. | |
| A4 | Dans le dépôt : `cmake -S test/host -B test/host/build && cmake --build test/host/build && ctest --test-dir test/host/build` | Tous les tests passent sur le Mac. | |
| A5 | Brancher le CoreInk en USB-C. `ls /dev/cu.*` | Un port `/dev/cu.usbserial-…` (puce CP2104). Sinon, installer le pilote Silicon Labs CP210x et noter sa version. | |

### B. Diagnostic des pilotes

```
cd test/board/diagnostic
idf.py set-target esp32
idf.py -p /dev/cu.usbserial-XXXX build flash monitor
```

Pour quitter le moniteur : `Ctrl-]`. L'application avance d'une étape à
chaque appui sur la molette ; les messages commencent par `diag:`.

| # | Étape | Attendu | À noter | Résultat |
|---|---|---|---|---|
| B1 | Flashage | Le flashage réussit, le moniteur affiche `==== diagnostic Retimer ====`. | Si le flashage échoue : maintenir le bouton marche pendant le flashage. | |
| B2 | 1. alimentation | `batterie : …` entre 3300 et 4200 mV (sur USB, plutôt 4100 à 4300). | Les trois valeurs. | |
| B3 | 2. écran, complet | « TOTAL 12:34 » en haut, « RAPPEL 05:00 » en bas, sans négatif. | Image correcte, en négatif (alors la polarité est inversée), ou rien (BUSY, ou autre écran que le GDEW0154M09). Durée du rafraîchissement. | |
| B4 | 2. écran, partiels | Le décompte 00:20 → 00:01 s'affiche, une mise à jour par seconde. | Durée affichée de chaque partielle (ms). Traces visibles ? Clignotement noir à chaque mise à jour ? | |
| B5 | 2. écran, alerte | Écran inversé : fond noir, « 00:00 » et « FIN » en blanc. | Lisible à 2 m ? Traces de l'image précédente ? | |
| B6 | 3. buzzer | Cinq bips de 1000 à 4000 Hz, puis la sonnerie de rappel (3 bips) et de fin (2 × 3 bips). | La fréquence la plus forte. Audible depuis une autre pièce ? | |
| B7 | 4. LED | La LED verte s'allume pendant l'une des deux phases. | « allumée à 0 » ou « allumée à 1 », et où elle se trouve sur le boîtier. | |
| B8 | 5. boutons | Chaque appui affiche `GPIO n (…) : 0`, puis `: 1` au relâchement. | **Quel GPIO pour chaque commande**, en particulier le bouton sous la molette (attendu : GPIO 5) et le bouton marche (attendu : GPIO 27). Rebonds (plusieurs lignes pour un appui) ? | |
| B9 | 6. veille légère, sans toucher | Réveil après environ 10 000 ms, cause 4. | Durée mesurée. | |
| B10 | 6. veille légère, avec un bouton | Relancer (Reset), refaire jusqu'à l'étape 6, appuyer sur le bouton sous la molette pendant la veille : réveil immédiat, cause 7. | Le bouton sous la molette réveille-t-il ? Et la molette ? | |
| B11 | 7. veille profonde, sur USB | Après 30 s, `réveil de veille profonde n° 1 après ~30000 ms, bouton 0`. | Durée. | |
| B12 | 7. veille profonde, molette haut puis bas | Réveil immédiat, `bouton 1` (haut) ou `bouton 4` (bas). | Le réveil marche-t-il ? L'image reste-t-elle à l'écran pendant la veille ? | |
| B13 | 7. veille profonde, **sur batterie** | Débrancher l'USB pendant la veille, attendre 30 s, rebrancher et relancer le moniteur sans Reset (`idf.py -p … monitor`). | La carte s'est-elle éteinte (GPIO 12 mal maintenu) ou réveillée ? Le compteur de veilles a-t-il augmenté ? | |

### C. Firmware Retimer

```
cd ../../..        (racine du dépôt)
idf.py set-target esp32
idf.py -p /dev/cu.usbserial-XXXX build flash monitor
```

| # | Faire | Attendu | Résultat |
|---|---|---|---|
| C1 | Allumer. | Écran « RETIMER EN VEILLE », puis veille profonde (`veille profonde` dans le journal). | |
| C2 | Molette haut, un cran. | Réveil, « TOTAL 00:05 » qui clignote (DEM-02), puis départ du décompte après 3 s (DEM-04). | |
| C3 | Maintenir la molette haut 5 s. | La valeur monte de plus en plus vite (REG-03). Noter si le rythme convient. | |
| C4 | Bouton sous la molette, plusieurs appuis. | Le rappel passe 1, 2, 3, 5 min… puis « ARRÊT ». | |
| C5 | Régler 2 min et un rappel de 1 min, attendre. | À 1 min : sonnerie de rappel et zone du bas inversée (ECH-01). Appui sur la molette : la zone redevient normale (ACQ-04). | |
| C6 | Laisser finir. | À la fin : sonnerie de fin, écran entier inversé, « 00:00 » et « FIN » (ECH-02, ETA-07). Appui sur la molette : retour à la veille (ETA-19). | |
| C7 | Pendant un décompte, molette bas jusqu'à 0. | Arrêt et retour à la veille (ACQ-01). | |
| C8 | Décompte de 10 min chronométré au téléphone. | Écart à la fin, en secondes (dérive de l'horloge, coreink.md section 4). | |
| C9 | Pendant un décompte, regarder l'écran en affichage réduit (« 09:-- »). | Une mise à jour par minute, puis chaque seconde sur la dernière minute (MAT-04). | |

### D. Pour la suite

- Laisser la carte en veille, sur batterie, toute la nuit. Le lendemain :
  allume-t-elle encore ? Tension de la batterie au réveil (journal) ?
- Noter toute remarque sur la lisibilité, le bruit, la prise en main.

## Ce que les résultats changent

| Si… | Alors… |
|---|---|
| B3 : image en négatif | Inverser la polarité dans `eink_display.c` (une ligne). |
| B3 : rien ne s'affiche | Vérifier le journal (`BUSY toujours actif`) ; essayer l'autre écran (GDEY0154D67). |
| B4 : partielles trop lentes ou traces | Ajuster `CST_DISPLAY_PARTIALS_BEFORE_FULL`, et l'affichage réduit. |
| B7 : allumée à 1 | Décocher `RETIMER_LED_ACTIVE_LOW` dans `idf.py menuconfig`. |
| B8 : autre GPIO | Corriger `input_coreink.c` et `coreink.md`. |
| B10 : le bouton du rappel ne réveille pas | Revoir la stratégie de veille (coreink.md, section 4). |
| B13 : extinction en veille profonde | Revoir le maintien du GPIO 12. |
| C8 : dérive de plus de 1 s sur 10 min | Recaler l'heure sur l'horloge BM8563. |
