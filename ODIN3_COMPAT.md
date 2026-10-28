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

Paquet `org.dolphinemu.dolphinemu.odin3`, installé à côté de l'officielle `org.dolphinemu.dolphinemu`. Banc adb dans `Tools/odin3/` : installation (`install.sh`), verrou partagé de la console (`lock.sh`), manettes DS4 virtuelles par uinput (`vpad.c`) et captures d'écran. `measure.sh` compte les images que SurfaceFlinger reçoit. Le relevé du 28/10 a lancé chaque jeu une minute, sans manette, copié dans le dossier de l'appli sur la carte (notre build n'a pas l'accès à tout le stockage), avec le journal de Dolphin en avertissements (`Config/Logger.ini`, gardé depuis). Les jeux PAL tournent à 50 fps tant qu'on n'a pas répondu Oui à leur question du mode 60 Hz.

## Jeux

| Jeu | Statut | Vu jusqu'à | Mesure | Notes | Testé le | Build |
|---|---|---|---|---|---|---|
| 007: Nightfire (France) `GO7F69` | 🟠 Menus/intro | écran titre | 50 fps (jeu PAL 50 Hz) | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Baten Kaitos (France), DVD 1 `GKBPAF` | 🟠 Menus/intro | intro (île volante) | 44,8 fps à l'intro, 22 au pire sur 1 s | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Beyond Good & Evil `GGEY41` | 🟠 Menus/intro | choix de la langue | 50 fps | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Bloody Roar: Primal Fury (Europe) `GBLP52` | 🟠 Menus/intro | intro | 58 fps, 39 au pire sur 1 s | un grand rectangle blanc sur la capture de l'intro, à vérifier en jeu | 2026-10-28 | 2f5e540648 |
| Dragon Ball Z: Budokai 2 (Europe) `GZ3PB2` | 🟠 Menus/intro | choix de la langue | 50 fps | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Eternal Darkness (Europe) `GEDP01` | 🟠 Menus/intro | logo Dolby de l'intro | 52,5 fps, pause de 3 s au chargement | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Fire Emblem: Path of Radiance (Europe) `GFEP01` | 🟠 Menus/intro | avertissement santé | 59,9 fps | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Mario Kart Wii (USA) `RMCE01` | 🟠 Menus/intro | titre, choix du permis (sauvegarde « Hamz' » de l'officielle) | 58,3 fps à l'intro le 28/10, 27 au pire sur 1 s | lancé par ES-DE ; écran noir avant a4f6d1bb5c (blocage des joueurs auto au démarrage Wii) ; joueurs façon Switch : deux faux pads Xbox prennent J1 et J2, l'Odin J3, que le jeu voit comme Wiimote 3 (« La manette a été changée », témoin 3) ; les pads partis, l'Odin reprend J1 et J2/J3 sont libérés ; J1 joue à la manette GC, comme le veut le réglage du jeu (`GameSettings/RMCE01.ini` : Wiimote 1 coupée) ; aucune course faite | 2026-10-26 | a4f6d1bb5c |
| Mario Party 4 (Europe) `GMPP01` | 🟠 Menus/intro | logos, intro, choix du fichier | pas mesuré | joueurs façon Switch : l'Odin seule est J1 au démarrage ; un faux pad du dongle Xbox qui appuie prend J1, puis la copie DS4 J2 et l'Odin J3 ; le pad parti, l'Odin reprend J1 en 0,5 s (a87fb0d184) ; lancé par ES-DE (`%ROMPROVIDER%`, c86cd51301) ; adaptateur GameCube pas encore essayé (pas branché) | 2026-10-26 | c86cd51301 (2609-140) |
| Mario Party 5 (Europe) `GP5P01` | 🟠 Menus/intro | écran titre | 50 fps | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Mario Party 6 (Europe) `GP6P01` | 🟠 Menus/intro | avertissement santé | 50 fps | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Mario Party 7 (Europe) `GP7P01` | 🟠 Menus/intro | avertissement santé | 50 fps | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Mario Smash Football (Europe) `G4QP01` | 🟠 Menus/intro | avertissement santé | 59,6 fps | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Metroid Prime (Europe) `GM8P01` | 🟠 Menus/intro | écran titre | 59,7 fps | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Ōkami (Europe) `ROWP08` | 🟠 Menus/intro | intro | 30 fps, avec PAL60 (EuRGB60) coupé comme activé | le SYSCONF Wii du fork, repris de l'officielle, a PAL60 coupé ; l'activer ne change rien à l'intro, laissé coupé | 2026-10-28 | 2f5e540648 |
| Paper Mario: La Porte Millénaire (Europe) `G8MP01` | 🟠 Menus/intro | avertissement santé | 55,9 fps, pause de 2 s au chargement | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Resident Evil (Europe) `GBIP08` | 🔴 Ne marche pas | alerte au démarrage | – | « GFX FIFO: Unknown Opcode 0xfe » 0,2 s après le lancement, à chaque essai (2 sur 2) : bogue connu de Dolphin, ouvert depuis 2020 (https://bugs.dolphin-emu.org/issues/12321) | 2026-10-28 | 2f5e540648 |
| Resident Evil Zero (Europe), disque 1 `GBZP08` | 🟠 Menus/intro | question du mode 60 Hz | 50 fps avant la réponse | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Resident Evil: Code: Veronica X (Europe) `GCDP08` | 🟠 Menus/intro | choix de la langue | écran fixe, pas de mesure utile | aucune erreur au journal ; game1.iso est le même jeu | 2026-10-28 | 2f5e540648 |
| Skies of Arcadia: Legends (Europe) `GEAP8P` | 🟠 Menus/intro | intro | 24,6 fps à l'intro | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Sonic Mega Collection (Europe) `GSOP8P` | 🟠 Menus/intro | question du mode 60 Hz | 50 fps avant la réponse | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| SoulCalibur II (France) `GRSPAF` | 🟠 Menus/intro | message de la carte mémoire | 59,9 fps | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Star Fox Adventures (Europe) v1.01 `GSAP01` | 🟠 Menus/intro | démo de l'intro | 51,6 fps, pause de 1,7 s au chargement | « GFX FIFO: Unknown Opcode 0xdb » une fois sur deux pendant la démo (désynchro GPU du double cœur) | 2026-10-28 | 2f5e540648 |
| Super Mario Sunshine (Europe) `GMSP01` | 🟠 Menus/intro | intro, titre, choix du fichier, nouvelle partie jusqu'à la cinématique d'ouverture | 30 images/s (le jeu tourne à 30), 25 en 50 Hz ; temps émulé collé à l'horloge sur 80 s : plein régime | aucun défaut vu ; Vulkan, 2x, attente des shaders au lancement ; deux runs identiques au pixel jusqu'au titre, mêmes images que le banc VM d'odin3-lab | 2026-10-21 | 41878ef489 (2609-90, pas encore sur `odin3`) |
| Super Smash Bros. Brawl (Europe) `RSBP01` | 🔴 Ne marche pas | écran noir au lancement par ES-DE | – | fichier, pas émulateur : le .rvz de la carte est tronqué (3 001 360 000 octets, l'en-tête en annonce 6 197 466 188) et Dolphin le refuse (« File size is incorrect ») ; le .zip voisin contient le .rvz entier ; un échec de démarrage laisse un écran noir au lieu d'un message | 2026-10-26 | 0c243caa5a |
| Tales of Symphonia (France), DVD 1 `GQSFAF` | 🟠 Menus/intro | intro | 47,5 fps, pause de 1,6 s au chargement | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| The Legend of Zelda: Four Swords Adventures (Europe) `G4SP01` | 🟠 Menus/intro | écran titre | 59,7 fps | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| The Legend of Zelda: The Wind Waker (Europe) `GZLP01` | 🟠 Menus/intro | écran titre | 34,7 fps à l'écran titre (le jeu tourne à 30) | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Viewtiful Joe (Europe) `GVJP08` | 🟠 Menus/intro | intro | 33,5 fps à l'intro | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Wario World (Europe) `GWWP01` | 🔴 Ne marche pas | alerte au démarrage | – | fichier, pas émulateur : warioworld.iso fait 275 Mo au lieu de 1,46 Go, « The disc could not be read » | 2026-10-28 | 2f5e540648 |
| WarioWare: Smooth Moves (Europe) `RODP01` | 🟠 Menus/intro | intro | 60 fps | écritures NAND qui bloquent l'émulation 2 à 3 ms au démarrage | 2026-10-28 | 2f5e540648 |
| Wiimms MKW Fun 2011-06 `RMCE09` | 🟠 Menus/intro | intro | pas mesuré (calque vide) | écritures NAND qui bloquent l'émulation 3 ms | 2026-10-28 | 2f5e540648 |
| XIII (Europe) `GX3X41` | 🟠 Menus/intro | choix de la langue | 25 fps | aucune erreur au journal | 2026-10-28 | 2f5e540648 |
| Zack & Wiki: Le Trésor de Barbaros (Europe) `RTZP08` | 🟠 Menus/intro | écran titre | 36 fps à l'intro, pause de 0,9 s | PAL60 activé ou non : même mesure | 2026-10-28 | 2f5e540648 |

Pas encore lancés sur notre build : les WAD (Castlevania ReBirth, Chronos Twins DX, Dr. Mario), la .rvz de Star Fox Adventures et game1.iso (doublons).
