//-------------------------------------------------
//                  Yocto v2.x
//              Midi Function
//-------------------------------------------------

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
  /* MidiSend(channel + 0x80);
   MidiSend(value);
   MidiSend(0x00);*/
}
//Send note ON
void MidiSendNoteOn (byte channel, byte value, byte velocity)
{
  MIDI.sendNoteOn(value + 12, velocity, channel);
  /* MidiSend(channel + 0x90);
   MidiSend(value);
   MidiSend(velocity);*/
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
        //Serial.println(velocity);
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
  AddDoutTrig(midiTrigPending);
  midiTrigPending = 0;
  StartTrigPulseIfIdle();//Timer2 relache les sorties ; cf le commentaire de la fonction
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












