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
//   3. TIMER2 PLUTOT QU'UNE COMPARAISON SUR TIMER1, et ce n'est pas un detail : en
//      synchro esclave Timer1 est ARRETE, CountPPQN() etant appele par l'horloge MIDI
//      entrante. Une comparaison OCR1B ne se declencherait jamais et les trigs
//      resteraient colles en l'air.
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
//Timer2 plutot qu'une comparaison OCR1B sur Timer1 : en synchro esclave Timer1 est
//arrete et CountPPQN() est appele par l'horloge MIDI entrante, donc une comparaison sur
//Timer1 ne partirait jamais et le trig resterait colle.
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
  //compteur grimperait jusqu'a 0xFFFF avant de reboucler, un hoquet de 32 ms. On
  //redemarre la periode plutot.
  if (TCNT1 > OCR1A) TCNT1 = 0;
  SREG = sreg;
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
