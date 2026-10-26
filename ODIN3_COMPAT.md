# Compatibilité Odin 3 : Dolphin (GameCube/Wii)

Notre propre liste de compatibilité, tenue d'après nos tests sur l'AYN Odin 3 (Snapdragon 8 Elite, Adreno 830) avec notre build de la branche `odin3`. Chaque test met à jour la ligne du jeu : statut, mesure, date et commit testé.

Autres émulateurs : [Eden (Switch)](https://github.com/HamzaDevz/eden/blob/odin3/ODIN3_COMPAT.md) · [Azahar (3DS)](https://github.com/HamzaDevz/azahar/blob/odin3/ODIN3_COMPAT.md) · [Dolphin (GC/Wii)](https://github.com/HamzaDevz/dolphin/blob/odin3/ODIN3_COMPAT.md) · [Cemu (Wii U)](https://github.com/HamzaDevz/Cemu/blob/odin3/ODIN3_COMPAT.md) · [Vita3K (Vita)](https://github.com/HamzaDevz/Vita3K/blob/odin3/ODIN3_COMPAT.md) · [aPS3e (PS3)](https://github.com/HamzaDevz/aps3e/blob/odin3/ODIN3_COMPAT.md) · [Bachata S4 (PS4)](https://github.com/HamzaDevz/bachata-s4/blob/odin3/ODIN3_COMPAT.md) · [xemu (Xbox)](https://github.com/HamzaDevz/xemu/blob/odin3/ODIN3_COMPAT.md)

## Échelle

| Statut | Sens |
|---|---|
| 🟢 Parfait | vu en partie, plein régime, aucun défaut vu |
| 🔵 Jouable | vu en partie, défauts mineurs |
| 🟡 Problèmes | vu en partie, défauts gênants (fps, graphismes, son) |
| 🟠 Menus/intro | démarre, pas encore vu en partie |
| 🔴 Ne marche pas | crash, écran noir ou blocage |
| ⚪ Non testé | présent sur la console, jamais lancé |

## Comment on teste

Paquet `org.dolphinemu.dolphinemu.odin3`, installé à côté de l'officielle `org.dolphinemu.dolphinemu`. Banc adb dans `Tools/odin3/` : installation (`install.sh`), verrou partagé de la console (`lock.sh`), manettes DS4 virtuelles par uinput (`vpad.c`) et captures d'écran. `measure.sh` et `summarize.py` existent mais n'ont pas encore servi : aucune mesure de fps pour l'instant.

## Jeux

| Jeu | Statut | Vu jusqu'à | Mesure | Notes | Testé le | Build |
|---|---|---|---|---|---|---|
| Mario Kart Wii (USA) `RMCE01` | 🟠 Menus/intro | titre, choix du permis (sauvegarde « Hamz' » de l'officielle) | pas mesuré | lancé par ES-DE ; écran noir avant a4f6d1bb5c (blocage des joueurs auto au démarrage Wii) ; joueurs façon Switch : deux faux pads Xbox prennent J1 et J2, l'Odin J3, que le jeu voit comme Wiimote 3 (« La manette a été changée », témoin 3) ; les pads partis, l'Odin reprend J1 et J2/J3 sont libérés ; J1 joue à la manette GC, comme le veut le réglage du jeu (`GameSettings/RMCE01.ini` : Wiimote 1 coupée) ; aucune course faite | 2026-10-26 | a4f6d1bb5c |
| Mario Party 4 (Europe) `GMPP01` | 🟠 Menus/intro | logos, intro, choix du fichier | pas mesuré | joueurs façon Switch : l'Odin seule est J1 au démarrage ; un faux pad du dongle Xbox qui appuie prend J1, puis la copie DS4 J2 et l'Odin J3 ; le pad parti, l'Odin reprend J1 en 0,5 s (a87fb0d184) ; lancé par ES-DE (`%ROMPROVIDER%`, c86cd51301) ; adaptateur GameCube pas encore essayé (pas branché) | 2026-10-26 | c86cd51301 (2609-140) |
| Super Smash Bros. Brawl (Europe) `RSBP01` | 🔴 Ne marche pas | écran noir au lancement par ES-DE | – | fichier, pas émulateur : le .rvz de la carte est tronqué (3 001 360 000 octets, l'en-tête en annonce 6 197 466 188) et Dolphin le refuse (« File size is incorrect ») ; le .zip voisin contient le .rvz entier ; un échec de démarrage laisse un écran noir au lieu d'un message | 2026-10-26 | 0c243caa5a |
| Super Mario Sunshine (Europe) `GMSP01` | 🟠 Menus/intro | intro, titre, choix du fichier, nouvelle partie jusqu'à la cinématique d'ouverture | 30 images/s (le jeu tourne à 30), 25 en 50 Hz ; temps émulé collé à l'horloge sur 80 s : plein régime | aucun défaut vu ; Vulkan, 2x, attente des shaders au lancement ; deux runs identiques au pixel jusqu'au titre, mêmes images que le banc VM d'odin3-lab | 2026-10-21 | 41878ef489 (2609-90, pas encore sur `odin3`) |

Les autres jeux GameCube/Wii de la carte n'ont pas encore été testés sur notre build (les temps de jeu de `TimePlayed.ini` viennent de parties faites sur l'officielle).
