//-------------------------------------------------
//                  Yocto v2.x
//                 SEQ Functions
//-------------------------------------------------

/////////////////////Function//////////////////////
//Initialyse the sequencer before to run
void InitSeq()
{
  LoadSeqSetup();
  ppqn = 0;
  globalStepCount = 0;
  seq.configPage = 0;
  seq.configMode = FALSE;
  randomSeed(analogRead(0));
  seq.dir = FORWARD;
  seq.bpm = seq.defaultBpm;

  switch (seq.sync){
  case MASTER:
    DisconnectMidiHandleRealTime();
    TimerStart();//cf timer
    break;
  case SLAVE:
    TimerStop();
    MIDI.setHandleClock(HandleClock);
    MIDI.setHandleStart(HandleStart);
    MIDI.setHandleStop(HandleStop);
    MIDI.setHandleContinue(HandleContinue);
    break;
  }
}

//copy pattern to buffer
void CopyPatternToBuffer()
{
  for (byte i = 0; i < NBR_INST; i++){ 
    bufferedPattern.inst[i] =  pattern[ptrnBuffer].inst[i] ;
    bufferedPattern.length[i] =  pattern[ptrnBuffer].length[i] ;
    bufferedPattern.scale[i] =  pattern[ptrnBuffer].scale[i] ;
    bufferedPattern.shuffle[i] =  pattern[ptrnBuffer].shuffle[i] ;
    bufferedPattern.dir[i] =  pattern[ptrnBuffer].dir[i] ;
    for (byte j = 0; j < NBR_STEP; j++){
      bufferedPattern.velocity[i][j] = pattern[ptrnBuffer].velocity[i][j]; 
      bufferedPattern.probality[i][j] = pattern[ptrnBuffer].probality[i][j];
      bufferedPattern.uTiming[i][j] = pattern[ptrnBuffer].uTiming[i][j]; 
      bufferedPattern.retrig[i][j] = pattern[ptrnBuffer].retrig[i][j];
    }
  }
  bufferedPattern.globalLength = pattern[ptrnBuffer].globalLength;
  bufferedPattern.globalScale = pattern[ptrnBuffer].globalScale;
  bufferedPattern.globalShuffle = pattern[ptrnBuffer].globalShuffle;
  bufferedPattern.globalDir = pattern[ptrnBuffer].globalDir;
  bufferedPattern.groupPos = pattern[ptrnBuffer].groupPos;
  bufferedPattern.groupLength = pattern[ptrnBuffer].groupLength;
  bufferedPattern.extLength = pattern[ptrnBuffer].extLength;
  bufferedPattern.globalAcc = pattern[ptrnBuffer].globalAcc;

  for ( byte j = 0; j < pattern[ptrnBuffer].extLength; j++){
    bufferedPattern.extNote[j] = pattern[ptrnBuffer].extNote[j];
  }
}

//paste buffer to current pattern
void PasteBufferToPattern()
{
  for (byte i = 0; i < NBR_INST; i++){ 
    pattern[ptrnBuffer].inst[i] =  bufferedPattern.inst[i];
    pattern[ptrnBuffer].length[i] =   bufferedPattern.length[i] ;
    pattern[ptrnBuffer].scale[i] =   bufferedPattern.scale[i] ;
    pattern[ptrnBuffer].shuffle[i] =   bufferedPattern.shuffle[i] ;
    pattern[ptrnBuffer].dir[i] =   bufferedPattern.dir[i] ;
    for (byte j = 0; j < NBR_STEP; j++){
      pattern[ptrnBuffer].velocity[i][j] = bufferedPattern.velocity[i][j]; 
      pattern[ptrnBuffer].probality[i][j] =  bufferedPattern.probality[i][j];
      pattern[ptrnBuffer].uTiming[i][j] =  bufferedPattern.uTiming[i][j]; 
      pattern[ptrnBuffer].retrig[i][j] =  bufferedPattern.retrig[i][j]; 
    }
  }
  pattern[ptrnBuffer].globalLength =  bufferedPattern.globalLength;
  pattern[ptrnBuffer].globalScale =  bufferedPattern.globalScale;
  pattern[ptrnBuffer].globalShuffle =  bufferedPattern.globalShuffle;
  pattern[ptrnBuffer].globalDir =  bufferedPattern.globalDir;
  pattern[ptrnBuffer].groupPos =  bufferedPattern.groupPos;
  pattern[ptrnBuffer].groupLength =  bufferedPattern.groupLength;
  pattern[ptrnBuffer].extLength = bufferedPattern.extLength;
  pattern[ptrnBuffer].globalAcc = bufferedPattern.globalAcc;

  for ( byte j = 0; j < bufferedPattern.extLength; j++){
    pattern[ptrnBuffer].extNote[j] = bufferedPattern.extNote[j];
  }
}

//init pattern
void InitPattern()
{
  if (!group.priority){
    group.length = pattern[ptrnBuffer].groupLength;
    //groupPos est la place du pattern courant DANS sa chaine, donc une chaine qui
    //commence a curPattern - groupPos. En arithmetique byte, groupPos > curPattern
    //rebouclait vers 255 : nextPattern depassait alors 127 et LoadPattern() allait lire
    //dans la zone des pistes (TRACK_OFFSET), produisant un pattern de bruit. Lecture
    //seule, donc rien n'etait detruit - mais c'etait audible.
    //Cas atteignable : LoadPatternGroup() sait corrompre groupPos. Un groupPos plus
    //grand que le numero du pattern est de toute facon incoherent ; on retombe alors
    //sur "la chaine commence ici", qui est la lecture la plus proche du contenu.
    byte gPos = pattern[ptrnBuffer].groupPos;
    group.firstPattern = (gPos <= curPattern) ? curPattern - gPos : curPattern;
  }
  UpdateLongerInst();//the loaded pattern decides which instrument spans the measure
  /////////////////////COULD BE REMOVED/////////////////////////////
  //Init Ride, Crash velocity to HIGH_VEL
  /*for (int stp = 0; stp < NBR_STEP; stp++){
   pattern[ptrnBuffer].velocity[CH][stp] = instVelHigh[CH];//
   pattern[ptrnBuffer].velocity[OH][stp] = instVelHigh[OH];//
   pattern[ptrnBuffer].velocity[PC2][stp] = instVelHigh[PC2];//
   pattern[ptrnBuffer].velocity[GLOBAL_ACC][stp] = HIGH_VEL;//GLOBAL_ACC
   pattern[ptrnBuffer].velocity[TRIG_OUT][stp] = HIGH_VEL;//TRIG_OUT
   pattern[ptrnBuffer].velocity[EXT_INST][stp] = HIGH_VEL;//EXT_INST
   }*/
  ////////////////////////////////////////////////////////////////////
  switch (pattern[ptrnBuffer].scale[curInst]){
  case  SCALE_16:
    scaleBtn.counter = 0;
    break;
  case  SCALE_32:
    scaleBtn.counter = 1;
    break;
  case  SCALE_8t:
    scaleBtn.counter = 2;
    break;
  case  SCALE_16t:
    scaleBtn.counter = 3;
    break;
  }
}

//Convert Instrument Word to Step Word
void InstToStepWord()
{
  for (int a = 0; a < NBR_STEP; a++){
    pattern[ptrnBuffer].step[a] = 0;
    for (int b = 0; b < NBR_INST; b++){
      if (bitRead(pattern[ptrnBuffer].inst[b],a)) bitSet(pattern[ptrnBuffer].step[a],b);
    }
  }
}

///////////////////////shift left pattern///////////////////////////////
void ShiftLeftPattern()
{

  patternWasEdited = TRUE;

  //only shift selected instruments
  if (instBtn && !shiftBtn.pressed){
    //remember first step velocity value
    byte temp = pattern[ptrnBuffer].velocity[curInst][0];

    //Shift Left trig value
    if (bitRead(pattern[ptrnBuffer].inst[curInst],0))pattern[ptrnBuffer].inst[curInst] = pattern[ptrnBuffer].inst[curInst] >> 1 | (1U << pattern[ptrnBuffer].length[curInst]);
    else pattern[ptrnBuffer].inst[curInst] = pattern[ptrnBuffer].inst[curInst] >> 1;

    //shift left velocity value
    for (int stp = 0; stp <= pattern[ptrnBuffer].length[curInst]; stp++){
      if (stp == pattern[ptrnBuffer].length[curInst]) pattern[ptrnBuffer].velocity[curInst][stp] = temp;
      else pattern[ptrnBuffer].velocity[curInst][stp] = pattern[ptrnBuffer].velocity[curInst][stp+1];
    }
  }
  // shift full pattern
  else{
    byte temp[NBR_INST];
    //Loop as many instrument
    for (int nbrInst = 0; nbrInst < NBR_INST; nbrInst++){
      //remember first step velocity value of each instrument
      temp[nbrInst] = pattern[ptrnBuffer].velocity[nbrInst][0];

      //Shift Left trig value of all instrument
      if (bitRead(pattern[ptrnBuffer].inst[nbrInst],0))pattern[ptrnBuffer].inst[nbrInst] = pattern[ptrnBuffer].inst[nbrInst] >> 1 | (1U << pattern[ptrnBuffer].length[nbrInst]);
      else pattern[ptrnBuffer].inst[nbrInst] = pattern[ptrnBuffer].inst[nbrInst] >> 1;

      //shift left velocity value of all instrument
      for (int stp = 0; stp <= pattern[ptrnBuffer].length[nbrInst]; stp++){
        if (stp == pattern[ptrnBuffer].length[nbrInst]) pattern[ptrnBuffer].velocity[nbrInst][stp] = temp[nbrInst];
        else pattern[ptrnBuffer].velocity[nbrInst][stp] = pattern[ptrnBuffer].velocity[nbrInst][stp + 1];      
      }
    }
  }
}

///////////////////////////Shift Right pattern/////////////////////////////////////////
void ShiftRightPattern()
{
  patternWasEdited = TRUE; 

  //shift right only selected instrument
  if (instBtn && !shiftBtn.pressed){
    //---------------Variable---------------------
    //Instrument Trig value
    unsigned int trigValue = pattern[ptrnBuffer].inst[curInst];
    //Last step variable
    byte lastStep = pattern[ptrnBuffer].length[curInst];
    //remember last step velocity value
    byte temp = pattern[ptrnBuffer].velocity[curInst][lastStep];

    //Shift right trig value
    if (bitRead(trigValue,lastStep)){
      trigValue = trigValue << 1 | 1;
      bitClear(trigValue, lastStep + 1);
    }
    else trigValue = trigValue << 1;

    //Set new trig value
    pattern[ptrnBuffer].inst[curInst] = trigValue;

    //shift right velocity
    for (int stp = lastStep; stp >= 0; stp--){
      if (stp == 0) pattern[ptrnBuffer].velocity[curInst][stp] = temp;
      else pattern[ptrnBuffer].velocity[curInst][stp] = pattern[ptrnBuffer].velocity[curInst][stp-1];
    }
  }

  //shift right all pattern
  else{
    //---------------Variable---------------------
    //Instrument Trig value
    unsigned int trigValue[NBR_INST]; 
    //Last step variable
    byte lastStep[NBR_INST]; 
    //remember last step velocity value
    byte temp[NBR_INST]; 

    //Loop as many instrument
    for (int a = 0; a < NBR_INST; a++){
      //Update variable
      trigValue[a] = pattern[ptrnBuffer].inst[a];
      lastStep[a] = pattern[ptrnBuffer].length[a];
      temp[a] = pattern[ptrnBuffer].velocity[a][lastStep[a]];

      //Shift right full pattern
      if (bitRead(trigValue[a],lastStep[a])){
        trigValue[a] = trigValue[a] << 1 | 1;
        bitClear(trigValue[a], lastStep[a] + 1);
      }
      else trigValue[a] = trigValue[a] << 1;

      //Set news trig values
      pattern[ptrnBuffer].inst[a] = trigValue[a];

      //Shift right full pattern velocity values 
      for (int stp = lastStep[a]; stp >= 0; stp--){
        if (stp == 0) pattern[ptrnBuffer].velocity[a][stp] = temp[a];
        else pattern[ptrnBuffer].velocity[a][stp] = pattern[ptrnBuffer].velocity[a][stp-1];      
      }
    }
  }

}
//Advance the pattern chain by one measure
//Single copy shared by PTRN_STEP and PTRN_PLAY. Each mode used to carry its own
//version : one pre-incremented group.pos, the other post-incremented it, so
//switching mode in the middle of a chain repeated or skipped a pattern.
//Convention kept : group.pos always holds the index of the NEXT pattern to play.
//playMode is the mode actually playing - MUTE is only a step button overlay.
void PatternChainUpdate(SeqMode playMode)
{
  if (!trackPosNeedIncremante) return;
  trackPosNeedIncremante = FALSE;

  if (group.length){
    if (group.pos > group.length) group.pos = 0;
    //Seconde barriere : meme avec un firstPattern sain, firstPattern + pos peut sortir
    //de la zone des patterns si la longueur de chaine lue en EEPROM est aberrante.
    //On reboucle sur le debut de la chaine plutot que d'aller lire ailleurs.
    unsigned int cand = (unsigned int)group.firstPattern + group.pos;
    if (cand >= MAX_PTRN){
      group.pos = 0;
      cand = group.firstPattern;
    }
    nextPattern = (byte)cand;
    group.pos++;
    needLcdUpdate = TRUE;
  }
  //Without a chain, only PTRN_PLAY defers a pattern change to the measure boundary
  else if (playMode != PTRN_PLAY) return;

  if (curPattern != nextPattern) selectedPatternChanged = TRUE;
}

//Aligne le pattern courant sur ce que la piste contient a la position courante---------
//Ces deux lignes etaient ecrites DOUZE fois : BACK, FWD, CLEAR en edition, CLEAR en
//lecture, la molette, l'insertion, la suppression, "aller a la derniere mesure", ENTER,
//l'entree dans le mode, le changement de piste, et l'avancement en lecture.
//La copie de l'entree dans le mode a ete ajoutee le 13/09/2026 parce qu'elle MANQUAIT :
//TRACK EDIT affichait apres un cycle d'allumage le pattern charge au boot au lieu de
//celui de la piste. Une logique dupliquee dont une copie s'oublie, c'est exactement le
//schema qui avait produit les bugs de chainage. Une fonction, et le compilateur garantit
//qu'aucun site ne diverge - et qu'il n'y aura pas de treizieme copie manquante.
//needLcdUpdate reste chez l'appelant : tous les sites n'en veulent pas.
void TrackPosToPattern()
{
  nextPattern = track[trkBuffer].patternNbr[trk.pos];
  if (curPattern != nextPattern) selectedPatternChanged = TRUE;
}

//Advance the track by one measure------------------------------------------------
void TrackPlayUpdate()
{
  if (!trackPosNeedIncremante) return;
  trackPosNeedIncremante = FALSE;

  trk.pos++;
  if (trk.pos >= track[trkBuffer].length) trk.pos = 0;
  TrackPosToPattern();
  needLcdUpdate = TRUE;
}

//Swap the pattern double buffer---------------------------------------------------
//Used to exist twice : here and in the Timer1 ISR. Only the ISR copy reset the step
//counters, and it never ran - the main loop cleared nextPatternReady first. Both
//also wrote ptrnBuffer, a non atomic read-modify-write shared with the interrupt.
//One copy now, in the main loop, keeping the step counter reset.
void SwitchPatternBuffer()
{
  nextPatternReady = FALSE;
  keybOct = DEFAULT_OCT;
  noteIndex = 0;
  InitMidiNoteOff();
  //When pattern changed init all instrument step counter
  for (byte z = 0; z < NBR_INST; z++){
    stepCount[z] = 0;
  }
  ptrnBuffer = !ptrnBuffer;
  InitPattern();//SHOULD BE REMOVED WHEN EEPROM WILL BE INITIALIZED
  InstToStepWord();
}

//Realign an instrument step counter on the measure grid---------------------------
//Instrument lengths can be edited while the sequencer runs, and nothing used to
//resynchronise stepCount[]. Going 16 -> 6 -> 16 steps left the instrument at an
//arbitrary phase : it kept playing, but no longer in step with the grid drawn on
//the step buttons. Taking globalStepCount modulo the new length puts it where it
//would be if it had always run at that length, in phase with the measure.
void ResyncStepCount(byte inst)
{
  uint8_t sreg = SREG;
  cli();
  int grid = globalStepCount;
  if (grid < 0) grid = 0;
  stepCount[inst] = grid % (pattern[ptrnBuffer].length[inst] + 1);
  SREG = sreg;
}

//Find which instrument carries the pattern length--------------------------------
//longerInstLength drives globalStepCount in the ISR - therefore the end of measure,
//and the advance of chains and tracks. It used to be assigned in one single place
//(the per instrument length edit) : never on a pattern load, never on a global
//length edit. A stale value made the measure advance on another instrument's scale.
void UpdateLongerInst()
{
  byte longest = 0;
  for (byte z = 1; z < NBR_INST; z++){
    if (pattern[ptrnBuffer].length[z] > pattern[ptrnBuffer].length[longest]) longest = z;
  }
  longerInstLength = longest;
}
