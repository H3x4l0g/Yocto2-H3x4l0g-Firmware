//======================================================================================
//
//   M I D I . I N O                            ce qui entre et ce qui sort
//
//   Deux sens qui n'ont rien a voir. A l'ENTREE, des notes qui declenchent les voix
//   analogiques et une horloge qui peut asservir toute la machine. A la SORTIE, la
//   piste EXT, l'ecoute au clavier, et l'horloge quand le Yocto est maitre.
//   Aucune voix analogique n'est recopiee vers la sortie : ce n'est pas un oubli.
//
//--------------------------------------------------------------------------------------
//   ENTRE PAR      MIDI.read(), vide par la boucle principale
//                  les callbacks HandleNoteOn/Off, HandleClock/Start/Stop/Continue
//
//   SORT VERS      SetMuxTrigOne() + AddDoutTrig()   les voix, sur note recue
//                  CountPPQN()                       l'horloge, en synchro esclave
//
//--------------------------------------------------------------------------------------
//   AVANT D'Y TOUCHER
//
//   1. LE THRU LOGICIEL EST COUPE VOLONTAIREMENT. La bibliotheque renvoie par defaut
//      vers la SORTIE tout ce qui arrive a l'entree, horloge comprise : un appareil
//      chaine derriere recevait 48 impulsions par noire au lieu de 24.
//
//   2. MidiTrigOn() N'ARME PAS, IL ACCUMULE. Les declenchements s'empilent dans
//      midiTrigPending et partent ensemble au MidiTrigFlush() de la boucle. Sans ca,
//      un accord de trois futs s'etalait sur trois tours de boucle.
//
//   3. HandleClock() TIRE SES QUATRE SOUS-TICKS D'UN COUP, et c'est un defaut connu :
//      les retards de swing, qui se comptent en ticks de 96 ppqn, s'ecrasent alors au
//      1/24 de noire. En esclave, plusieurs crans de shuffle sonnent pareil.
//
//======================================================================================

/////////////////////Function//////////////////////

//Remettre a plat les drapeaux de transport recus
void  InitMidiRealTime()
{
  midiStart = LOW;
  midiStop = LOW;
  midiContinue = LOW;
}

//Envoyer la note EXT en cours, et RETENIR ce qu'on a envoye----------------------------
//Les deux sites qui jouent la piste EXT - le tick du sequenceur et la frappe en mode TAP -
//faisaient chacun leur note-on puis posaient midiNoteOnActive a la main. Un seul endroit
//desormais : oublier de retenir la hauteur redonnerait exactement le bug que ce couple
//vient de fermer.
void MidiExtNoteOn(byte pitch, byte velocity)
{
  midiNoteOnPitch = pitch;
  midiNoteOnActive = TRUE;
  MidiSendNoteOn(seq.TXchannel, pitch, velocity);
}

//Couper la note EXT en cours : a l'arret, au changement de pattern, au rebouclage de la
//pile de notes, et sous un mute.
//
//ELLE SE RETIENT, ELLE NE SE RECALCULE PLUS. Le code d'origine reconstruisait la hauteur
//depuis extNote[noteIndexCpt - 1] - la note SUIVANTE moins un - et depuis extNote[] tel
//qu'il est MAINTENANT. Trois chemins ont le droit de deplacer l'un ou l'autre entre le
//note-on et le note-off :
//  - PLAY remet noteIndexCpt a zero. Appuyer sur PLAY pendant qu'une note sonne envoyait
//    donc le note-off sur extNote[extLength] au lieu de la bonne : la note en cours
//    restait en l'air, DEFINITIVEMENT. Quelques PLAY d'affilee et le synthe en face
//    accumule les notes coincees jusqu'a ne plus rien jouer.
//  - le coller de pattern reecrit extNote[] sous la note qui sonne.
//  - le mode clavier y ecrit note par note.
//Un octet retenu, et les trois disparaissent d'un coup.
//
//La copie sous interruptions fermees parce que les deux champs sont UN SEUL FAIT, ecrit
//par le tick comme par la boucle principale : les lire en deux temps laisserait la
//fenetre ou l'on relache la hauteur d'une note qui n'est plus celle qui sonne.
void InitMidiNoteOff()
{
  byte sreg = SREG;
  cli();
  boolean active = midiNoteOnActive;
  byte pitch = midiNoteOnPitch;
  midiNoteOnActive = FALSE;
  SREG = sreg;

  if (active) MidiSendNoteOff(seq.TXchannel, pitch);
}

//Envoyer un note-off
void MidiSendNoteOff (byte channel, byte value)
{
  MIDI.sendNoteOff(value + 12, 0, channel);
}
//Envoyer un note-on
void MidiSendNoteOn (byte channel, byte value, byte velocity)
{
  MIDI.sendNoteOn(value + 12, velocity, channel);
}

//L'horloge recue, en synchro esclave
//AUCUN SOUS-TICK ICI : LES QUATRE VIENNENT DE TIMER1. Cette fonction ne fait plus que
//deux choses - mesurer la cadence du maitre, et deposer quatre credits.
//
//La v2.055 tirait le premier sous-tick ICI, pour caler la phase sur chaque impulsion. Ca
//marchait, mais ca mettait LE PLUS IMPORTANT DES QUATRE dans la boucle principale : un pas
//non swingue part quand ppqn % scale vaut zero, et tous ces instants-la sont des debuts de
//groupe. Les temps forts portaient donc la gigue de la boucle - de l'ordre de 0,36 ms,
//davantage sous un redessin d'ecran - pendant que les contretemps swingues, tires par
//l'interruption, etaient parfaitement droits. Exactement l'inverse de ce qu'on veut :
//un temps fort doit etre droit, un contretemps peut se promener.
//
//Le timer tourne donc librement a la cadence mesuree et place les quatre. La phase n'est
//posee qu'une fois, au depart ; ensuite c'est le quartz qui la tient, et non l'instant ou
//la boucle a remarque l'octet. Le compte exact de quatre par impulsion est garanti par les
//credits : quatre deposes ici, un depense par tick.
//
//CE QU'ON PERD : la phase n'est plus recalee sur chaque impulsion, donc un decalage
//constant peut s'installer contre le maitre - borne par le credit, qui se vide si l'on
//prend de l'avance et sature si l'on prend du retard. Un decalage constant ne s'entend
//pas, une gigue si.
//Un delayMicroseconds(2000) trainait entre le deuxieme et le troisieme appel, du temps ou
//l'interruption du sequenceur tenait les sorties de trig avec sa propre attente active.
//C'est Timer2 qui porte l'impulsion depuis, et cette attente est partie la premiere.
//zabox est arrive a la meme conclusion sur la NAVA et l'a mise en commentaire la-bas.
//La sortie DIN n'est plus levee ici : CountPPQN() la mene comme en maitre, front montant
//sur le premier sous-tick du groupe et descendant sur le troisieme. Elle ne faisait ici
//qu'une pointe de la duree des quatre appels, la ou le maitre rend un vrai carre.
void HandleClock()
{
  //L'intervalle entre les deux dernieres impulsions, qui sert de cadence au timer.
  unsigned long now = micros();
  static unsigned long prevClock = 0;
  unsigned long dt = now - prevClock;
  prevClock = now;

  SlaveBpmMeasure(dt);  //le tempo du maitre, pour l'ecran
  SlaveClockRate(dt);   //la cadence du timer, qui tourne librement

  //QUATRE CREDITS, PLAFONNES. L'interruption en depense un par tick et ne fait rien quand
  //la caisse est vide : le compte de quatre par impulsion est donc exact sans que personne
  //ait a compter les ticks. Timer un peu rapide, la caisse se vide et il attend ; timer un
  //peu lent, elle se remplit et il rattrape. Le plafond de deux impulsions evite qu'un
  //maitre relance apres un silence ne fasse partir une rafale.
  //L'interruption ecrit slaveTicks, donc la lecture-modification-ecriture doit etre entiere.
  uint8_t sreg = SREG;
  cli();
  byte t = (byte)(slaveTicks + 4);
  slaveTicks = (t > SLAVE_TICK_MAX) ? (byte)SLAVE_TICK_MAX : t;
  SREG = sreg;
}

//Start recu
void HandleStart()
{
  midiStart = HIGH;
}

//Stop recu
void HandleStop()
{
  midiStop = HIGH;
}

//Continue recu
void HandleContinue()
{
  midiContinue = HIGH;
}

//Debrancher les rappels de transport, quand la machine redevient maitre
void DisconnectMidiHandleRealTime()
{
  MIDI.disconnectCallbackFromType(midi::Clock);
  MIDI.disconnectCallbackFromType(midi::Start);
  MIDI.disconnectCallbackFromType(midi::Stop);
  MIDI.disconnectCallbackFromType(midi::Continue);
}

//Quelle voix repond a quelle note----------------------------------------------------
//La correspondance etait ecrite TROIS fois en switch : une pour le note-on, une pour le
//note-off, et une pour le note-on a velocite nulle - qui est un note-off deguise. Les
//deux dernieres etaient identiques au caractere pres, cinquante lignes chacune. Ajouter
//une note ou corriger un numero demandait trois retouches concordantes ; c'est la meme
//famille de piege que les douze copies du recalage de piste.
//
//Table en PROGMEM indexee par (note - 34), donc rien en RAM. Les trous sont les notes
//General MIDI que le Yocto n'a pas.
#define MIDI_NOTE_FIRST 34
#define MIDI_NOTE_LAST  55
#define MIDI_NOTE_TR1   60
#define MIDI_NO_INST    0xFF

static const byte midiNoteInst[] PROGMEM = {
  RM,           //34  Rimshot
  BD, BD,       //35  Acoustic Bass Drum / 36 Bass Drum 1
  MIDI_NO_INST, //37  Side Stick
  SD,           //38  Acoustic Snare
  HC,           //39  Hand Clap
  SD,           //40  Electric Snare
  LT,           //41  Low Floor Tom
  CH,           //42  Closed Hi Hat
  MIDI_NO_INST, //43  High Floor Tom
  MIDI_NO_INST, //44  Pedal Hi Hat
  MT,           //45  Low Tom
  OH,           //46  Open Hi Hat
  MT, MT,       //47  Low-Mid Tom / 48 Hi-Mid Tom
  CB,           //49  Crash Cymbal 1
  HT,           //50  High Tom
  CY,           //51  Ride Cymbal 1
  MIDI_NO_INST, //52  Chinese Cymbal
  PC2, PC2, PC2 //53 Ride Bell / 54 Tambourine / 55 Splash Cymbal
};

static byte MidiNoteToInst(byte pitch)
{
  if (pitch < MIDI_NOTE_FIRST || pitch > MIDI_NOTE_LAST) return MIDI_NO_INST;
  return pgm_read_byte(&midiNoteInst[pitch - MIDI_NOTE_FIRST]);
}

//Note recue----------------------------------------------------------------------------
//La note 60 ne declenche pas une voix : elle tient TRIG-1 comme une porte, basse a
//l'appui et haute au relacher - cette sortie est active a l'etat bas, cf Clock.ino.
//SetTrigTime() sait ne pas la relacher lui-meme, cf le commentaire de Trig.ino.
void HandleNoteOn(byte channel, byte pitch, byte velocity)
{
  if (channel != seq.RXchannel) return;
  //Une note ON a velocite nulle EST un note-off : la norme l'autorise et beaucoup de
  //claviers n'envoient que ca. Le traitement etait recopie ; il est delegue.
  if (velocity == 0){
    HandleNoteOff(channel, pitch, velocity);
    return;
  }
  if (pitch == MIDI_NOTE_TR1){
    TR1_LOW;
    return;
  }
  byte inst = MidiNoteToInst(pitch);
  if (inst != MIDI_NO_INST) MidiTrigOn(inst, velocity);
}

//Note relachee-------------------------------------------------------------------------
//La velocite de relacher n'est pas lue : elle fait partie de la signature du rappel.
void HandleNoteOff(byte channel, byte pitch, byte velocity)
{
  (void)velocity;
  if (channel != seq.RXchannel) return;
  if (pitch == MIDI_NOTE_TR1){
    TR1_HIGH;
    return;
  }
  byte inst = MidiNoteToInst(pitch);
  if (inst != MIDI_NO_INST) MidiTrigOff(inst);
}

//Declencher une voix analogique depuis une note MIDI recue----------------------------
//Trois defauts tenaient dans ces cinq lignes :
//  1. SetMuxTrigMidi() balayait les douze adresses du multiplexeur pour poser UNE
//     velocite. SetMuxTrigOne() n'ecrit que la bonne.
//  2. delayMicroseconds(1000) tenait l'impulsion par attente active DANS la boucle
//     principale. Timer2 la porte, comme pour le sequenceur depuis la correction de
//     TRIG-1 : on arme, on rend la main, l'interruption relache.
//  3. SetDoutTrig(1 << inst) REMPLACAIT le masque : une note arrivant pendant
//     l'impulsion du sequenceur tronquait son declenchement. On accumule desormais.
//
//Et le declenchement lui-meme est differe : MIDI.read() ne rend qu'UN message par
//appel, donc BD + SD + CY sur le meme pas arrivaient en trois messages traites a trois
//tours de boucle d'ecart. Les bits s'empilent ici, la boucle vide la file puis appelle
//MidiTrigFlush() une fois - un accord MIDI redevient aussi simultane qu'un pas du
//sequenceur, qui a toujours ecrit son masque complet en une fois.
void MidiTrigOn(byte inst, byte velocity)
{
  if (instWasMidiTrigged[inst] == FALSE){
    SetMuxTrigOne(inst, velocity);
    midiTrigPending |= (1 << inst);
    instWasMidiTrigged[inst] = TRUE;
  }
}

//Declencher d'un coup tout ce que la salve a arme-------------------------------------
//Appele par la boucle principale une fois la file MIDI videe. Rien a faire si rien
//n'attend : le cas courant est celui-la, et il ne coute qu'un test.
void MidiTrigFlush()
{
  if (!midiTrigPending) return;

  //Lever les bits et armer le compteur sont un seul geste, interruptions fermees.
  //Deux facons pour Timer2 de venir tout defaire entre les deux :
  //
  //  - son interruption tombe PENDANT la sequence. ResetDoutTrig() rabat alors les bits
  //    qu'on vient de lever, et StartTrigPulseIfIdle() voit un compteur arrete et arme
  //    2 ms sur un registre deja vide. La voix n'a eu qu'une microseconde d'etat haut au
  //    lieu des cent que le front demande : note faible, ou muette. Quelques
  //    microseconde de fenetre sur 2 ms, donc rare - le genre de note manquante qu'on
  //    met sur le compte du clavier. Le cli() ci-dessous ferme ce cas.
  //
  //  - son echeance etait DEJA tombee en arrivant ici, son interruption attendant d'etre
  //    servie. Le cli() ne l'annule pas : elle partirait a la restauration de SREG, apres
  //    notre travail, et l'effacerait. On fait donc son travail a sa place, avant de
  //    lever quoi que ce soit - c'est exactement le corps de ISR(TIMER2_COMPA_vect),
  //    ecrire 1 sur OCF2A effacant le drapeau en attente.
  //
  //Le drapeau ne peut etre leve que par une impulsion reellement echue : StartTrigPulse()
  //l'efface avant d'armer, et compteur a l'arret il ne peut pas se lever tout seul.
  byte sreg = SREG;
  cli();
  if (TIFR2 & _BV(OCF2A)){
    TCCR2B = 0;
    TIMSK2 = 0;
    TIFR2  = _BV(OCF2A);
    ResetDoutTrig();
  }
  AddDoutTrig(midiTrigPending);
  midiTrigPending = 0;
  StartTrigPulseIfIdle();//Timer2 relache les sorties ; cf le commentaire de la fonction
  SREG = sreg;
}

//Relacher le verrou d'une voix : la note suivante pourra la redeclencher
void MidiTrigOff(byte inst)
{
  instWasMidiTrigged[inst] = FALSE;
}

//Tout couper, dehors ET dedans---------------------------------------------------------
//Appelee au STOP, et sur un Stop ou un Continue recu.
//instWasMidiTrigged[] n'etait remis a FALSE que par le note-off de la note EXACTE. Un
//note-off perdu - cable debranche, clavier remis a zero, message avale par un hub -
//laissait donc le drapeau leve, et MidiTrigOn() rendait la main aussitot : la voix
//devenait SOURDE AU MIDI, definitivement, jusqu'a extinction de la machine. Rien nulle
//part ne remettait ces seize drapeaux a zero.
//STOP est le geste de panique naturel et cette fonction porte deja ce nom : elle libere
//donc aussi les verrous locaux, et rend TRIG-1 au repos si une note 60 le tenait.
void SendAllNoteOff()
{
  //La note EXT etait confiee au SEUL CC 123, que beaucoup de machines ignorent - une
  //x0xb0x parmi elles. Un note-off explicite d'abord, le CC ensuite pour le reste : on
  //ne demande pas poliment de couper ce qu'on sait couper soi-meme. Ca remet aussi
  //midiNoteOnActive a zero, qui restait leve apres un STOP et faisait partir un note-off
  //perime au pas EXT suivant.
  InitMidiNoteOff();
  MIDI.sendControlChange(ALL_NOTE_OFF , 0, seq.TXchannel);
  for (byte z = 0; z < NBR_INST; z++) instWasMidiTrigged[z] = FALSE;
  TR1_HIGH;
}












