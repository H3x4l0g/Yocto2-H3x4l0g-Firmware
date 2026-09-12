//-------------------------------------------------
//                  Yocto v2.x
//                  BPM
//-------------------------------------------------

/////////////////////Function//////////////////////
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
    if (p % pattern[ptrnBuffer].globalScale == (pattern[ptrnBuffer].globalScale/2)) tapStepCount++;

    if (tapStepCount > pattern[ptrnBuffer].globalLength) tapStepCount = 0;

      //stepValue is volatile and read by Led.ino, but it was also accumulated with
      //|= inside the 16 instrument loop - one SRAM round trip per iteration.
      //Built in a local, published once.
      unsigned int sv = 0;

    for (byte z = 0; z < NBR_INST; z++){
      if (p % pattern[ptrnBuffer].scale[z] == 0) stepChanged[z] = TRUE;//Step changed ?

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
      //The delay is read from the swing table rather than taken as (setting - 1) ticks,
      //so one setting means one ratio on all four resolutions : cf define.h.
      int swing = (stepCount[z] & 1)
        ? -(int)SWING_TICKS(pattern[ptrnBuffer].scale[z], pattern[ptrnBuffer].shuffle[z])
        : 0;

      if (((p + swing) % pattern[ptrnBuffer].scale[z] == 0) && stepChanged[z])
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
          curStep[z] = random(0, pattern[ptrnBuffer].length[z]+1);
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
    if (p % pattern[ptrnBuffer].scale[longerInstLength] == 0){
      globalStepCount++;
    }

    //Set Metronome value to be trigged
    if (p % pattern[ptrnBuffer].globalScale == 0){
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
      // if(stepChanged[z]){
      //  stepChanged[z] = FALSE;//flag that we already trig this step
      //  stepCount[z]++;
      if (stepCount[z] > pattern[ptrnBuffer].length[z]){
        stepCount[z] = 0;
      }
      //}
    }

    /* //init each instrument step counter
     for (byte z = 0; z < NBR_INST; z++){
     if (stepCount[z] > pattern[ptrnBuffer].length[z]){
     stepCount[z] = 0;
     }
     }*/

    if (globalStepCount > pattern[ptrnBuffer].globalLength){// && (p % 24 == pattern[ptrnBuffer].scale - 1))
      endMeasure = TRUE;
      trackPosNeedIncremante = TRUE;
      globalStepCount = 0;
      //The buffer swap used to be duplicated here ; it never ran, the main loop
      //cleared nextPatternReady first. Single copy in SwitchPatternBuffer() now,
      //which also stops the interrupt writing ptrnBuffer behind the main loop.
    }
    if (p % pattern[ptrnBuffer].globalScale == 4 && globalStepCount == 0){ 
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








































