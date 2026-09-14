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

//initialize midi real time variable
void  InitMidiRealTime()
{
  midiStart = LOW;
  midiStop = LOW;
  midiContinue = LOW;
}

//intialize note off when stop or patternChanged  or ext instrument loop back
void InitMidiNoteOff()
{
  if(midiNoteOnActive){
    midiNoteOnActive = FALSE;
    if (noteIndexCpt) MidiSendNoteOff(seq.TXchannel, pattern[ptrnBuffer].extNote[noteIndexCpt - 1]);
    else MidiSendNoteOff(seq.TXchannel, pattern[ptrnBuffer].extNote[pattern[ptrnBuffer].extLength]);
  }
}

//Send note OFF
void MidiSendNoteOff (byte channel, byte value)
{
  MIDI.sendNoteOff(value + 12, 0, channel);
}
//Send note ON
void MidiSendNoteOn (byte channel, byte value, byte velocity)
{
  MIDI.sendNoteOn(value + 12, velocity, channel);
}

//Handle clock
void HandleClock()
{
  DIN_CLK_HIGH;
  //A delayMicroseconds(2000) used to sit between the second and third call. It was there
  //to let the trigger pulse finish before the next tick could retrigger, back when the
  //sequencer interrupt held the outputs with its own 2 ms busy wait. Timer2 carries the
  //pulse now, so the wait had nothing left to wait for - it only blocked the main loop
  //for 2 ms on every incoming MIDI clock, which is 48 ms per second at 120 BPM.
  //zabox reached the same conclusion on the NAVA and commented it out there.
  //The four back to back calls are still wrong - the four sub ticks ought to be spread
  //over the incoming clock period rather than fired at once - but that is a rewrite,
  //noted separately in TODO.md.
  CountPPQN();//execute 4x because internal sequencer run as 96 ppqn
  CountPPQN();
  CountPPQN();
  CountPPQN();
  DIN_CLK_LOW;

}

//handle start
void HandleStart()
{
  midiStart = HIGH;
}

//handle stop
void HandleStop()
{
  midiStop = HIGH;
}

//handle Continue
void HandleContinue()
{
  midiContinue = HIGH;
}

//Disconned midi real time callback
void DisconnectMidiHandleRealTime()
{
  MIDI.disconnectCallbackFromType(midi::Clock);
  MIDI.disconnectCallbackFromType(midi::Start);
  MIDI.disconnectCallbackFromType(midi::Stop);
  MIDI.disconnectCallbackFromType(midi::Continue);
}

//Handle noteON
void HandleNoteOn(byte channel, byte pitch, byte velocity)
{
  //Midi note On with 0 velocity as Midi note Off
  if (velocity == 0){
    if (channel == seq.RXchannel){
      switch (pitch){
      case 35:
      case 36:
        MidiTrigOff(BD);
        break;
      case 38:
      case 40:
        MidiTrigOff(SD);
        break;
      case 41:
        MidiTrigOff(LT);
        break;
      case 45:
      case 47:
      case 48:
        MidiTrigOff(MT);
        break;
      case 50:
        MidiTrigOff(HT);
        break;
      case 34:
        MidiTrigOff(RM);
        break;
      case 39:
        MidiTrigOff(HC);
        break;
      case 42:
        MidiTrigOff(CH);
        break;
      case 46:
        MidiTrigOff(OH);
        break;
      case 51:
        MidiTrigOff(CY);
        break;
      case 49:
        MidiTrigOff(CB);
        break;
      case 55:
      case 54:
      case 53:
        MidiTrigOff(PC2);
        break;
      case 60:
        TR1_HIGH;
        break;
      }
    }
  }
  else{
    if (channel == seq.RXchannel){
      switch (pitch){
      case 35:
      case 36:
        MidiTrigOn(BD, velocity);
        break;
      case 38:
      case 40:
        MidiTrigOn(SD, velocity);
        break;
      case 41:
        MidiTrigOn(LT, velocity);
        break;
      case 45:
      case 47:
      case 48:
        MidiTrigOn(MT, velocity);
        break;
      case 50:
        MidiTrigOn(HT, velocity);
        break;
      case 34:
        MidiTrigOn(RM, velocity);
        break;
      case 39:
        MidiTrigOn(HC, velocity);
        break;
      case 42:
        MidiTrigOn(CH, velocity);
        break;
      case 46:
        MidiTrigOn(OH, velocity);
        break;
      case 51:
        MidiTrigOn(CY, velocity);
        break;
      case 49:
        MidiTrigOn(CB, velocity);
        break;
      case 55:
      case 54:
      case 53:
        MidiTrigOn(PC2, velocity);
        break;
      case 60:
        TR1_LOW;
        break;
      }
    }
  }
}

//Handle noteOFF
void HandleNoteOff(byte channel, byte pitch, byte velocity)
{
  if (channel == seq.RXchannel){
    switch (pitch){
    case 35:
    case 36:
      MidiTrigOff(BD);
      break;
    case 38:
    case 40:
      MidiTrigOff(SD);
      break;
    case 41:
      MidiTrigOff(LT);
      break;
    case 45:
    case 47:
    case 48:
      MidiTrigOff(MT);
      break;
    case 50:
      MidiTrigOff(HT);
      break;
    case 34:
      MidiTrigOff(RM);
      break;
    case 39:
      MidiTrigOff(HC);
      break;
    case 42:
      MidiTrigOff(CH);
      break;
    case 46:
      MidiTrigOff(OH);
      break;
    case 51:
      MidiTrigOff(CY);
      break;
    case 49:
      MidiTrigOff(CB);
      break;
    case 55:
    case 54:
    case 53:
      MidiTrigOff(PC2);
      break;
    case 60:
      TR1_HIGH;
      break;
    }
  }   
}


//MidiTrigOn insturment
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

//MidiTrigOff insturment
void MidiTrigOff(byte inst)
{
  instWasMidiTrigged[inst] = FALSE;
}

//Midi send all note off
void SendAllNoteOff()
{
  MIDI.sendControlChange(ALL_NOTE_OFF , 0, seq.TXchannel);	
}












