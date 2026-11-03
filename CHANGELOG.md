# Yocto v2 — changelog

De la **v2.003beta** d'e-licktronic — dernière version publiée — à la **v2.074**. Seul ce qui change le comportement de la machine est listé.

## Encodeur

- Un cran de la molette avance d'un pas. Il en comptait **quatre**.
- Sur les écrans d'édition de pattern, la molette nue ne touche plus au **tempo** — qui
  n'y est même pas affiché. On ne le déplace plus en croyant régler autre chose.
- TEMPO maintenu reste le chemin vers le tempo, et en affiche la valeur pendant qu'on
  tourne, depuis n'importe quel écran d'édition. **Y compris sur le slot ACC et en mode
  clavier**, où l'écran montrait bien le tempo pendant que la molette éditait l'accent
  global ou la note sous le curseur.
- **Et le tempo suit la main.** Tant que TEMPO était tenu, l'écran se réécrivait à chaque
  tour de boucle : la machine passait son temps à parler au LCD au lieu de lire la molette,
  et un cran sur deux se perdait dès qu'on tournait un peu vite. L'écran ne se redessine
  plus que lorsque la valeur change.
- SHUFFLE maintenu règle le shuffle en PATT TAP aussi. Il y bougeait le tempo en silence.
- Le « Total Accent » s'appelle **Global Accent** : tous les paramètres qui portent sur
  le pattern entier sont des « global » dans cette machine.
- **Sur la page 2 du mode config, la molette n'écrit plus les réglages de la page 1.**
  Les trois positions marquées `nc.` — celles qui affichent `xxx` — réglaient en réalité
  le BPM de démarrage et les deux canaux MIDI. On pouvait ressortir du mode config avec un
  canal de réception qu'on n'avait pas choisi, sans rien à l'écran pour le dire, et ENTER
  le gravait. Elles ne font plus rien.
- **La molette ne touche qu'à ce qui est affiché.** C'est désormais la règle, et elle
  tient en une phrase. En MUTE elle réglait le tempo en silence pendant que l'écran
  montrait une page d'édition, un bargraphe de shuffle ou l'accent global : elle retrouve
  le mode qui joue dessous. En TRACK PLAY, où le tempo n'est pas affiché non plus, elle
  devient inerte. PATT PLAY reste le seul écran où elle règle le tempo nue — c'est le seul
  où il se lit.

## Micro-timing — nouveau

- **Chaque pas peut être retardé de 0 à 23 ticks**, soit environ 5 ms par cran à 120 BPM.
  De quoi pousser une voix derrière le temps sans changer la grille.
- **Pour jouer un coup AVANT un pas, on l'écrit sur le pas précédent et on le retarde
  presque à fond.** Le clap juste avant la caisse claire se fait comme ça.
- **Le shuffle mange une partie de la course.** Un pas swingué est déjà en retard : ce
  qui reste jusqu'au coup suivant est plus court, et le réglage s'y arrête au lieu de
  faire disparaître le coup.
- Le réglage est le troisième champ de la page 2 : `pas rtg utm ins`.
- Un roulement part avec son coup : le décalage emmène le groupe entier.
- La piste externe n'est pas concernée.

## Retrig — nouveau

- **Tenir SHUFFLE affiche à nouveau son bargraphe** quand un écran de réglage de pas est
  ouvert. La molette réglait bien le shuffle, mais l'écran montrait autre chose : on
  réglait à l'aveugle.

- **Chaque pas peut sonner 2, 3, 4, 6 ou 8 fois dans son propre intervalle.** Roulements de
  charley, buzz de caisse claire, bégaiement de grosse caisse — et les triolets sont
  atteignables, ce qui a décidé de la forme du réglage.
- **`SHIFT` + une touche de pas, deux fois** : la première ouvre les conditions de ce pas,
  la seconde son roulement, la troisième referme. Comme les pages du mode config.
- Les seize LED montrent les pas qui roulent, comme elles montrent les pas conditionnés.
- Le placement d'un coup ne dépend plus du nombre de voix qui roulent en même temps.
- Toutes les répétitions sonnent **au même niveau** que le coup principal, et le pas
  suivant interrompt le roulement.
- Une condition ou une sourdine gouverne **le pas entier**, répétitions comprises.
- **Les patterns déjà enregistrés ne changent pas.**

## Conditions de trig — nouveau

- **Décaler un pattern emmène ses conditions avec lui.** Les flèches de décalage
  faisaient avancer les trigs et les vélocités en laissant les conditions sur place :
  chaque pas héritait de la condition de son voisin.

- **Chaque pas peut ne jouer qu'une mesure sur X, ou une fois sur deux.** Deux réglages par
  pas et par instrument : une condition cyclique — `1:2` une mesure sur deux, `3:4` la
  troisième de quatre, jusqu'à `5:8` — et une probabilité. Les deux se combinent : `1:4` à
  50 %, c'est une note qui tombe une mesure sur quatre, et seulement une fois sur deux
  parmi celles-là.
- **SHIFT + une touche de pas** ouvre l'écran de ce pas-là, en édition de pattern. Le même
  geste sur le même pas le referme ; sur un autre pas, il y saute. Pendant ce temps les
  seize LED montrent quels pas de la voix portent une condition.
- Un pas conditionné **se tait sans quitter la grille** : la piste externe garde le rapport
  entre sa grille et sa pile de notes, exactement comme sous un mute.
- **Les patterns déjà enregistrés ne changent pas.** La place existait dans la mémoire
  depuis l'origine sans jamais servir ; un marqueur distingue les patterns qui portent des
  conditions de ceux qui n'en ont jamais eu.
- La molette de probabilité se lit dans le sens de la densité, du premier cran au dernier :
  6 %, 12 %, … 93 %, puis 100 % qui veut dire toujours.
- Le champ d'instrument enjambe le slot d'accent global, qui n'a pas de pas à conditionner.
- **Effacer un pas efface sa condition.** Elle restait en place : réécrire un trig au même
  endroit le rendait conditionné sans que rien ne le dise.
- **Les seize LED montrent la tête de lecture pendant qu'on règle une condition.** Elles
  n'affichaient que les pas conditionnés : régler « une mesure sur quatre » sans voir
  tourner la mesure revenait à régler à l'aveugle.
- CLEAR efface les conditions avec le reste.

## Mute et solo

- **Sortir du solo rend les sourdines qu'on avait posées avant.** Le solo les écrasait :
  il fallait ensuite ressortir pad par pad. C'est maintenant un calque, pas une réécriture.
- **Soloer une voix ne lui retire plus ses accents**, et n'arrête plus les deux sorties de
  trig. L'accent global n'est pas un son mais un niveau ajouté à une voix, et TR1/TR2
  clockent du matériel extérieur : soloer la grosse caisse ne doit pas arrêter le
  séquenceur qu'elle pilote. La piste EXT, elle, se tait comme les autres voix — elle fait
  du son.
- On peut soloer **plusieurs voix ensemble** ; SHIFT + le pad d'une voix soloée la retire,
  et retirer la dernière sort du solo. La pression sur l'encodeur rend tout, solo compris.
- Pendant un solo, un pad seul ne fait plus rien : il modifiait une sourdine invisible
  qu'on ne découvrait qu'en sortant.
- **L'encodeur tenu enfoncé n'empêche plus de muter.** Il remettait les sourdines à zéro
  en continu, donc toute pression sur un pad était défaite aussitôt.

## Mode config

- **Quitter le mode config sans avoir fait ENTER l'affiche : `[NOT SAVED!]`.** Les réglages
  restent actifs, mais ne survivaient pas à l'extinction, et l'avertissement — la LED
  d'ENTER qui clignote — s'éteignait avec le mode, donc à l'instant où il servait.
- **Le curseur ne s'arrête plus sur les trois emplacements vides de la page 2.** Il y
  disparaissait trois fois sur quatre, ce qui se lisait comme une page en panne.

## Modes

- Démarrage sur **PATT PLAY**, plus sur l'écran d'édition.
- Changer de famille TRACK ↔ PATT demande l'**arrêt** du transport : les deux ne jouent
  pas la même chose. Le passage conserve alors le caractère lecture/édition, au lieu de
  retomber sur l'écran d'édition quoi qu'on quitte.
- La bascule lecture ↔ édition **dans** une famille reste libre, machine en marche
  comprise : elle ne change que ce que font les touches, jamais ce qui joue.
- Quitter TRACK par la touche PATT ne jette plus les modifications de piste en silence.
- **MUTE s'annonce en grand à l'écran.** Rien ne disait qu'on y était : on s'en apercevait
  en voulant changer de pattern, trop tard.

## Lisibilité du panneau

- **La LED NUM s'allume quand on tient NUM.** Elle n'était jamais allumée — le firmware
  réservait la lampe et n'y écrivait rien. NUM montre le numéro de piste, sert de
  modificateur et fait entrer en mode clavier : c'était le seul bouton maintenu dont le
  panneau ne disait rien.
- BANK maintenu allume les **huit banques** en veilleuse, la courante à pleine intensité.
- Les touches 9 à 16 ne sélectionnent plus la banque H : ce ne sont pas des banques.
- Le curseur de lecture suit la mesure, plus la longueur de l'instrument sélectionné.
- L'écran ne reste plus figé en MUTE.
- Écran d'accueil centré, avec un balayage des LED de pas.

## Les boutons du panneau

- **Les boutons ont enfin un vrai anti-rebond.** Ce qui s'appelait comme ça n'en était
  pas un : la machine comparait deux lectures prises à quelques dixièmes de milliseconde
  l'une de l'autre. Elles sont maintenant espacées de 5 ms pour de bon.
- **Et le panneau a 5 ms de retard en plus** — une pression était vue entre 0 et 5 ms
  après le doigt, elle l'est entre 5 et 10 ms. C'est un retard **constant**, pas une
  irrégularité : la main s'y accorde sans le savoir.
- Ce qu'on achète avec : la machine passait environ **5 % de son temps les interruptions
  coupées** à relire les boutons en boucle, et c'est l'interruption coupée qui décale les
  coups. C'est tombé à 0,4 %.

## Touches de pas

- Une touche pouvait devenir **définitivement muette** : sortir du mode clavier en la
  maintenant la tuait jusqu'au prochain passage par ce mode. Corrigé.
- **En mode TAP, les touches 13 à 16 jouent les quatre pistes sans voix** — accent global,
  instrument externe, TRIG-1 et TR2. Elles frappaient toutes une grosse caisse, et
  l'enregistraient : quatre touches pour douze instruments écrits, les quatre dernières
  tombaient sur la case zéro.
- **Une frappe en TAP ne relâche plus une note MIDI tenue.** Le balayage du convertisseur
  de vélocité, recopié à la main dans ce mode, réécrivait douze fois par frappe le port qui
  porte aussi les sorties de trig : le gate d'une note 60 retombait au passage.
- **Les frappes du mode TAP restent dans le pattern où on les a jouées.** Elles
  n'entrent dans le pattern qu'à la fin de la mesure, pour ne pas sonner deux fois. Mais
  quitter TAP ou arrêter avant la barre les laissait en attente, sans rien à l'écran : au
  passage suivant en TAP, elles tombaient dans le pattern courant de ce moment-là, alors
  que leur vélocité était restée dans le premier. Sortir du mode ou arrêter les inscrit
  maintenant tout de suite, et une sauvegarde faite à la barre ne leur fait plus manquer
  la fusion. *(v2.072)*
- **Un accord tapé en TAP tombe sur un seul pas, à pleine vélocité.** Chaque frappe
  lisait le pas deux fois, pour la vélocité puis pour le trig : sur le seuil d'arrondi,
  la vélocité pleine partait sur un pas et le trig sur le suivant, et la frappe
  s'enregistrait en demi. Les voix d'un accord pouvaient aussi se partager entre deux
  pas. *(v2.073)*

## Tempo et swing

- **Tourner le tempo en marche ne décale plus le temps en cours.** Le cran arrivait pendant
  qu'une période était déjà entamée, et la suivante repartait de zéro : le temps en cours
  s'allongeait d'une fraction de pas à chaque cran. Il part maintenant à l'instant où il
  était dû.

- Les 221 valeurs de BPM sont exactes. Il n'y en avait que **sept**.
- Plus de hoquet de 32 ms en tournant le tempo.
- Plage de shuffle de 50 % à **70,8 %**, contre 62,5 % au mieux avant.
- Un cran de shuffle veut dire un **pourcentage**, le même sur les quatre résolutions.
  Shuffle triolet exact au cran 9.
- Le swing reste sur les pas impairs. Il dérivait sur les pairs, et sa phase était
  arbitraire dès qu'on changeait de pattern ou de longueur.
- Plus de déphasage permanent après un changement de longueur d'instrument.
- **SHUFFLE et SHIFT+SHUFFLE ont enfin deux écrans.** Les deux gestes dessinaient le même,
  titre compris, avec un bargraphe qui lisait toujours la valeur de la voix courante : le
  réglage global affichait donc autre chose que ce qu'il réglait, et le premier cran
  faisait sauter le curseur sans prévenir. Le titre nomme maintenant ce qu'on règle, et le
  bargraphe montre sa valeur.
- Après un réglage de shuffle global, **un cran de molette ne reste plus sans effet** au
  hasard : ni redessin, ni clignotement d'ENTER, sur la valeur que le geste global venait
  de poser partout.
- **Changer une scale pendant que ça tourne ne décale plus le pas 1.** Le compteur de
  l'instrument restait où il était et se mettait à avancer à une autre cadence : la grille
  ne retombait plus sur le temps, et le seul recours était d'arrêter puis de refaire PLAY.
  Le geste remet maintenant l'instrument en phase — l'instrument courant seul, les seize
  sur SHIFT.
- **Et la remise en phase après une édition de longueur tient enfin compte de la scale.**
  Des hats en 1/32 sous une référence en 1/16 étaient renvoyés un demi-tour de pattern en
  arrière à chaque réglage de longueur. La position se compte désormais en ticks écoulés
  depuis la barre, la seule mesure que des instruments de cadences différentes partagent.

## Patterns et chaînes

- Sélectionner une chaîne en cours de lecture ne joue plus le dernier pattern d'abord.
- Basculer édition ↔ lecture ne répète ni ne saute un pattern.
- MUTE ne gèle plus l'avancement de la chaîne.
- Le pattern courant n'est plus rejoué quand il se trouve être le premier de la chaîne.
- **La banque suit le pattern.** Arriver sur D13 par une piste affichait `A61`, éclairait
  la LED de banque A, et la touche de pas 3 renvoyait en A3. L'écran, la LED et la touche
  disent maintenant la même chose.
- **Choisir une banque ne change plus le pattern qui joue.** Venant de A3, choisir la
  banque D chargeait **D3**, et jetait la chaîne en cours au passage. Le geste arme la
  banque, rien de plus : la touche de pas qui suit choisit le pattern. L'armement attend
  cette touche — une chaîne qui avance entre les deux ne l'efface plus — et l'écran ne
  change de lettre que lorsque le pattern change vraiment.
- **Les marches au changement de pattern à haut tempo ont fondu.** Le chargement depuis
  l'EEPROM bloquait 115 ms et commençait *à* la barre : au-delà de 230 BPM les premiers
  pas du nouveau pattern se rejouaient. Le bus tourne désormais à sa vitesse nominale et
  le chargement prend 30 ms — de près de deux pas de retard à un demi.
- Une chaîne ne peut plus aller lire **hors de la zone des patterns**. Sur une chaîne dont
  la position enregistrée était incohérente, le calcul rebouclait et le séquenceur allait
  lire dans la zone des pistes : un pattern de bruit. Rien n'était détruit — c'était une
  lecture — mais ça s'entendait.
- **CLEAR remet aussi la mesure d'aplomb.** Effacer un pattern réécrivait les seize
  longueurs et divisions mais laissait la longueur, la division et le shuffle **globaux**
  sur les valeurs d'avant. Effacer un pattern de huit pas donnait seize pas par instrument
  et une mesure qui finissait toujours au huitième : chaînes et pistes avançaient au
  milieu. Et comme CLEAR arme la sauvegarde, la valeur fausse partait en EEPROM au premier
  ENTER.
- **Au changement de pattern, les compteurs s'alignent sur la barre** au lieu d'être remis
  à zéro. Zéro dit quel index, jamais quand : un instrument plus rapide que la mesure
  frappait une fois de trop avant la barre, y jouait son index 0, et tout son pattern
  restait décalé d'un pas pour toute sa durée.
- **La mesure tombe sur l'instrument le plus long en durée, et non en nombre de pas.**
  Seize pas de 1/32 durent deux temps, douze pas de 1/16 en durent trois : les premiers
  l'emportaient, et la barre tombait au milieu de la phrase des seconds. Enchaîné avec un
  pattern ordinaire, ça bouclait en 6 temps au lieu de 7. Un pattern dont tous les
  instruments partagent la résolution ne bouge pas d'un tick.
- **Le métronome n'écrase plus les vélocités du pattern.** Le clic posait ses propres
  valeurs dans la piste de rimshot : un rimshot à demi sur un temps revenait à plein après
  un aller-retour sur GUIDE, sans qu'ENTER clignote pour avertir, et la donnée partait en
  EEPROM à la première sauvegarde faite pour une autre raison. Le clic est une fonction
  machine, il ne passe plus par le pattern.

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
- **TRACK PLAY redessiné** : la piste à gauche, la position à droite, le titre seul en
  haut. Le pattern engagé n'y est plus — en mode piste, ce qu'on suit est la piste et l'on
  en est où ; le pattern est un détail d'exécution, et TRACK EDIT le montre pour qui le
  veut.
- **Le champ `num` de TRACK WRITE s'appelle `trk`** : c'est le numéro de piste, et le dire
  évite de le deviner. Surtout, **il répond enfin à la molette** — l'écran promettait un
  paramètre que la molette ne servait pas.
- **La LED ENTER ne clignote plus sur une piste jamais touchée.** Une accolade manquante
  faisait tomber l'exécution dans les champs voisins : après un chargement de piste, le
  témoin de longueur restait périmé et réarmait la sauvegarde tout seul. Maintenir ENTER
  réécrivait alors la piste en EEPROM pour rien.
- **Tenir une touche de piste ne recharge plus la piste en boucle** en TRACK PLAY. Le test
  portait sur un niveau et non sur un front : une seconde de pression valait une
  quarantaine de relectures de huit pages d'EEPROM.
- **Une position de piste jamais écrite ne fait plus jouer de bruit.** Un octet de zone
  vierge vaut 255 là où les patterns s'arrêtent à 128 : l'adresse calculée sortait de la
  zone des patterns pour aller lire dans celle des pistes.

## Instrument externe

- La pile de notes ne se tronque plus. Revenir à la note 3 d'une pile de vingt et en
  rejouer une la coupait à 4.
- **Plus de note restée en l'air.** BACK et FWD font sonner chaque note en parcourant la
  pile ; maintenir l'un puis appuyer sur l'autre coupait la mauvaise note, et la première
  tenait indéfiniment.
- Les douze notes du haut ne repartent plus dans l'octave du bas. Au-delà d'une certaine
  hauteur, la note envoyée débordait et rebouclait ; la molette s'arrête maintenant avant.
- **Plus de note en l'air au clavier.** Tenir une touche, changer d'octave, relâcher : la
  coupure partait sur la nouvelle hauteur et la note tenue sonnait indéfiniment.
- **La pile ne se troue plus.** BACK et FWD s'arrêtent à l'emplacement d'ajout, comme sur
  les pistes. On pouvait avancer à l'index 40 d'une pile de cinq notes et y poser une note :
  les places intermédiaires gardaient ce qui traînait en mémoire.
- **La pile de notes s'enregistre enfin.** Sa longueur n'était ni sauvegardée ni relue :
  les notes revenaient au complet, mais sans leur point de bouclage, qui gardait la valeur
  laissée par le pattern chargé avant. La piste EXT n'a jamais été enregistrable depuis
  2018 — le copier-coller la transportait, lui, ce qui trahissait l'oubli plutôt que le
  choix.
- **Le copier-coller ne perd plus la dernière note de la pile** — celle qui ferme la
  phrase. Elle gardait ce que le pattern de destination avait à cette place.
- **L'accent arrive sur la piste EXT.** Ses pas étaient à deux états, présent ou absent, et
  leur vélocité était bien enregistrée mais jamais relue. Ils rejoignent le cycle à trois
  états des douze voix — éteint, normal, accent — et le note-on porte la vélocité du pas :
  une x0xb0x y lit son accent. Les pas déjà enregistrés sonnent comme avant, ils
  s'affichent seulement en demi plutôt qu'en plein. Contrepartie : effacer un pas demande
  deux pressions de plus, comme sur les douze voix.
- **Le curseur ne sort plus de la pile.** On pouvait l'emmener bien au-delà de la dernière
  note, éditer là une note qui ne serait jamais jouée, et se demander pourquoi on ne
  l'entend pas.
- **Éditer une note à la molette marque le pattern** : ENTER clignote, et le travail ne
  part plus en silence au chargement suivant. La même note posée à la touche de pas était
  bien marquée — c'est l'asymétrie qui a trahi l'oubli.
- **Muter EXT ne fige plus la pile, et ne laisse plus de note en l'air.** La grille dit
  quand, la pile dit quoi, et c'est leur rapport qui fait la fonction : muter arrêtait la
  pile, donc au démute la phrase repartait décalée de tous les pas passés sous le mute. Il
  suffisait d'ailleurs de tout muter pour la figer sans toucher à EXT. Et une note tenue au
  moment du mute restait tenue jusqu'au STOP.
- **La pile repart du début au changement de pattern.** Elle était le seul état de la piste
  EXT à y survivre : aller du pattern 1 au 2 puis revenir laissait la pile du 1 décalée de
  ce que le 2 avait consommé — un pattern ne sonnait plus pareil d'une fois sur l'autre. La
  dérive contre la grille, elle, ne bouge pas : c'est la fonction.
- **PLAY ne laisse plus de note coincée.** Le note-off reconstruisait la hauteur au lieu de
  retenir celle qu'il avait envoyée ; PLAY remettant le compteur de pile à zéro, la coupure
  partait sur une autre note et celle qui sonnait n'en recevait jamais. Quelques PLAY
  d'affilée suffisaient à empiler les notes bloquées sur le synthé en face.

## Voix

- **Le demi de la PC2 remonte de 20 à 40**, comme les huit autres voix. Hérité tel quel
  d'une table de TR-909, il donnait un écart de 1 à 4 entre demi et plein, là où les huit
  autres sont à 1 pour 2 et les charleys à 1 pour 1,3. Relevé à l'oreille, confirmé par la
  table.

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
- **Le shuffle fonctionne en synchro esclave.** L'horloge MIDI entrante bat quatre fois
  moins vite que le séquenceur, et les quatre pas de compte qu'elle déclenchait partaient
  tous au même instant : le retard de swing n'avait plus que 20,8 ms de résolution à
  120 BPM, si bien que plusieurs crans de shuffle sonnaient identiques. Les quatre sont
  maintenant étalés sur l'intervalle mesuré entre deux impulsions. Le micro-timing, s'il
  arrive un jour, aurait eu exactement le même problème.
- **En synchro esclave, changer de pattern ne fait plus prendre du retard.** Le chargement
  d'un pattern tenait la machine assez longtemps pour qu'elle manque des pas de compte, et
  elle ne les rattrapait jamais : le retard s'installait, et le changement suivant s'y
  ajoutait. Les transferts avec la mémoire laissent maintenant passer l'horloge entrante,
  et l'horloge sait rattraper si elle prend malgré tout du retard.
- **En synchro esclave, les temps forts sont droits.** Le premier des quatre pas de compte
  que produit chaque impulsion d'horloge était placé par la boucle principale, et c'est
  celui sur lequel tombent tous les pas non swingués : les temps forts tremblaient d'une
  fraction de milliseconde pendant que les contretemps, eux, étaient parfaitement réguliers.
  C'est le quartz qui les place maintenant, tous les quatre.
- **Le tempo du maître s'affiche en synchro esclave.** L'écran montrait trois pavés pleins
  à la place du tempo. Il est mesuré sur quatre noires d'horloge entrante, ce qui le rend
  assez stable pour être lu — un intervalle isolé porte trop de gigue pour afficher autre
  chose qu'un chiffre qui papillonne. Les trois pavés ne restent que tant qu'il n'y a pas
  d'horloge à mesurer.
- **La sortie DIN rend un vrai signal en esclave.** Elle n'y faisait qu'une pointe de
  quelques dizaines de microsecondes, là où le mode maître rend un carré. Les deux modes
  sortent le même signal maintenant.
- **En synchro esclave, l'horloge n'est plus doublée en sortie.** La bibliothèque MIDI
  renvoyait vers la sortie tout ce qui arrivait à l'entrée, horloge comprise : un appareil
  chaîné derrière recevait 48 impulsions par noire au lieu de 24.
- Plus d'impulsion DIN-sync ou trigger perdue en jeu MIDI dense. Les bits d'adresse du
  convertisseur de vélocité et les sorties temps réel partagent le même port, et une note
  reçue pouvait écraser une impulsion en cours d'écriture.
- **Une voix ne devient plus sourde au MIDI pour de bon.** Le verrou qui empêche une note
  répétée de relancer une voix avant son relâcher n'était levé que par le note-off de la
  note exacte : un message perdu — câble débranché en cours de note, clavier remis à zéro,
  hub bavard — et la voix ne répondait plus au MIDI jusqu'à extinction de la machine, sans
  rien à l'écran pour le dire. STOP, et un Stop ou un Continue reçus, libèrent maintenant
  les seize verrous.
- **Une note MIDI ne sort plus faible ou muette.** Entre le lever des bits et l'armement de
  l'impulsion, l'interruption de relâchement pouvait tout défaire : la voix n'avait qu'une
  microseconde d'état haut au lieu des cent que le front demande. De l'ordre de 0,2 % des
  notes en jeu dense, et rien pour la distinguer d'une fausse manœuvre au clavier.
- **Un vrai note-off au lieu du seul CC 123** quand tout s'arrête. Beaucoup de machines
  ignorent le All Notes Off — une x0xb0x parmi elles.
- **Changer le canal MIDI de réception fonctionne enfin.** Le réglage était écrit mais
  jamais transmis à la réception : après l'avoir changé, la machine ne répondait plus à
  aucune note, sur aucun canal, jusqu'au prochain allumage.
- **Repasser de esclave à maître marche du premier coup.** Sur une machine sauvée en
  esclave, le premier cran de molette vers « maître » ne faisait rien du tout : ni l'écran
  ni l'horloge ne bougeaient, et il fallait d'abord tourner dans l'autre sens — où il n'y
  a rien — pour décoincer le réglage. Même chose pour la synchro de changement de pattern.

## Fiabilité

- **Sauvegarder est trois fois plus rapide, et la machine ne se fige plus pendant.**
  Après chaque page écrite, le firmware attendait 10 ms en aveugle — le double de ce que la
  fiche de l'EEPROM donne pour maximum, et le triple de ce qu'elle prend vraiment. Une
  sauvegarde de piste, c'était 80 ms pendant lesquelles plus rien ne répondait : ni les
  LED, ni la sortie TRIG-1, ni l'horloge MIDI entrante en synchro esclave. La puce dit
  maintenant elle-même quand elle a fini.
- **Une sauvegarde ratée le dit.** L'écran affichait `[SAVED!]` quoi qu'il arrive : une
  mémoire occupée, mal contactée ou absente perdait sa page en silence, et on ne
  l'apprenait qu'au rechargement. Il affiche maintenant `[SAVE FAILED n]`, où `n` est le
  code d'erreur — 2 : la mémoire ne répond pas ; 3 : elle refuse une donnée ; 4 ou 5 :
  bus I2C en défaut ; 6 : elle ne termine jamais son écriture. Les chaînes enregistrées
  au pas et l'initialisation de la mémoire au premier démarrage, qui n'affichaient rien,
  signalent aussi leur échec. *(v2.072)*
- **Plus de gel au démarrage.** Il arrivait une fois sur deux ou trois : deux fusibles
  faux depuis l'assemblage du PCB — détection de sous-tension désactivée, oscillateur mal
  configuré — et deux attentes non bornées dans le code.
- Plus de LED ni de sorties trigger aléatoires à la mise sous tension.
- L'encodeur lisait un créneau à 16 MHz sur son canal B depuis toujours : l'horloge
  système sortait sur cette broche. C'était le tout premier symptôme du projet.
- L'interruption d'horloge faisait trente-six divisions par tick, sur un processeur qui
  ne sait pas faire. Il en reste **une**, vérifiée au désassemblage. Deux d'entre elles
  tournaient même à l'arrêt, sur une machine qui ne jouait rien. Le temps rendu à la boucle
  principale se voit sur tout ce qui répond au geste.

---

Compilé pour ATmega1284P avec MightyCore, zéro warning. Les patterns et les pistes vivent
dans les EEPROM externes : aucune mise à jour ne les touche.
