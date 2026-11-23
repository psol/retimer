#!/usr/bin/env python3
"""Liste les exigences de la partie 1 de SPEC.md qui n'ont aucun test.

Un test « couvre » une exigence quand son nom contient l'identifiant, par
exemple test_ECH_05_ETA_16_reminder_restarts_from_ack couvre ECH-05
(docs/PLAN.md, section 4.2).

Certaines exigences ne se vérifient pas sur l'hôte (matériel, principes) :
elles sont listées dans NOT_ON_HOST avec la raison, et vérifiées sur la carte ou
en essai.

    python3 tools/coverage.py          code de sortie 1 si une exigence manque
"""
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parent.parent

NOT_ON_HOST = {
    "GEN-01": "principe : tout sur l'appareil",
    "GEN-02": "principe : un seul décompte (structure de la minuterie)",
    "GEN-03": "principe : pas de modes (une fonction par commande, saisie)",
    "GEN-04": "principe, vérifié par ECH-01 et ECH-02",
    "GEN-06": "vérifié par la structure du composant constantes",
    "GEN-07": "vérifié par la CI sur macOS arm64 et sur le Mac",
    "CMD-04": "capteur « mains sales » absent de cette étape (CLAUDE.md)",
    "CMD-07": "indicateurs d'alerte : vérifiés sur la carte",
    "ACQ-06": "capteur « mains sales » absent de cette étape",
    "ACQ-07": "capteur « mains sales » absent de cette étape",
    "ACQ-08": "capteur « mains sales » absent de cette étape",
    "ACQ-09": "capteur « mains sales » absent de cette étape",
    "ERR-05": "capteur « mains sales » absent de cette étape",
    "ETA-02": "état sans transition propre, couvert par ETA-10 à ETA-13",
    "ETA-03": "couvert par DEM-01 à DEM-04",
    "ETA-04": "couvert par ECH et MOD",
    "ETA-05": "couvert par ETA-17 à ETA-21",
    "ETA-06": "couvert par ECH-04",
    "HP-01": "hors périmètre",
    "HP-02": "hors périmètre",
    "HP-03": "hors périmètre",
}

PREFIXES = "GEN|CMD|PLG|REG|DEM|ECH|MOD|ACQ|ETA|ERR|HP"

spec = (root / "SPEC.md").read_text(encoding="utf-8")
part1 = spec.split("# Partie 2")[0]
# Identifiants définis : en gras, ou en première colonne d'un tableau.
ids = set(re.findall(r"\*\*((?:%s)-\d\d)\b" % PREFIXES, part1))
ids |= set(re.findall(r"^\|\s*((?:%s)-\d\d)\s*\|" % PREFIXES, part1, re.M))

tests = ""
for f in sorted((root / "test/host").glob("test_*.c")):
    tests += f.read_text(encoding="utf-8")
covered = {m.replace("_", "-")
        for m in re.findall(r"(?<![A-Z])(?:%s)_\d\d(?!\d)" % PREFIXES, tests)}

elsewhere = {i for i in ids if i in NOT_ON_HOST}
missing = sorted(i for i in ids if i not in covered and i not in NOT_ON_HOST)
print(f"{len(ids)} exigences en partie 1 : {len(ids & covered)} testées sur "
      f"l'hôte, {len(elsewhere - covered)} vérifiées ailleurs.")
if missing:
    print("Sans test :", ", ".join(missing))
    sys.exit(1)
print("Toutes les exigences applicables ont au moins un test.")
