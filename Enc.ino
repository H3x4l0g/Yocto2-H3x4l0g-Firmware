//-------------------------------------------------
//                  Yocto v2.x
//                 Encoder function
//-------------------------------------------------

////////////////////////Function//////////////////////

void EncGet()
{
  //Sample the encoder ONCE per loop, the branches below only consume the notch
  EncRead();

  //////////////////////////////////SHUFFLE////////////////////////////////////
  if (shufBtn.pressed && curSeqMode == PTRN_STEP){
    if (shiftBtn.pressed){
      pattern[ptrnBuffer].globalShuffle = EncGet(pattern[ptrnBuffer].globalShuffle, 1);
      if(pattern[ptrnBuffer].globalShuffle <= 1) pattern[ptrnBuffer].globalShuffle = 1;
      if(pattern[ptrnBuffer].globalShuffle >= MAX_SHUF_TYPE) pattern[ptrnBuffer].globalShuffle = MAX_SHUF_TYPE;
      static byte prevShuf;
      if (pattern[ptrnBuffer].globalShuffle != prevShuf){
        prevShuf = pattern[ptrnBuffer].globalShuffle;
        needLcdUpdate = TRUE;
        patternWasEdited = TRUE;
        for (byte z = 0; z < NBR_INST; z++){
          pattern[ptrnBuffer].shuffle[z ] = pattern[ptrnBuffer].globalShuffle;
        }
      }
    }
    else{
      pattern[ptrnBuffer].shuffle[curInst] = EncGet(pattern[ptrnBuffer].shuffle[curInst], 1);
      if(pattern[ptrnBuffer].shuffle[curInst] <= 1) pattern[ptrnBuffer].shuffle[curInst] = 1;
      if(pattern[ptrnBuffer].shuffle[curInst] >= MAX_SHUF_TYPE) pattern[ptrnBuffer].shuffle[curInst] = MAX_SHUF_TYPE;
      static byte prevShufInst[NBR_INST];
      if (pattern[ptrnBuffer].shuffle[curInst] != prevShufInst[curInst]){
        prevShufInst[curInst] = pattern[ptrnBuffer].shuffle[curInst];
        needLcdUpdate = TRUE;
        patternWasEdited = TRUE;
      }
    }
  }
  //////////////////////////////////TOTAL ACCENT////////////////////////////////////
  //Behind INST held, because the knob became the instrument selector below : ungated,
  //scrolling onto ACC trapped the knob there - the value edit ate the notch and the only
  //way out was INST + step button, which you have to know. On ACC the screen IS the
  //value (a bargraph), so the knob belongs to it once you ask for it.
  if (instBtn && curInst == TOTAL_ACC && curSeqMode == PTRN_STEP){
    pattern[ptrnBuffer].totalAcc = EncGet(pattern[ptrnBuffer].totalAcc, 1);
    pattern[ptrnBuffer].totalAcc = constrain(pattern[ptrnBuffer].totalAcc, 0, 13);
    static byte prevTotalAcc;
    if (pattern[ptrnBuffer].totalAcc != prevTotalAcc){
      prevTotalAcc = pattern[ptrnBuffer].totalAcc;
      patternWasEdited = TRUE;
      needLcdUpdate = TRUE;
    }
  }
  ///////////////////////////////////TRACK WRITE////////////////////////////////////
  else if (curSeqMode == TRACK_WRITE && !tempoBtn.pressed && !seq.configMode){
    switch (curIndex){
      //track position
    case 0://track position
      if(instBtn) trk.pos = EncGet(trk.pos, 10);
      else trk.pos = EncGet(trk.pos, 1);
      trk.pos = constrain(trk.pos, 0, MAX_TRACK_POS);
      static unsigned int prevTrkPos;
      if (trk.pos != prevTrkPos){
        prevTrkPos = trk.pos;
        nextPattern = track[trkBuffer].patternNbr[trk.pos];
        if(curPattern != nextPattern) selectedPatternChanged = TRUE;
        needLcdUpdate = TRUE;
        break;
      case 1:// track pattern
        if(instBtn) nextPattern = EncGet(nextPattern, 16);
        else nextPattern = EncGet(nextPattern, 1);
        nextPattern = constrain(nextPattern, 0, MAX_PTRN - 1);
        static unsigned int prevNextPattern;
        if (nextPattern != prevNextPattern){
          prevNextPattern = nextPattern;
          if(curPattern != nextPattern) selectedPatternChanged = TRUE;
          needLcdUpdate = TRUE;
        }
        break;
      case 2://track length
        if(instBtn) track[trkBuffer].length = EncGet(track[trkBuffer].length, 10);
        else track[trkBuffer].length = EncGet(track[trkBuffer].length, 1);
        track[trkBuffer].length = constrain(track[trkBuffer].length, 0, MAX_TRACK_POS);
        static unsigned int prevTrkLength;
        if (track[trkBuffer].length != prevTrkLength){
          prevTrkLength = track[trkBuffer].length;
          trackNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
        break;
      case 3://track number
        //toDo
        break;
      }
    }
  }

  ///////////////////////////////////KEYBOARD MODE////////////////////////////////////
  else if (keyboardMode){
    switch (curIndex){
      //track position
    case 0://external instrument note index
      if(instBtn) noteIndex = EncGet(noteIndex, 10);
      else noteIndex = EncGet(noteIndex, 1);
      noteIndex = constrain(noteIndex, 0, 99);
      static unsigned int prevNoteIndex;
      if (noteIndex != prevNoteIndex){
        prevNoteIndex = noteIndex;
        needLcdUpdate = TRUE;
        break;
      case 1://external instrument note
        if(instBtn) pattern[ptrnBuffer].extNote[noteIndex] = EncGet(pattern[ptrnBuffer].extNote[noteIndex], 12);
        else pattern[ptrnBuffer].extNote[noteIndex] = EncGet(pattern[ptrnBuffer].extNote[noteIndex], 1);
        pattern[ptrnBuffer].extNote[noteIndex] = constrain(pattern[ptrnBuffer].extNote[noteIndex], 0, 127);
        static unsigned int prevExtNote;
        if (pattern[ptrnBuffer].extNote[noteIndex] != prevExtNote){
          prevExtNote = pattern[ptrnBuffer].extNote[noteIndex];
          needLcdUpdate = TRUE;
        }
        break;
      case 2://external instrument notes stack length
        if(instBtn) pattern[ptrnBuffer].extLength = EncGet(pattern[ptrnBuffer].extLength, 10);
        else pattern[ptrnBuffer].extLength = EncGet(pattern[ptrnBuffer].extLength, 1);
        pattern[ptrnBuffer].extLength = constrain(pattern[ptrnBuffer].extLength, 0, 99);
        static unsigned int prevExtLength;
        if (pattern[ptrnBuffer].extLength != prevExtLength){
          prevExtLength = pattern[ptrnBuffer].extLength;
          patternNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
        break;
      case 3://octave
        keybOct = EncGet(keybOct, 1);
        keybOct = constrain(keybOct, 0, 7);
        static unsigned int prevKeybOct;
        if (keybOct != prevKeybOct){
          prevKeybOct = keybOct;
          needLcdUpdate = TRUE;
        }
        break;
      }
    }
  }
  ///////////////////////////////////CONFIG MODE////////////////////////////////////
  else if (seq.configMode){
    switch (curIndex){
      //track position
    case 0:
      if(seq.configPage == 1){//sync select
        seq.sync = EncGet(seq.sync, 1);
        seq.sync = constrain(seq.sync, 0, 1);
        static boolean prevSeqSync;
        if (seq.sync != prevSeqSync){
          prevSeqSync = seq.sync;
          seq.syncChanged = TRUE;
          seq.setupNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
      }
      else if(seq.configPage == 2){//pattern change sync select
        seq.ptrnChangeSync = EncGet(seq.ptrnChangeSync, 1);
        seq.ptrnChangeSync = constrain(seq.ptrnChangeSync, 0, 1);
        static boolean prevPtrnSyncChange;
        if (seq.ptrnChangeSync != prevPtrnSyncChange){
          prevPtrnSyncChange = seq.ptrnChangeSync;
          // seq.syncChanged = TRUE;
          seq.setupNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
      }
      break;
    case 1://external instrument note

      seq.defaultBpm = EncGet(seq.defaultBpm, 1);
      seq.defaultBpm = constrain(seq.defaultBpm, MIN_BPM, MAX_BPM);
      static unsigned int prevDefaultBpm;
      if (seq.defaultBpm != prevDefaultBpm){
        prevDefaultBpm = seq.defaultBpm;
        seq.setupNeedSaved = TRUE;
        needLcdUpdate = TRUE;
      }
      break;
    case 2://external instrument notes stack length

      seq.TXchannel = EncGet(seq.TXchannel, 1);
      seq.TXchannel = constrain(seq.TXchannel, 1, 16);
      static unsigned int prevTX;
      if (seq.TXchannel != prevTX){
        prevTX = seq.TXchannel;
        seq.setupNeedSaved = TRUE;
        needLcdUpdate = TRUE;
      }
      break;
    case 3://octave
      seq.RXchannel = EncGet(seq.RXchannel, 1);
      seq.RXchannel = constrain(seq.RXchannel, 1, 16);
      static unsigned int prevRX;
      if (seq.RXchannel != prevRX){
        prevRX = seq.RXchannel;
        MIDI.setInputChannel(seq.RXchannel);
        seq.setupNeedSaved = TRUE;
        needLcdUpdate = TRUE;
      }
      break;
    }
  }
  ////////////////////////////PATTERN EDIT : INSTRUMENT SELECT//////////////////////
  //The knob had nothing to do on the pattern edit screen : it fell through to the BPM
  //while the LCD read "ptr len scl ins" and showed no tempo at all. It drives the ins
  //field now - and len, scl and the whole step LED grid follow, since all three read
  //curInst. Same range as INST + step button, BD to TR2, so nothing new is reachable
  //and nothing becomes unreachable.
  //TEMPO held gives the knob back to the BPM, exactly as it already does in TRACK_WRITE
  //- and LcdUpdate() already overlays the tempo on row 1 while that button is down, so
  //the escape hatch shows itself.
  //Placed last in the chain on purpose : SHUFFLE, TOTAL ACC, keyboard and config all
  //get first claim on the notch, and what is left over is the instrument.
  else if ((curSeqMode == PTRN_STEP || curSeqMode == PTRN_TAP) && !tempoBtn.pressed){
    //Compared against curInst itself rather than a static copy of it, as the branches
    //above do : one variable instead of two that can disagree.
    byte newInst = constrain(EncGet(curInst, 1), 0, NBR_INST - 1);
    if (newInst != curInst){
      curInst = newInst;
      needLcdUpdate = TRUE;
    }
  }
  else{
    seq.bpm = EncGet(seq.bpm,1);
    if (seq.bpm <= MIN_BPM) seq.bpm = MIN_BPM;
    if (seq.bpm >= MAX_BPM) seq.bpm = MAX_BPM;
    static unsigned int curBpm;
    if( seq.bpm != curBpm)
    {
      curBpm = seq.bpm;
      TimerSetFrequency();
      if (curSeqMode == PTRN_PLAY || tempoBtn.pressed) needLcdUpdate = TRUE;
    }
  }
}


//Read the encoder and validate ONE notch per detent-----------------------
//A mechanical encoder runs a complete quadrature cycle (4 transitions)
//between two detents. Counting every transition made the edited value jump
//by 4 at each detent, so the transitions are accumulated instead and a notch
//is emitted only when the knob settles back on its detent.
void EncRead()
{
  byte thisState = digitalRead(ENC_PINB) | (digitalRead(ENC_PINA) << 1);

  if (thisState == oldStateEnc) return;

  encAccu += knobDir[thisState | (oldStateEnc << 2)];
  oldStateEnc = thisState;

  if (thisState == ENC_DETENT_STATE){
    //Contact bounce sums back to 0, only a real rotation reaches the threshold
    if (encAccu >= ENC_NOTCH_THRESHOLD) encNotch = 1;
    else if (encAccu <= -ENC_NOTCH_THRESHOLD) encNotch = -1;
    encAccu = 0;
  }
}

//Apply the pending notch to a value--------------------------------------
//The notch is consumed here, so when several parameters are candidates in
//the same loop (SHUFFLE + BPM for instance) only the first one moves.
int EncGet(int value, int dif)
{
  if (encNotch > 0) value += dif;
  else if (encNotch < 0){
    value -= dif;
    if (value < 0) value = 0;// every edited parameter is positive, no wrap around
  }
  encNotch = 0;
  return value;
}





















