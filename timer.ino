//======================================================================================
//
//   T I M E R . I N O                          qui bat, et qui relache
//
//   Timer1 bat la mesure : c'est lui qui declenche CountPPQN() 96 fois par noire.
//   Tout le reste du fichier ne fait qu'une chose, RELACHER ce que le sequenceur a arme.
//   Et il le fait de deux facons, pour deux destinations qui n'ont rien a voir :
//
//     les seize bits des registres a decalage   Timer2, en interruption,  2 ms
//     TRIG-1, la prise du panneau               millis(), a la boucle,   10 ms
//
//   Les deux durees sont heritees et n'ont jamais ete justifiees nulle part.
//
//   Ces deux moities vivaient dans deux fichiers, et chacun n'en disait que la sienne :
//   l'en-tete d'ici affirmait que Timer2 relachait "les sorties de trig", ce qui etait
//   faux puisqu'il ne touche pas a TRIG-1, et Trig.ino ne savait rien de Timer2. Il
//   fallait ouvrir les deux pour voir l'asymetrie - c'est precisement pourquoi personne
//   ne l'avait vue. Reunis le 14/09/2026.
//
//--------------------------------------------------------------------------------------
//   AVANT D'Y TOUCHER
//
//   1. LES SIX SORTIES DE COMPARAISON TOMBENT SUR DES LIGNES DE COMMANDE. Releve sur le
//      schema, docs/eagle/ - ce n'est pas une coincidence, le concepteur a mis les
//      selections de boitier sur les ports B et D, exactement la ou l'AVR place ses
//      sorties de PWM :
//
//        OC0A  PB3   TRIG_CS     le verrou des registres de trig
//        OC0B  PB4   LED_CS      le verrou des registres de LED
//        OC1B  PD4   SW_CS       le chargement des registres de boutons
//        OC1A  PD5   DAC_CS      la selection du convertisseur
//        OC2B  PD6   INH_MUX1    l'inhibition du premier multiplexeur
//        OC2A  PD7   INH_MUX2    l'inhibition du second
//        OC3A  PB6   DIN         l'entree de la chaine de registres
//        OC3B  PB7   SCK         L'HORLOGE SPI DE TOUTE LA MACHINE
//
//      Tant que les bits COM de TCCR0A, TCCR1A, TCCR2A restent a zero, les broches
//      appartiennent au port et tout va bien. C'est le cas aujourd'hui - "TCCR1A = 0" et
//      "TCCR2A = _BV(WGM21)" ne sont pas seulement un choix de mode, ils garantissent
//      cela - mais rien ne le disait.
//
//      Concretement : JAMAIS d'analogWrite() sur PB3, PB4, PD4, PD5, PD6 ou PD7, et si
//      Timer3 sert un jour au flam, ne pas armer ses sorties de comparaison : elles sont
//      sur le bus SPI.
//
//   2. SUR CETTE CARTE, SEULE TRIG-1 SORT VRAIMENT. Releve sur le schema, docs/eagle/ :
//
//        PA2   TRIG_1   -> J2, un connecteur.   La seule qui aille quelque part.
//        PA3   TRIG_2   -> TEST_MODE.1          une broche du cavalier de test
//        PA4   TRIG_3   -> TEST_MODE.3          idem
//
//      Le pattern porte pourtant une piste TR2 complete - seize pas, editable,
//      enregistree en EEPROM - et Clock.ino fait bien TR2_HIGH quand elle se declenche.
//      Le signal s'arrete sur un cavalier. Et il n'en revient jamais : TR2_LOW n'est
//      ecrit NULLE PART dans le depot, InitIO() ne pose que TR1_HIGH, et trig2State est
//      mis a HIGH sans que personne ne le relise.
//
//   3. TIMER2 PLUTOT QU'UNE COMPARAISON SUR TIMER1. En synchro esclave, Timer1 tourne
//      a la cadence du maitre et non a celle du tempo interne, et il ne tire que s'il a
//      du credit : une comparaison OCR1B y serait tributaire d'une horloge exterieure et
//      s'arreterait avec elle, les trigs restant colles en l'air. Timer2, lui, ne depend
//      de rien. (Avant le 18/09/2026, Timer1 etait simplement arrete en esclave, puis il
//      a interpole par rafales : la conclusion n'a jamais change, la raison deux fois.)
//
//   4. StartTrigPulseIfIdle() NE RELANCE PAS UNE IMPULSION EN COURS. Les voix partent
//      sur le FRONT montant : un bit qui ne redescend jamais fait taire les notes
//      suivantes du meme instrument. Un flux MIDI serre relancerait le compte sans fin.
//
//   5. TimerSetFrequency() FAIT UNE SEULE DIVISION, en 32 bits. L'ancienne en faisait
//      deux en 16 bits et tronquait deux fois : 7 des 221 tempos etaient justes, avec
//      jusqu'a 0,54 BPM d'ecart - 0,75 s de derive en quatre minutes.
//
//======================================================================================

//Demarrer le compteur de mesure--------------------------------------------------------
//L'ordre compte : le mode et la periode sont poses AVANT d'autoriser l'interruption, et
//le drapeau de comparaison est efface juste avant. Sans cet effacement, une comparaison
//restee en attente d'un arret precedent partirait des l'autorisation - un tick fantome
//au demarrage. StartTrigPulse() prend la meme precaution sur Timer2 depuis toujours ;
//Timer1 ne l'avait pas, par simple asymetrie.
//TCCR1A reste a zero : cf le point 1 de l'en-tete, ses bits COM commandent DAC_CS.
void TimerStart()
{
  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);  //CTC, prediviseur 8 : 16 MHz / 8 = 2 MHz de comptage
  TCNT1 = 0;                        //un depart franc : sans ca, le rattrapage ci-dessous
                                    //verrait un compte herite et tirerait un tick aussitot
  TimerSetFrequency();              //pose OCR1A, et recale TCNT1 s'il est deja au-dela
  TIFR1 = _BV(OCF1A);               //jeter une comparaison restee en attente
  TIMSK1 = _BV(OCIE1A);
}

//Arreter le compteur de mesure---------------------------------------------------------
//L'horloge se coupe D'ABORD, le reste ensuite. L'ecriture chainee "TCCR1A = TCCR1B =
//OCR1A = 0" faisait l'inverse : elle evalue de droite a gauche, donc elle mettait OCR1A
//a zero pendant que le compteur tournait encore en CTC. Sans consequence en pratique,
//l'arret suivant de quelques cycles - mais c'etait juste par accident.
void TimerStop(void)
{
  TCCR1B = 0;                       //l'horloge d'abord : plus rien ne compte
  TIMSK1 = 0;
  TCCR1A = 0;
  OCR1A = 0;
}

//L'impulsion de trig, un coup unique----------------------------------------------------
//L'interruption du sequenceur tenait les sorties des registres a decalage en l'air avec
//un delayMicroseconds(2000) - 2 ms d'attente active DANS l'interruption, plus que la
//periode d'un tick entier a 250 BPM. C'est Timer2 qui porte l'impulsion desormais, et il
//declenche une interruption minuscule dont tout le travail est de relacher les sorties.
//Timer2 plutot qu'une comparaison OCR1B sur Timer1 : cf le point 3 de l'en-tete. En
//esclave, Timer1 ne tourne que par rafales entre deux impulsions MIDI, donc une
//comparaison sur lui tomberait dans les trous et le trig resterait colle.
//Prediviseur 128 -> 125 kHz, un tick vaut 8 us, donc 2 ms font 250 ticks.
#define TRIG_PULSE_TICKS 250

void StartTrigPulse()
{
  TCCR2A = _BV(WGM21);              //CTC, OCR2A est le sommet - et les bits COM restent
                                    //a zero, ils commandent les deux INH_MUX (point 1)
  TCNT2  = 0;
  OCR2A  = TRIG_PULSE_TICKS - 1;
  TIFR2  = _BV(OCF2A);              //jeter une comparaison restee en attente
  TIMSK2 = _BV(OCIE2A);
  TCCR2B = _BV(CS22) | _BV(CS20);   //depart, prediviseur 128
}

//Armer l'impulsion sans relancer celle qui court deja---------------------------------
//Le declenchement MIDI s'AJOUTE a ce qui est arme (AddDoutTrig). S'il relancait le
//compte a chaque note, un flux plus serre que 2 ms tiendrait le registre haut
//indefiniment - et comme les voix se declenchent sur le FRONT montant, un bit qui ne
//redescend jamais fait taire les notes SUIVANTES du meme instrument. Le note-off remet
//instWasMidiTrigged a FALSE, mais le bit serait deja a 1 : pas de nouveau front, pas de
//son.
//On laisse donc courir l'echeance en cours. Une note arrivant a 1,9 ms n'a que 0,1 ms
//d'etat haut, ce qui suffit : elle a eu son front, et c'est tout ce que la voix regarde.
//Le sequenceur, lui, garde un StartTrigPulse() inconditionnel - il REMPLACE le masque,
//donc ses voix ont besoin d'un front neuf et d'une impulsion pleine.
//TCCR2B non nul = Timer2 en marche ; l'interruption de fin le remet a zero.
void StartTrigPulseIfIdle()
{
  if (TCCR2B) return;
  StartTrigPulse();
}

ISR(TIMER2_COMPA_vect)
{
  TCCR2B = 0;                       //coup unique : le compteur s'arrete lui-meme
  TIMSK2 = 0;
  ResetDoutTrig();
}

//La periode de Timer1 pour un tempo donne----------------------------------------------
//Cadence voulue = (BPM * PPQN) / 60 Hz, donc une periode de (F_CPU / 8) * 60 / (BPM *
//PPQN) comptes. Tenu en 32 bits et divise UNE fois. L'ancienne version divisait en deux
//temps, (bpm * PPQN) / 60 puis (F_CPU / 8) / ce resultat : elle tronquait deux fois et
//ne tombait juste que sur 7 des 221 tempos - jusqu'a 0,54 BPM d'ecart, soit 0,75 s de
//derive en quatre minutes a 121 BPM.
//En CTC le compteur fait OCR1A + 1 ticks par periode, d'ou le -1.
//La borne du tempo n'est pas cosmetique : elle interdit un bpm nul, qui diviserait par
//zero. Et le resultat tient dans les 16 bits d'OCR1A sur toute la plage - 41666 comptes
//a 30 BPM, 5000 a 250.
void TimerSetFrequency()
{
  unsigned int bpm = constrain(seq.bpm, MIN_BPM, MAX_BPM);
  unsigned long ticks = ((unsigned long)(F_CPU / 8) * 60UL) / ((unsigned long)bpm * PPQN);

  uint8_t sreg = SREG;
  cli();
  OCR1A = (unsigned int)(ticks - 1);
  //Un OCR1A neuf plus bas que le compte deja atteint serait simplement manque : le
  //compteur grimperait jusqu'a 0xFFFF avant de reboucler, un hoquet de 32 ms.
  //
  //ON FAIT PARTIR LE TICK DU, ON NE REDEMARRE PAS LA PERIODE. La parade d'ici remettait
  //TCNT1 a zero, ce qui ne perd pas de tick mais en ETIRE un : celui qui etait deja du
  //attendait encore une periode entiere, jusqu'a 5,2 ms a 120 BPM. Poser OCR1A - 1 fait
  //partir la comparaison au compte suivant, donc tout de suite.
  //L'idee vient du firmware MSS pour la NAVA, qui a trouve le meme defaut de son cote
  //et note qu'il s'entendait A CHAQUE CRAN de la molette de tempo en marche.
  //OCR1A ne peut pas valoir zero ici : ticks vaut au moins 5000 a 250 BPM.
  if (TCNT1 >= OCR1A) TCNT1 = OCR1A - 1;
  SREG = sreg;
}

//L'HORLOGE ESCLAVE, INTERPOLEE---------------------------------------------------------
//En synchro esclave l'horloge MIDI arrive a 24 ppqn et la machine bat a 96 : chaque
//impulsion recue doit rendre QUATRE sous-ticks. HandleClock() appelait CountPPQN() quatre
//fois d'affilee, donc les quatre au meme instant d'horloge murale.
//
//Un pas part quand "ppqn % scale == retard", et le retard se compte en ticks de 96 ppqn.
//Quatre ticks au meme instant, ce sont quatre retards qui n'en font qu'un : le swing
//s'effondrait sur une resolution de 1/24 de noire - 20,8 ms a 120 BPM - et les seize crans
//de shuffle ne rendaient que trois ou quatre valeurs distinctes. Le micro-timing aurait
//subi exactement la meme chose.
//
//Timer1 tourne donc LIBREMENT a la cadence mesuree - l'intervalle entre les deux dernieres
//impulsions, divise par quatre - et place les quatre sous-ticks. Il compte a 2 MHz, donc un
//sous-tick de dt/4 microsecondes vaut dt/2 comptes.
//
//LA PHASE N'EST POSEE QU'UNE FOIS, au depart. La v2.055 la recalait sur CHAQUE impulsion,
//en tirant le premier sous-tick depuis la boucle principale : la phase etait exacte, mais
//le plus important des quatre ticks portait la gigue de la boucle. Un pas non swingue part
//quand ppqn % scale vaut zero, et tous ces instants-la sont des debuts de groupe - donc les
//temps forts etaient les seuls a trembler, pendant que les contretemps swingues, tires par
//l'interruption, etaient parfaitement droits. L'inverse de ce qu'on veut.
//
//La mesure sert de cadence pour l'intervalle qui vient, ce qui est la seule chose possible :
//on ne peut pas etaler quatre ticks sur une periode qui n'est pas encore ecoulee. Un maitre
//qui accelere ou ralentit se rattrape a l'impulsion suivante, le credit absorbant l'ecart.
//
//OU LA PREDICTION CASSE. Anton Savov - antto - fait le meme pari dans le n0nx0x2, son
//firmware pour x0xb0x, pour convertir l'horloge MIDI en DIN sync, et son manuel enumere
//les cas ou il se perd (antonsavov.net, n0nx0x2 manual, section 3.1.1) : un maitre qui change de tempo
//d'un tick a l'autre en changeant de morceau, un maitre qui CESSE d'emettre a l'arret -
//beaucoup de stations le font - et une horloge volontairement shufflee.
//Les trois arrivent ici de la meme facon : la prediction est trop longue, les sous-ticks
//restants n'ont pas fini de partir, et l'impulsion suivante les tire d'un coup en
//arrivant. C'est exactement l'ancien comportement, pour cette impulsion-la seulement, et
//la suivante retrouve la phase. Le garde-fou de plage attrape le cas du maitre relance
//apres un silence, ou l'intervalle mesure n'a plus aucun sens.
//Savov offre une largeur d'impulsion FIXE - 2 a 5 ms - pour quand la prediction casse,
//parce que sa sortie DIN en depend. Ici elle n'en depend pas : la sortie DIN tient un
//rapport cyclique moitie, soit 1250/BPM millisecondes, donc JAMAIS MOINS DE 5 ms sur les
//30 a 250 BPM de la machine - la plus large des cinq valeurs fixes qu'il propose. Rien a
//prevoir tant qu'aucun appareil DIN ne se plaint.
void SlaveClockRate(unsigned long dt)
{
  unsigned long counts = dt >> 1;
  //Hors de cette plage on ne sait pas ce qu'on mesure : premiere impulsion apres le
  //passage en esclave, maitre arrete puis relance, cable rebranche. On garde alors la
  //cadence precedente plutot que de piloter le timer avec n'importe quoi - les credits,
  //eux, continuent d'arriver, donc rien ne se perd.
  //La plage couvre 20 a 600 BPM la ou la machine en admet 30 a 250.
  if (counts < 2000 || counts > 60000) return;

  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);  //CTC, prediviseur 8 : 2 MHz de comptage
  TIMSK1 = _BV(OCIE1A);

  uint8_t sreg = SREG;
  cli();
  unsigned int top = (unsigned int)counts - 1;
  OCR1A = top;
  if (!slavePhaseSet){
    //LA PHASE NE SE POSE QU'UNE FOIS, au premier intervalle mesurable apres un depart.
    //TCNT1 juste sous le sommet fait partir le tick au compte suivant, donc ici : le
    //premier sous-tick tombe sur l'impulsion recue, et le decalage contre le maitre est
    //nul. Ce tick-la, et lui seul, porte la latence de la boucle principale.
    TCNT1 = top - 1;
    slavePhaseSet = TRUE;
  }
  else if (TCNT1 >= top){
    //Meme rattrapage qu'en maitre, cf TimerSetFrequency() : un sommet deja depasse serait
    //manque et le compteur courrait jusqu'a 0xFFFF.
    TCNT1 = top - 1;
  }
  SREG = sreg;
}

//LE TEMPO DU MAITRE, MESURE SUR QUATRE NOIRES-------------------------------------------
//En esclave, l'ecran affichait trois paves pleins a la place du tempo : la machine ne bat
//pas, donc seq.bpm ne veut plus rien dire - mais le maitre, lui, a un tempo, et rien
//n'empechait de le lire dans ce qu'il envoie.
//
//QUATRE NOIRES, ET PAS UN INTERVALLE. Un intervalle isole porte toute la gigue de
//l'emetteur, du cable, de l'opto-coupleur et de notre propre boucle : un affichage qui
//papillonne entre 119 et 121 est pire que pas d'affichage du tout, parce qu'on ne peut
//plus distinguer un maitre qui derive d'un maitre qui va bien. Sur 96 impulsions la
//gigue se moyenne et l'entier affiche ne bouge plus.
//C'est la lecon du n0nx0x2 sur le meme probleme.
//
//240000000 = 4 x 60e6 : quatre noires en dt microsecondes font 4*60e6/dt battements par
//minute. La somme tient large en 32 bits - 8e6 microsecondes a 30 BPM - et la demi-somme
//ajoutee arrondit au lieu de tronquer, un maitre a 90,0 s'affichant sinon 89.
//
//Un intervalle hors plage remet le compte a zero : il empoisonnerait la moyenne, et il
//veut dire qu'il s'est passe quelque chose - maitre relance, cable rebranche.
//La derniere valeur affichee, elle, survit : effacer l'ecran a chaque PLAY ne sert personne.
#define SLAVE_BPM_CLOCKS 96

void SlaveBpmMeasure(unsigned long dt)
{
  static unsigned long bpmSum = 0;
  static byte bpmCount = 0;

  if (dt < 4000UL || dt > 120000UL){
    bpmSum = 0;
    bpmCount = 0;
    return;
  }
  bpmSum += dt;
  if (++bpmCount < SLAVE_BPM_CLOCKS) return;

  unsigned int v = (unsigned int)((240000000UL + (bpmSum >> 1)) / bpmSum);
  bpmSum = 0;
  bpmCount = 0;
  if (v != slaveBpm){
    slaveBpm = v;
    needLcdUpdate = TRUE;//deux secondes entre deux au plus, l'ecran peut le porter
  }
}

//Couper les sous-ticks encore armes-----------------------------------------------------
//Une remise a zero de ppqn - PLAY, START recu, CONTINUE - annule le groupe en cours : les
//sous-ticks qui restaient appartenaient a une phase qui n'existe plus, et les laisser
//partir declencherait des pas que personne n'a demandes, juste au moment ou la machine
//demarre. La prochaine impulsion recue reposera la phase.
//En maitre, Timer1 EST l'horloge de la machine : on n'y touche pas.
void SlaveClockIdle()
{
  if (seq.sync == MASTER) return;
  TIMSK1 = 0;
  TCCR1B = 0;
  uint8_t sreg = SREG;
  cli();
  slaveTicks = 0;//les credits d'un groupe que la remise a zero vient d'annuler
  SREG = sreg;
  slavePhaseSet = FALSE;//la prochaine impulsion reposera la phase
}

//Relacher TRIG-1 quand sa duree est ecoulee--------------------------------------------
//Appelee a chaque tour de boucle principale. C'est l'autre moitie du metier de ce
//fichier : pas d'interruption ici, une simple comparaison sur millis().
void SetTrigTime()
{
   if (trig1State && !trigTimeOk){
    trigTime = millis();
    trigTimeOk = HIGH;
  }
  //Ce test tournait sans garde, et c'etait deux bugs sur une ligne.
  //
  //UN : trig1State est leve par l'interruption du sequenceur (Clock.ino). Quand elle
  //tombait entre les deux if, trigTime portait encore l'horodatage de l'impulsion
  //precedente, donc millis() - trigTime depassait deja TRIG_TIME_MS et l'impulsion toute
  //neuve etait coupee sur place - le coup de trig ecourte qu'on entendait de temps en
  //temps sur TRIG-1. Diagnostique independamment par zabox sur la NAVA, "[1.027] solves
  //short trig 1 pulse".
  //
  //DEUX : la note MIDI 60 mene TRIG-1 comme une porte, TR1_LOW a l'appui et TR1_HIGH au
  //relacher (Midi.ino). Sans garde, ce test tournait a chaque tour de boucle et ramenait
  //TR1 en l'air aussitot : cette porte ne survivait pas a une iteration.
  //
  //Exiger trigTimeOk fait que seule une impulsion demarree PAR cette fonction peut etre
  //arretee par elle, et seulement une fois son horodatage pris.
  if (trigTimeOk && (millis() - trigTime >= TRIG_TIME_MS)){
    TR1_HIGH;//TRIG-1 revient au repos - cette sortie est active a l'etat bas
    trig1State = trigTimeOk = LOW;
  }
}
