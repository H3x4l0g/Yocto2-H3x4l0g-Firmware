# Yocto v2 — changelog

De la **v2.003beta** d'e-licktronic — dernière version publiée, projet retiré du commerce
en 2017 — à la **v2.017**. Seul ce qui change le comportement de la machine est listé ;
le détail technique vit dans `TODO.md`.

## Encodeur

- Un cran de la molette avance d'un pas. Il en comptait **quatre**.
- En PATT EDIT la molette choisit l'**instrument** : le nom, la longueur, la scale et la
  grille de pas suivent. Elle était sur le tempo, qui n'est même pas affiché là.
- TEMPO maintenu rend la main au tempo, depuis n'importe quel écran d'édition.
- SHUFFLE maintenu règle le shuffle en PATT TAP aussi. Il y bougeait le tempo en silence.
- La valeur de Global Accent — ex « Total Accent » — se règle avec INST maintenu.

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

## Pistes

- La longueur ne se corrompt plus à 999.
- BACK et FWD avancent d'un pas, plus de deux.
- L'insertion fonctionne sur une piste vide.

## Synchro et sorties

- TRIG-1 : plus d'impulsions tronquées, et la note MIDI 60 pilote à nouveau la sortie.
- Horloge MIDI entrante : 2 ms d'attente active par tick supprimées, soit 48 ms par
  seconde à 120 BPM rendues à la boucle principale.
- Dérive sur quatre minutes ramenée de 0,75 s à 13 ms.

## Fiabilité

- **Plus de gel au démarrage.** Il arrivait une fois sur deux ou trois : deux fusibles
  faux depuis l'assemblage du PCB — détection de sous-tension désactivée, oscillateur mal
  configuré — et deux attentes non bornées dans le code.
- Plus de LED ni de sorties trigger aléatoires à la mise sous tension.
- L'encodeur lisait un créneau à 16 MHz sur son canal B depuis toujours : l'horloge
  système sortait sur cette broche. C'était le tout premier symptôme du projet.

---

Compilé pour ATmega1284P avec MightyCore, zéro warning. Les patterns et les pistes vivent
dans les EEPROM externes : aucune mise à jour ne les touche.
