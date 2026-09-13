//-------------------------------------------------
//                  Yocto v2.x
//                  BPM
//-------------------------------------------------

/////////////////////Function//////////////////////
//Un pas au hasard, sans division-------------------------------------------------------
//La direction RANDOM appelait random(0, length+1) DANS l'interruption d'horloge.
//random() de l'Arduino passe par une multiplication 32 bits puis __divmodsi4 - une
//division 32 bits logicielle - pour ramener le tirage dans l'intervalle : plusieurs
//centaines de cycles, a chaque pas, pour chaque instrument en RANDOM.
//
//Ici un xorshift 16 bits : trois decalages et trois XOR. La mise a l'echelle se fait en
//multipliant l'octet de poids faible par (maxStep + 1) et en gardant le poids fort -
//mul est une instruction du coeur AVR, deux cycles. Le biais vaut au plus 1/256 sur un
//intervalle de 16 valeurs, ce qui ne s'entend pas sur un tirage de pas.
//
//L'etat initial est fixe, comme l'etait celui de random() sans randomSeed() : la suite
//est identique d'un allumage a l'autre. C'etait deja le cas, ce n'est pas une
//regression - et pour un sequenceur ca a meme un avantage, la meme session rejoue pareil.
static unsigned int rngState = 0xACE1;

byte RandomStep(byte maxStep)
{
  rngState ^= rngState << 7;
  rngState ^= rngState >> 9;
  rngState ^= rngState << 8;
  //maxStep + 1 vaut au plus NBR_STEP = 16, l'octet au plus 255 : 4080, ca tient.
  return (byte)((((unsigned int)(rngState & 0xFF)) * ((unsigned int)maxStep + 1)) >> 8);
}

//Timer interrupt
ISR(TIMER1_COMPA_vect) { 
  CountPPQN(); 
}

//Tick proceed each pulse
void CountPPQN()
{
  //p is volatile and was re-read from SRAM on each of its ~40 uses below.
  //One read here, one write back at the end.
  byte p = ppqn;

  if (p % (PPQN/2) == 0) blinkTempo = !blinkTempo;
  if (p  % (pattern[ptrnBuffer].scale[curInst]/2) == 0) blinkFast[curInst] = LOW;
  //if (p % 4 == 0) MIDI.sendRealTime(midi::Clock);//MidiSend(CLOCK_CMD);//as NAVA seq is 96ppqn we need to send clock each 4 internal ppq

  if (p % 4 == 0) {
    //Writing UDR1 directly rather than through the library is [zabox]'s fix for the MIDI
    //clock lag, and it stays. What goes is the unbounded wait that used to guard it.
    //
    //UDRE1 is only ever set once MIDI.begin() has enabled the UART1 transmitter, and
    //setup() starts Timer1 in InitSeq() about 230 ms BEFORE that call - LoadTrack() and
    //LoadPattern() sit in between, at 100 kHz over I2C. So the interrupt fires some
    //forty times while the port is still closed. A spin here runs inside an interrupt
    //with the flags masked : nothing can preempt it, nothing can recover it, the machine
    //is simply dead until the power is cut.
    //
    //Dropping a clock byte that has nowhere to go costs nothing - the sequencer is not
    //running yet and no one is listening. Freezing costs everything.
    if (UCSR1A & (1 << UDRE1)) UDR1 = CLOCK_CMD;//Tick
  }
  if (seq.sync == MASTER){
    if (p % 4 == 0){
      DIN_CLK_HIGH;
      dinClkState = HIGH;
    }
    else if (p % 4 == 2) {
      DIN_CLK_LOW;
      dinClkState = LOW;
    }    
  }

  if (isRunning)
  {
    //p % scale ne depend que de p et de la scale, et il n'y a que QUATRE scales possibles.
    //C'etait recalcule pour chacun des seize instruments, deux fois chacun, plus quatre
    //fois hors de la boucle : 36 divisions par tick, dont seize sur 16 bits.
    //Calcule ici une fois par tick, avec des diviseurs constants a la compilation - le
    //compilateur les traduit en multiplication par reciproque au lieu d'appeler
    //__udivmodqi4. Indexe par SCALE_ROW(), le meme rang que la table de swing.
    //Effet de bord bienvenu : une scale corrompue a zero faisait une division par zero,
    //ce qui n'est plus possible.
    //Ecrit en une seule vraie division, et pas quatre :
    //  16 et 32 sont des puissances de deux, donc de simples masques ;
    //  12 se deduit de 24 par une comparaison, puisque p % 24 est deja inferieur a 24 ;
    //  reste 24, la seule qui divise vraiment.
    //Les operandes sont gardes en octets : ecrit tel quel, "p % 24" promeut en entier et
    //sort __divmodhi4, 230 cycles. En octets, avr-gcc emet une multiplication par
    //reciproque, une dizaine de cycles.
    byte m24 = (byte)(p % (byte)SCALE_16);
    byte modBySc[4];
    modBySc[SCALE_ROW(SCALE_16)]  = m24;                                        //24
    modBySc[SCALE_ROW(SCALE_32)]  = (m24 >= SCALE_32) ? m24 - SCALE_32 : m24;   //12
    modBySc[SCALE_ROW(SCALE_16t)] = p & (SCALE_16t - 1);                        //16
    modBySc[SCALE_ROW(SCALE_8t)]  = p & (SCALE_8t - 1);                         //32

    byte gs    = pattern[ptrnBuffer].globalScale;
    byte gsMod = modBySc[SCALE_ROW(gs)];

    if (gsMod == (gs/2)) tapStepCount++;

    if (tapStepCount > pattern[ptrnBuffer].globalLength) tapStepCount = 0;

      //stepValue is volatile and read by Led.ino, but it was also accumulated with
      //|= inside the 16 instrument loop - one SRAM round trip per iteration.
      //Built in a local, published once.
      unsigned int sv = 0;

    for (byte z = 0; z < NBR_INST; z++){
      byte sc  = pattern[ptrnBuffer].scale[z];
      byte mod = modBySc[SCALE_ROW(sc)];
      if (mod == 0) stepChanged[z] = TRUE;//Step changed ?

      //-----------------------------------------------------------------------------
      //Here is everything happen each step
      //-----------------------------------------------------------------------------
      //Swing offset : even steps stay on the grid, odd steps are pushed (setting - 1)
      //ticks late. Derived from stepCount, never from a flag of its own.
      //shufPolarity was a boolean toggled on every step played, and it drifted : on a
      //pattern with an odd number of steps it came back inverted at each loop, so the
      //swing moved onto the even steps. It was reset on PLAY only - not on a pattern
      //change, not on a length change, not on a resync - so the swing phase was
      //effectively arbitrary the rest of the time. stepCount[z] already carries the
      //step number and is wrapped, reset and resynchronised everywhere it must be, so
      //reading its low bit makes the drift structurally impossible.
      //This also replaces the shuffle[MAX_SHUF_TYPE][2] table, which held nothing the
      //index did not already carry : column 0 was always 0, column 1 always minus the
      //row number.
      //Le retard vient de la table de swing plutot que de (reglage - 1) ticks, pour qu'un
      //cran veuille dire un ratio sur les quatre resolutions : cf define.h.
      //Le test etait "(p - retard) % scale == 0", une division SUR 16 BITS parce que la
      //soustraction promeut en int : 230 cycles, seize fois par tick. C'est la meme chose
      //que "p % scale == retard", qui ne coute qu'une comparaison sur le modulo deja en
      //main. L'equivalence tient parce que la table garantit retard < scale, et parce que
      //p - retard ne peut jamais tomber sur un multiple negatif de scale : p vaut au
      //moins 1 et le retard au plus 13, donc p - retard >= -12, alors que le premier
      //multiple negatif vaut -12 pour scale 12 - inatteignable, le retard y est borne a 5.
      byte d = (stepCount[z] & 1)
        ? SWING_TICKS(sc, pattern[ptrnBuffer].shuffle[z])
        : 0;

      if (mod == d && stepChanged[z])
      {//Each Step
        stepChanged[z] = FALSE;//flag that we already trig this step
        blinkFast[z] = HIGH;// TO BE REMOVED OUT OF THIS FOR LOOP

        //sequencer direction for each instruments-----------
        switch (pattern[ptrnBuffer].dir[z]){
        case FORWARD:
          curStep[z] = stepCount[z];
          break;
        case BACKWARD:
          curStep[z] = pattern[ptrnBuffer].length[z] - stepCount[z];
          break;
        case PING_PONG:
          if (curStep[z] == pattern[ptrnBuffer].length[z] && changeDir[z] == 1) changeDir[z] = 0;
          else if (curStep[z] == 0 && changeDir[z] == 0)  changeDir[z] = 1;
          if (changeDir[z]) curStep[z] = stepCount[z];
          else curStep[z] = pattern[ptrnBuffer].length[z] - stepCount[z];
          break;
        case RANDOM:
          curStep[z] = RandomStep(pattern[ptrnBuffer].length[z]);
          break;
        }
        //Set each step value in a 16bit word depending step counter and direction of each instrument
        //Compare the current step of each instrument and store in "stepValue"
        if ( z != GLOBAL_ACC ){//&& z != TR1 && z != TR2){
          sv |= ((bitRead(pattern[ptrnBuffer].inst[z],curStep[z])) << z);
        }

        //Set the multiplexer for velocity out just before Trig
        stepCount[z]++;
      }
    }

    //incremente global step count according the longer instrument length
    if (modBySc[SCALE_ROW(pattern[ptrnBuffer].scale[longerInstLength])] == 0){
      globalStepCount++;
    }

    //Set Metronome value to be trigged
    if (gsMod == 0){
      sv |= (bitRead(metronome,(globalStepCount-1)) << RM);
    }

    //Add Mute value to StepValue
    sv &= ~muteInst;
    stepValue = sv;//published for Led.ino

    //Is there something to trig ?
    if (sv) {
      if (bitRead(sv, TR1)){
        TR1_LOW;//Trigout pulse 
        trig1State = HIGH;
      }
      if (bitRead(sv, TR2)){
        TR2_HIGH;//Trigout pulse 
        trig2State = HIGH;
      }
      SetMux();
      //Trig according mute value
      SetDoutTrig(sv);
      StartTrigPulse();//Timer2 releases the outputs 2 ms later

      //Trig external instrument-------------------------------------
      if (bitRead(sv, EXT_INST))//pattern[ptrnBuffer].inst[EXT_INST], curStep[EXT_INST]))
      {
        InitMidiNoteOff();
        MidiSendNoteOn(seq.TXchannel, pattern[ptrnBuffer].extNote[noteIndexCpt], HIGH_VEL);
        midiNoteOnActive = TRUE;
        noteIndexCpt++;//incremente external inst note index
      }
      if (noteIndexCpt > pattern[ptrnBuffer].extLength){
        noteIndexCpt = 0;
      }
    }//END IF STEP VALUE

    for (byte z = 0; z < NBR_INST; z++){
      //  stepChanged[z] = FALSE;//flag that we already trig this step
      //  stepCount[z]++;
      if (stepCount[z] > pattern[ptrnBuffer].length[z]){
        stepCount[z] = 0;
      }
      //}
    }


    if (globalStepCount > pattern[ptrnBuffer].globalLength){// && (p % 24 == pattern[ptrnBuffer].scale - 1))
      endMeasure = TRUE;
      trackPosNeedIncremante = TRUE;
      globalStepCount = 0;
      //The buffer swap used to be duplicated here ; it never ran, the main loop
      //cleared nextPatternReady first. Single copy in SwitchPatternBuffer() now,
      //which also stops the interrupt writing ptrnBuffer behind the main loop.
    }
    if (gsMod == 4 && globalStepCount == 0){ 
      endMeasure = FALSE;
    }
  }

  if (++p > PPQN) p = 1;
  ppqn = p;
}// END COUNT FUNCTION

//
void Metronome(boolean state)
{
  if (state){
    metronome = 0x1111;//trig RM every time
    pattern[ptrnBuffer].velocity[RM][0] = 127;
    pattern[ptrnBuffer].velocity[RM][4] = 30;
    pattern[ptrnBuffer].velocity[RM][8] = 30;
    pattern[ptrnBuffer].velocity[RM][12] = 30;
  }
  else{
    metronome = 0;
    pattern[ptrnBuffer].velocity[RM][0] = HIGH_VEL;
    pattern[ptrnBuffer].velocity[RM][4] = HIGH_VEL;
    pattern[ptrnBuffer].velocity[RM][8] = HIGH_VEL;
    pattern[ptrnBuffer].velocity[RM][12] = HIGH_VEL;
  }
}








































