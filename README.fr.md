# Yocto2 — H3x4l0g Firmware

*English version: [README.md](README.md).*

La suite du firmware du **Yocto v2 d'e-licktronic**, le clone de TR-808 (ATmega1284P).
Il part de `Yocto_v2_003beta`, la dernière version qu'e-licktronic a publiée dans
[e-licktronic/Yocto-V2.0](https://github.com/e-licktronic/Yocto-V2.0), avant le retrait
du produit en 2017 et l'abandon du projet.

C'est un projet indépendant et non officiel, sans lien avec e-licktronic. Le code
d'origine, les schémas et les manuels sont les leurs. Le dépôt d'origine ne porte aucune
licence, et celui-ci n'en ajoute pas.

## Ce qui change

Tout ce qui change le comportement de la machine est dans [CHANGELOG.md](CHANGELOG.md)
(anglais : [CHANGELOG.en.md](CHANGELOG.en.md)). En bref :

- **Corrections** : l'encodeur avance d'un pas par cran, au lieu de quatre. Une édition
  ne fuit plus dans un champ qu'on ne regarde pas. Les gels à l'allumage et les blocages
  du bus I2C sont réglés, et une sauvegarde ratée le dit à l'écran. Le placement des
  trigs et de l'horloge est plus serré, et la synchro esclave ne double plus l'horloge
  sur le MIDI OUT.
- **Nouveau, réglable pas par pas** : le micro-timing (0 à 23 ticks de retard), le retrig
  (2, 3, 4, 6 ou 8 coups dans le pas) et les conditions de trig (jouer à la Xᵉ mesure sur
  Y, ou avec une probabilité).
- **Le format de la mémoire des patterns ne change pas.** Vos patterns existants
  continuent de marcher.

**Mode d'emploi des nouvelles fonctions : [docs/new-features.md](docs/new-features.md)**
(en anglais). Les écrans de la machine, eux, sont étiquetés en français.

## Mise à jour par MIDI, sans programmateur

Tous les Yocto v2 ont le bootloader SysEx d'e-licktronic dans leur puce. Il suffit d'une
interface MIDI.

1. Téléchargez le `.syx` dans les [Releases](../../releases). Leurs notes donnent le
   nombre de pages.
2. Allumez le Yocto **en tenant les touches de pas 1, 3 et 5**. Toutes les LED clignotent
   deux fois : le bootloader attend.
3. Envoyez le fichier sur le MIDI IN du Yocto, **avec une pause d'au moins 50 ms après
   chaque message SysEx** (après chaque `F7`). Voir plus bas pourquoi c'est important.
4. Les LED de pas comptent en binaire les pages acceptées. Quand le compte atteint le
   nombre de pages des notes de la Release, la machine redémarre et l'écran d'accueil
   affiche la nouvelle version.

**Si le compte s'arrête ailleurs, renvoyez le fichier.** Une page refusée est ignorée sans
aucun signal, et toutes les pages suivantes tombent alors une page trop bas. Le
bootloader n'écrit jamais sur lui-même, donc un transfert raté peut toujours se refaire.
Si ça échoue encore, soupçonnez d'abord l'interface MIDI : les petits câbles USB-MIDI
premier prix sont connus pour perdre des octets en SysEx.

### Pour envoyer le fichier

- **Windows** : `python tools/send_syx.py` liste les sorties MIDI. Lancez ensuite
  `python tools/send_syx.py yocto2.syx "<un bout du nom du port>"`. Il envoie un message
  à la fois et attend ce que le bootloader demande. Il n'utilise que la bibliothèque
  standard de Python. MIDI-OX essaie de mettre les 88 Ko du fichier en tampon d'avance,
  et nous n'avons pas réussi à le faire marcher.
- **macOS** : SysEx Librarian, avec un délai d'au moins 50 ms entre les messages.

### Pourquoi la pause

Le bootloader écrit chaque page de 256 octets interruptions coupées, pendant environ
9 ms. L'UART ne garde que trois octets pendant ce temps, donc un message qui arrive à ce
moment-là est perdu.

### Le protocole

Il n'avait jamais été documenté : il a été relu dans le bootloader lui-même. Chaque page
est un message :

```
F0 7D 08 08 02  7E 00  <256 octets en 512 quartets, poids fort d'abord>  <somme modulo 256, 2 quartets>  F7
```

Les pages ne portent pas d'adresse : elles s'écrivent dans l'ordre à partir de
l'adresse 0. Le message `F0 7D 08 08 02 7F 00 F7` lance l'application.
`tools/hex2syx.py` fabrique le fichier. Avant de l'écrire, le script le rejoue dans une
transcription de l'automate du bootloader.

## Gels à l'allumage : les fusibles

Beaucoup de machines sont sorties d'usine avec des fusibles qui rendent le démarrage
aléatoire :

| Fusible | Valeur d'usine | Effet |
| --- | --- | --- |
| lfuse | `0xAC` | Quartz en mode basse consommation, délai de démarrage court, et une horloge à 16 MHz qui sort sur le canal B de l'encodeur |
| efuse | `0xFF` | Détection de sous-tension désactivée |

La correction demande un programmateur ISP. N'écrivez que ces deux octets :

```
avrdude -c <programmateur> -p m1284p -U lfuse:w:0xF7:m -U efuse:w:0xFC:m
```

Les valeurs corrigées donnent un quartz en pleine excursion avec le délai de démarrage le
plus long, et une détection de sous-tension à 4,3 V. **Ne touchez pas au hfuse
(`0xDC`)**, et **n'utilisez jamais « Graver la séquence d'initialisation » de l'IDE
Arduino**. Elle réécrit les trois fusibles et efface le bootloader SysEx, qui n'existe
nulle part ailleurs.

## Flasher par ISP

Un téléversement ISP efface toute la puce, bootloader compris. Donc :

1. Sauvegardez d'abord votre propre puce : `avrdude ... -U flash:r:yocto-backup.hex:i`.
2. Fusionnez l'application avec le bootloader relu dans cette sauvegarde :
   `python tools/merge_bootloader.py yocto2.ino.hex yocto-backup.hex yocto2-with-bootloader.hex`.
3. Flashez le fichier fusionné. Rien n'est flashé tant qu'avrdude n'a pas affiché
   `bytes of flash verified`.

## Compiler

- Arduino avec **MightyCore 3.1.0**, carte `MightyCore:avr:1284:pinout=standard,variant=modelP,clock=16MHz_external,LTO=Os`.
- Bibliothèques : MIDI Library 5.0.2, LiquidCrystal, et **WireN**, fournie dans
  [libraries/WireN](libraries/WireN). WireN est le `Wire` du core avec `BUFFER_LENGTH` et
  `TWI_BUFFER_LENGTH` portés à 132 : une page EEPROM de 128 octets plus ses 2 octets
  d'adresse. Elle est sous LGPL 2.1, comme l'originale.
- La compilation ne doit produire aucun warning sur les fichiers de ce dépôt.

```
arduino-cli compile --warnings more --libraries libraries --fqbn "MightyCore:avr:1284:pinout=standard,variant=modelP,clock=16MHz_external,LTO=Os" .
python tools/hex2syx.py build/yocto2.ino.hex yocto2.syx
```

## Crédits

- Conception et firmware d'origine : **e-licktronic**.
- Plusieurs corrections ont été recoupées avec le firmware NAVA maintenu par **zabox**, et
  avec le [firmware NAVA de MSS](https://github.com/Modularsoundsystems/mss-nava-firmware).
  Les idées ont été réécrites ici, pas copiées.
