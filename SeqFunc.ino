//======================================================================================
//
//   S E Q F U N C . I N O                      les operations sur un pattern
//
//   Ce que Seq.ino declenche, ce fichier l'execute : copier, coller, decaler, changer
//   de direction, avancer une chaine, recaler la piste, basculer le buffer de pattern.
//   Des verbes, pas des gestes - aucune touche n'est lue ici.
//
//--------------------------------------------------------------------------------------
//   AVANT D'Y TOUCHER
//
//   1. SwitchPatternBuffer() NE PEUT PAS ALLER DANS L'INTERRUPTION. Elle envoie du MIDI
//      et reconstruit des champs : c'est ce qui interdit de precharger le pattern
//      suivant depuis le tick.
//
//   2. TrackPosToPattern() EST L'UNIQUE RECALAGE piste -> pattern. Ces deux lignes
//      etaient ecrites DOUZE fois, et une copie manquait : TRACK EDIT affichait apres
//      un cycle d'allumage le pattern charge au boot au lieu de celui de la piste.
//      Ne pas en faire une treizieme copie.
//
//======================================================================================

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

  //extLength EST L'INDEX DE LA DERNIERE NOTE, pas un compte - quatre sites le disent :
  //LCD.ino affiche extLength + 1, key.ino pose extLength = noteIndex - 1 en ajoutant une
  //note, Clock.ino joue bien l'index extLength avant de reboucler, et le note-off de
  //Midi.ino va chercher extNote[extLength]. Une pile de cinq notes a donc extLength = 4.
  //La borne etait stricte : la copie s'arretait a l'avant-derniere note et la derniere
  //gardait ce que le pattern de destination avait a cette place. C'est celle qui ferme
  //la phrase, donc celle qu'on entend le mieux.
  //extNote[] compte 128 entrees et extLength est borne a MAX_EXT_INST_NOTE = 99 : la
  //borne inclusive ne peut pas deborder.
  for ( byte j = 0; j <= pattern[ptrnBuffer].extLength; j++){
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

  //Borne inclusive, cf CopyPatternToBuffer() : extLength est l'index de la derniere note.
  for ( byte j = 0; j <= bufferedPattern.extLength; j++){
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

//Changer la direction de lecture------------------------------------------------------
//SHIFT + Shift Right fait tourner forward > backward > ping-pong > random.
//SHIFT + INST + Shift Right ne change que l'instrument courant ; sans INST, la valeur
//obtenue est recopiee sur les seize.
//
//patternWasEdited MANQUAIT. dir[] est pourtant bien ecrit en EEPROM par SavePattern() et
//relu par LoadPattern() - le chemin de donnees etait complet, seul le drapeau sautait.
//Consequence : la LED ENTER ne clignotait pas, maintenir ENTER n'enregistrait rien, et
//le prochain chargement du pattern ecrasait le changement. Le manuel est explicite juste
//au-dessus du passage : "Enter led is blinking to indicate that pattern was edited. To
//save it press Enter".
//Meme defaut, meme forme que le drapeau de piste efface en quittant TRACK par la touche
//PATT : une donnee bien modifiee en RAM, un drapeau absent, et l'ecran qui ment.
void ChangePatternDirection()
{
  if (instBtn){
    //Change current instrument direction
    if (pattern[ptrnBuffer].dir[curInst]++ >= MAX_SEQ_DIR) pattern[ptrnBuffer].dir[curInst] = FORWARD;
  }
  else {
    if (pattern[ptrnBuffer].dir[curInst]++ >= MAX_SEQ_DIR) pattern[ptrnBuffer].dir[curInst] = FORWARD;
    byte dir = pattern[ptrnBuffer].dir[curInst];
    for (byte z = 0; z < NBR_INST; z++){
      pattern[ptrnBuffer].dir[z] = dir;
    }
  }
  patternWasEdited = TRUE;
  needLcdUpdate = TRUE;
}

///////////////////////shift left pattern///////////////////////////////
void ShiftLeftPattern()
{

  patternWasEdited = TRUE;

  //Le modificateur est INST, pas SHIFT - et c'est ce qu'il faut lire ici. Le test
  //portait aussi "&& !shiftBtn.pressed", condition MORTE : l'appelant est deja dans la
  //branche "SHIFT relache" de SeqParameter(). Elle ne servait a rien sauf a laisser
  //croire que SHIFT jouait un role dans ce geste, ce qui envoie chercher la mauvaise
  //touche. Le manuel dit bien INST : "+ + => Change selected instrument direction".
  //only shift selected instruments
  if (instBtn){
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

  //Meme remarque que dans ShiftLeftPattern() : le modificateur est INST, et le
  //"&& !shiftBtn.pressed" etait une condition morte qui induisait en erreur.
  //shift right only selected instrument
  if (instBtn){
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
  //patternNbr[] SORT DE L'EEPROM ET N'EST VALIDE PAR PERSONNE. LoadTrack() borne depuis
  //le 14/09 la LONGUEUR de la piste, pas son CONTENU. Un octet de zone vierge vaut 0xFF,
  //soit 255, alors que MAX_PTRN vaut 128 - et LoadPattern() prend un byte sans rien
  //verifier : l'adresse calculee sortait de la zone des patterns pour aller lire dans
  //celle des pistes. Lecture seule, donc rien n'est detruit, mais ca joue du bruit.
  //C'est le danger que InitPattern() decrit deja pour group.firstPattern - il avait ete
  //bouche la, et pas ici, alors que LoadPattern() n'a que deux appelants et que celui-ci
  //est le seul qui puisse lui passer n'importe quoi.
  //Zero plutot qu'un ecretage a MAX_PTRN - 1 : un numero impossible ne designe aucun
  //pattern, et ecreter fabriquerait une donnee plausible a partir d'une donnee fausse.
  //Meme raisonnement que la longueur de piste remise a zero dans LoadTrack().
  byte ptrn = track[trkBuffer].patternNbr[trk.pos];
  nextPattern = (ptrn < MAX_PTRN) ? ptrn : 0;
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
  //LA PILE DE NOTES REPART DU DEBUT, ELLE AUSSI. noteIndexCpt - le compteur de LECTURE,
  //a ne pas confondre avec noteIndex juste au-dessus qui est le curseur d'EDITION -
  //etait le seul etat de la piste EXT a survivre au changement de pattern. Or extNote[]
  //et extLength appartiennent AU PATTERN : continuer un index dans une autre pile ne
  //veut rien dire, l'index 8 d'une pile de 10 n'ayant aucun rapport avec l'index 8 d'une
  //pile de 7.
  //En jeu ca donnait exactement ce qui a ete signale : aller du pattern 1 (7 notes) au
  //pattern 2 (10 notes) et revenir laissait la pile du 1 decalee d'un montant arbitraire
  //- celui de ce qu'avait consomme le 2. Un pattern ne sonnait plus pareil d'une fois
  //sur l'autre.
  //LA DERIVE CONTRE LA GRILLE N'EST PAS TOUCHEE, et c'est ce qui compte : elle se produit
  //A L'INTERIEUR d'un pattern qui boucle, et cette fonction n'est appelee que si le
  //numero change vraiment (cf selectedPatternChanged). Une pile de cinq notes sur seize
  //pas continue donc de tourner sur quatre mesures, comme avant.
  //Un octet, donc une ecriture atomique sur AVR : rien a fermer.
  noteIndexCpt = 0;
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
