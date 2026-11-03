# Yocto v2.074 — les nouveautés depuis la 2.003beta

*English version: [new-features.md](new-features.md).*

Ce document complète le *Yocto v2.002 user manual* d'origine, il ne le remplace pas :
tout ce que décrit le manuel d'origine marche toujours comme il est écrit, sauf là où ce
document dit autre chose.

La mise à jour ne touche pas à vos patterns ni à vos pistes. Les nouveaux réglages par
pas utilisent une place qui existait déjà dans chaque pattern sans jamais servir : les
patterns existants jouent exactement comme avant.

---

## 1. L'écran de réglage du pas

Trois nouvelles fonctions se règlent **par pas et par instrument** : les conditions de
trig, le retrig et le micro-timing. Elles partagent un écran, disponible en
**PATT EDIT**.

**Pour l'ouvrir** : tenez **SHIFT** et appuyez sur une **touche de pas**.

| Geste | Effet |
| --- | --- |
| SHIFT + pas | ouvre la page 1 de ce pas |
| SHIFT + le même pas, une deuxième fois | passe à la page 2 |
| SHIFT + le même pas, une troisième fois | ferme l'écran |
| SHIFT + un autre pas | saute sur ce pas, en page 1 |

Pas de touche de sortie à apprendre : on sort par où l'on est entré. Le geste est ignoré
quand LAST STEP, INST ou BANK est tenu, et en mode clavier, parce que SHIFT + pas y a
déjà un sens.

**Les deux pages** :

```
pas cnd prb ins          pas rtg utm ins
5   1:4  50  SD          5   x3  .   SD
```

| Étiquette | Sens |
| --- | --- |
| `pas` | le pas (1 à 16) |
| `cnd` | la condition de trig |
| `prb` | la probabilité |
| `rtg` | le retrig |
| `utm` | le micro-timing |
| `ins` | l'instrument |

Le pas et l'instrument restent aux mêmes colonnes sur les deux pages.

**Se déplacer** : **appuyez sur l'encodeur** pour passer au champ suivant, puis
**tournez-le** pour changer la valeur. Le curseur arrive sur la condition (ou sur le
retrig en page 2), pas sur le pas qu'on vient de désigner. On peut changer `pas` et `ins`
sans quitter l'écran. `ins` saute l'emplacement de l'accent global, qui n'a pas de pas à
régler.

Tant que l'écran est ouvert, les seize LED de pas montrent quels pas de l'instrument
portent un réglage, avec la tête de lecture.

**Sauvegarder** : comme pour toute édition de pattern, ENTER clignote et ENTER
sauvegarde.

**Effacer** : effacer un pas efface aussi sa condition, son retrig et son micro-timing.
CLEAR les efface tous. Décaler un pattern à gauche ou à droite emmène les réglages avec
leurs pas.

---

## 2. Les conditions de trig — `cnd` et `prb`

Un pas peut ne jouer que certaines mesures, ou seulement de temps en temps.

**Condition cyclique** (`cnd`) : `X:Y` veut dire *jouer à la mesure X de chaque groupe
de Y mesures*.

| Valeur | Joue |
| --- | --- |
| `---` | à chaque mesure (par défaut) |
| `1:2` · `2:2` | la première ou la deuxième de chaque groupe de 2 |
| `1:3` · `2:3` · `3:3` | la première, la deuxième ou la troisième de chaque groupe de 3 |
| `1:4` … `4:4` | la mesure 1, 2, 3 ou 4 de chaque groupe de 4 |
| `1:8` · `5:8` | la première ou la cinquième de chaque groupe de 8 |

**Probabilité** (`prb`) : de 6 % à 93 % par seizièmes, puis 100 %, qui veut dire
toujours. La molette se lit dans le sens de la densité : plus on monte, plus la note tombe
souvent.

**Les deux se combinent.** `1:4` à `50`, c'est une note *candidate* une mesure sur quatre,
et qui joue une fois sur deux parmi celles-là.

Un pas qu'une condition fait taire reste sur la grille. La piste EXT garde sa pile de
notes en phase avec la grille, exactement comme sous un mute.

---

## 3. Le retrig — `rtg`

Un pas peut sonner **2, 3, 4, 6 ou 8 fois** dans sa propre durée. Pour les roulements de
charley, les buzz de caisse claire, les bégaiements de grosse caisse. `x3` et `x6` donnent
des triolets.

| Valeur | Coups par pas |
| --- | --- |
| `x1` | 1 (normal) |
| `x2` `x3` `x4` `x6` `x8` | autant, régulièrement espacés |

- Toutes les répétitions sonnent **au même niveau** que le coup principal.
- Le pas suivant interrompt le roulement.
- Une condition ou un mute gouverne le pas entier, répétitions comprises.
- En page 2, les LED de pas montrent les pas qui roulent.
- Le placement ne dépend pas du nombre de voix qui roulent en même temps.

---

## 4. Le micro-timing — `utm`

Chaque pas peut être **retardé de 0 à 23 ticks**. À 120 BPM, un tick vaut environ 5 ms.
De quoi pousser une voix derrière le temps sans changer la grille.

- `.` veut dire *sur la grille* (aucun retard).
- **Pour jouer un coup *avant* un pas**, écrivez-le sur le pas précédent et retardez-le
  presque à fond. C'est ainsi qu'on place un clap juste avant la caisse claire.
- **Le shuffle mange une partie de la course.** Un pas swingué est déjà en retard, donc
  la place qui reste avant le coup suivant est plus courte. Le réglage s'y arrête au lieu
  de faire disparaître le coup.
- Un roulement (retrig) suit son pas : tout le groupe est retardé.
- La piste EXT n'est pas concernée.

---

## 5. Mute et solo

En mode **MUTE**, désormais annoncé en grand à l'écran :

| Geste | Effet |
| --- | --- |
| pad | mute ou démute la voix, comme avant |
| **SHIFT + pad** | met la voix en **solo**, ou l'en retire |
| appui sur l'encodeur | efface tous les mutes **et** tous les solos |

- **Plusieurs voix peuvent être en solo ensemble.** Retirer la dernière fait sortir du
  solo.
- **Le solo est un calque, pas une réécriture.** En sortant du solo, on retrouve les
  mutes posés avant.
- Pendant un solo, un pad seul ne fait rien : on ne peut pas modifier un mute qu'on ne
  voit pas.
- **L'accent global, TR1 et TR2 ne sont jamais coupés par un solo.** L'accent n'est pas
  un son mais un niveau ajouté à une voix. TR1 et TR2 cadencent du matériel extérieur :
  mettre la grosse caisse en solo ne doit pas arrêter le séquenceur qu'elle pilote. La
  piste EXT fait du son, donc un solo la coupe comme les autres voix.

---

## 6. Tempo et shuffle

- **TEMPO tenu + molette** règle le tempo depuis n'importe quel écran d'édition, et
  l'affiche pendant qu'on tourne. La molette seule ne règle le tempo qu'en **PATT PLAY**,
  le seul écran qui l'affiche.
- **SHUFFLE tenu** règle le shuffle de **l'instrument courant**. Le titre le nomme :
  `Shuffle  SD`.
- **SHIFT + SHUFFLE tenus** règlent **les seize** d'un coup : `Shuffle  GLOBAL`. Le
  bargraphe montre la valeur qu'on règle.
- Le shuffle va de 50 % à 70,8 %. Chaque cran vaut le même pourcentage à toutes les
  résolutions, et **le cran 9 est un shuffle triolet exact**.
- **Changer la scale d'un instrument en marche le remet en phase** sur la mesure. SHIFT
  remet les seize en phase.
- Les 221 valeurs de BPM sont exactes.

---

## 7. Banques et patterns

- **BANK tenu** allume les **huit banques** en veilleuse, la courante à pleine intensité.
  Seules les touches 1 à 8 sont des banques.
- **Choisir une banque ne change plus ce qui joue.** BANK + un pas *arme* la banque, et
  la touche de pas suivante choisit le pattern dedans. L'écran ne change de lettre que
  quand le pattern change vraiment.
- La lettre de banque, la LED de banque et les touches de pas sont toujours d'accord avec
  le pattern qui joue.
- Le **changement de direction** marche aussi en PATT PLAY, comme le décrit le manuel
  d'origine, et il est sauvegardé avec le pattern.

---

## 8. Les petits changements qu'on remarque

- **L'encodeur avance d'un pas par cran** (il en comptait quatre), et il ne change que ce
  que l'écran affiche.
- La machine démarre en **PATT PLAY**.
- **Passer de TRACK à PATT demande d'arrêter le transport.** Passer de la lecture à
  l'édition dans une même famille reste libre.
- **En mode TAP, les touches 13 à 16** jouent les quatre pistes qui n'ont pas de voix :
  accent global, instrument externe, TRIG-1 et TR2. Elles frappaient toutes la grosse
  caisse.
- **La piste EXT a des accents.** Ses pas passent par éteint / normal / accent comme les
  douze voix, et le note-on MIDI porte la vélocité du pas.
- La LED NUM s'allume quand on tient NUM.
- **En synchro esclave, le tempo du maître s'affiche**, moyenné sur quatre temps. Les
  trois pavés pleins ne restent que tant qu'aucune horloge n'arrive.
- **Quitter le mode config sans ENTER** affiche `[NOT SAVED!]` : les réglages sont actifs
  mais ne survivront pas à l'extinction.
- **Une sauvegarde ratée le dit** : `[SAVE FAILED n]` remplace `[SAVED!]`.

  | n | Sens |
  | --- | --- |
  | 2 | la mémoire ne répond pas |
  | 3 | la mémoire refuse un octet |
  | 4, 5 | défaut du bus I2C |
  | 6 | l'écriture ne se termine jamais |

Toutes les corrections sont dans [CHANGELOG.md](../CHANGELOG.md) (anglais :
[CHANGELOG.en.md](../CHANGELOG.en.md)).
