# Yocto v2 — changelog

De la **v2.003beta** d'e-licktronic — dernière version publiée, projet retiré du commerce
en 2017 — à la **v2.030**. Seul ce qui change le comportement de la machine est listé ;
le détail technique vit dans `TODO.md`.

## Encodeur

- Un cran de la molette avance d'un pas. Il en comptait **quatre**.
- Sur les écrans d'édition de pattern, la molette nue ne touche plus au **tempo** — qui
  n'y est même pas affiché. On ne le déplace plus en croyant régler autre chose.
- TEMPO maintenu reste le chemin vers le tempo, et en affiche la valeur pendant qu'on
  tourne, depuis n'importe quel écran d'édition.
- SHUFFLE maintenu règle le shuffle en PATT TAP aussi. Il y bougeait le tempo en silence.
- Le « Total Accent » s'appelle **Global Accent** : tous les paramètres qui portent sur
  le pattern entier sont des « global » dans cette machine.

## Modes

- Démarrage sur **PATT PLAY**, plus sur l'écran d'édition.
- Changer de famille TRACK ↔ PATT demande l'**arrêt** du transport : les deux ne jouent
  pas la même chose. Le passage conserve alors le caractère lecture/édition, au lieu de
  retomber sur l'écran d'édition quoi qu'on quitte.
- La bascule lecture ↔ édition **dans** une famille reste libre, machine en marche
  comprise : elle ne change que ce que font les touches, jamais ce qui joue.
- Quitter TRACK par la touche PATT ne jette plus les modifications de piste en silence.

## Lisibilité du panneau

- BANK maintenu allume les **huit banques** en veilleuse, la courante à pleine intensité.
- Les touches 9 à 16 ne sélectionnent plus la banque H : ce ne sont pas des banques.
- Le curseur de lecture suit la mesure, plus la longueur de l'instrument sélectionné.
- L'écran ne reste plus figé en MUTE.
- Écran d'accueil centré, avec un balayage des LED de pas.

## Touches de pas

- Une touche pouvait devenir **définitivement muette** : sortir du mode clavier en la
  maintenant la tuait jusqu'au prochain passage par ce mode. Corrigé.

## Tempo et swing

- Les 221 valeurs de BPM sont exactes. Il n'y en avait que **sept**.
- Plus de hoquet de 32 ms en tournant le tempo.
- Plage de shuffle de 50 % à **70,8 %**, contre 62,5 % au mieux avant.
- Un cran de shuffle veut dire un **pourcentage**, le même sur les quatre résolutions.
  Shuffle triolet exact au cran 9.
- Le swing reste sur les pas impairs. Il dérivait sur les pairs, et sa phase était
  arbitraire dès qu'on changeait de pattern ou de longueur.
- Plus de déphasage permanent après un changement de longueur d'instrument.

## Patterns et chaînes

- Sélectionner une chaîne en cours de lecture ne joue plus le dernier pattern d'abord.
- Basculer édition ↔ lecture ne répète ni ne saute un pattern.
- MUTE ne gèle plus l'avancement de la chaîne.
- Le pattern courant n'est plus rejoué quand il se trouve être le premier de la chaîne.
- **La banque suit le pattern.** Arriver sur D13 par une piste affichait `A61`, éclairait
  la LED de banque A, et la touche de pas 3 renvoyait en A3. L'écran, la LED et la touche
  disent maintenant la même chose.
- **Les marches au changement de pattern à haut tempo ont fondu.** Le chargement depuis
  l'EEPROM bloquait 115 ms et commençait *à* la barre : au-delà de 230 BPM les premiers
  pas du nouveau pattern se rejouaient. Le bus tourne désormais à sa vitesse nominale et
  le chargement prend 30 ms — de près de deux pas de retard à un demi.
- Une chaîne ne peut plus aller lire **hors de la zone des patterns**. Sur une chaîne dont
  la position enregistrée était incohérente, le calcul rebouclait et le séquenceur allait
  lire dans la zone des pistes : un pattern de bruit. Rien n'était détruit — c'était une
  lecture — mais ça s'entendait.

## Direction de lecture

- **Le changement de direction est accessible en PATT PLAY**, comme le manuel le décrit,
  et plus seulement en édition. Son jumeau — le décalage gauche/droite — y était déjà :
  les deux gestes sont enfin symétriques.
- **Et il se sauvegarde.** Changer la direction ne faisait pas clignoter la LED ENTER,
  maintenir ENTER n'enregistrait rien, et le prochain chargement du pattern écrasait le
  changement. La donnée partait bien en EEPROM ; c'est le drapeau qui manquait.

## Pistes

- La longueur ne se corrompt plus à 999.
- BACK et FWD avancent d'un pas, plus de deux.
- L'insertion fonctionne sur une piste vide.
- **ENTER ne tronque plus la piste.** Réécrire un pattern en position 4 d'une piste de 20
  la ramenait à 4 mesures. La longueur ne fait plus que s'étendre ; la raccourcir reste
  possible, délibérément, à la molette sur le champ `len`.
- Après un cycle d'allumage, TRACK EDIT affiche **le pattern que la piste contient**, et
  non celui chargé au démarrage. Les données n'étaient pas perdues, l'écran mentait
  jusqu'à ce qu'on bouge.
- La position se compte à partir de **1** en lecture comme en édition. La première mesure
  s'appelait 0 d'un côté et 1 de l'autre.
- **Plus de trou au bout de la piste.** Sur une piste de 8, avancer jusqu'à la position 12
  puis ENTER portait la longueur à 12 et laissait les positions 9 à 11 pleines de ce qui
  traînait en mémoire. FWD et la molette s'arrêtent maintenant à l'emplacement d'ajout,
  celui où `SHIFT+NUM` emmène déjà.
- **Vider une piste depuis la fin ne demande plus une pression sur deux.** Le curseur
  retombait après chaque suppression sur l'emplacement d'ajout, où il n'y a rien à
  supprimer ; il fallait un BACK entre chaque. Quatre pressions vident désormais une piste
  de quatre. En contrepartie, ENTER juste après une suppression **écrase** la dernière
  position au lieu d'ajouter à la suite — `SHIFT+NUM` ramène à l'emplacement d'ajout.

## Instrument externe

- La pile de notes ne se tronque plus. Revenir à la note 3 d'une pile de vingt et en
  rejouer une la coupait à 4.
- **Plus de note restée en l'air.** BACK et FWD font sonner chaque note en parcourant la
  pile ; maintenir l'un puis appuyer sur l'autre coupait la mauvaise note, et la première
  tenait indéfiniment.
- Les douze notes du haut ne repartent plus dans l'octave du bas. Au-delà d'une certaine
  hauteur, la note envoyée débordait et rebouclait ; la molette s'arrête maintenant avant.

## Synchro et sorties

- TRIG-1 : plus d'impulsions tronquées, et la note MIDI 60 pilote à nouveau la sortie.
- Horloge MIDI entrante : 2 ms d'attente active par tick supprimées, soit 48 ms par
  seconde à 120 BPM rendues à la boucle principale.
- Dérive sur quatre minutes ramenée de 0,75 s à 13 ms.
- **Un accord MIDI part d'un seul coup.** Grosse caisse, caisse claire et cymbale sur le
  même pas arrivaient en trois messages, traités à trois tours de boucle d'écart — une
  attaque molle au lieu d'un coup net. Ils partent maintenant sur le même front, comme un
  pas du séquenceur.
- **Une note MIDI ne coupe plus le pas en cours.** Elle remplaçait le déclenchement du
  séquenceur au lieu de s'y ajouter : le coup du pattern était tronqué.
- **Et elle ne met plus les autres voix à zéro.** Chaque note MIDI reposait les douze
  vélocités d'un coup, avec des valeurs nulles pour toute voix jamais frappée en MIDI.
  Séquenceur en marche ça se rattrapait au pas suivant ; à l'arrêt, non.
- **En synchro esclave, l'horloge n'est plus doublée en sortie.** La bibliothèque MIDI
  renvoyait vers la sortie tout ce qui arrivait à l'entrée, horloge comprise : un appareil
  chaîné derrière recevait 48 impulsions par noire au lieu de 24.
- Plus d'impulsion DIN-sync ou trigger perdue en jeu MIDI dense. Les bits d'adresse du
  convertisseur de vélocité et les sorties temps réel partagent le même port, et une note
  reçue pouvait écraser une impulsion en cours d'écriture.

## Fiabilité

- **Plus de gel au démarrage.** Il arrivait une fois sur deux ou trois : deux fusibles
  faux depuis l'assemblage du PCB — détection de sous-tension désactivée, oscillateur mal
  configuré — et deux attentes non bornées dans le code.
- Plus de LED ni de sorties trigger aléatoires à la mise sous tension.
- L'encodeur lisait un créneau à 16 MHz sur son canal B depuis toujours : l'horloge
  système sortait sur cette broche. C'était le tout premier symptôme du projet.
- L'interruption d'horloge faisait trente-six divisions par tick, sur un processeur qui
  n'en sait pas faire. Elle en fait **zéro**. Le temps rendu à la boucle principale se
  voit sur tout ce qui répond au geste.

---

Compilé pour ATmega1284P avec MightyCore, zéro warning. Les patterns et les pistes vivent
dans les EEPROM externes : aucune mise à jour ne les touche.
