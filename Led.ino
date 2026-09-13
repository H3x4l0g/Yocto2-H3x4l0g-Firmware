//-------------------------------------------------
//                  Yocto v2.x
//              Sequencer Led
//-------------------------------------------------

////////////////////////Function//////////////////////

//The eight bank candidates, the current one at full brightness--------------------
//BANK held turns the step buttons into bank select, and there are only NBR_BANK = 8 of
//them : half the row does nothing. The panel used to light a single LED, the current
//bank - it said where you were, never what was reachable.
//flagLedIntensity is the same temporal dither that fades low velocity steps further
//down : what appears ONLY in the one-frame-in-nine branch reads as dim, what appears in
//both reads at full brightness. curBank sits inside the candidate mask, so it is lit on
//every frame while the seven others merely glow.
unsigned int BankLeds()
{
  if (flagLedIntensity >= 8){
    flagLedIntensity = 0;
    return (1U << NBR_BANK) - 1;//the eight candidates, one frame in nine
  }
  flagLedIntensity++;
  return 1U << curBank;//the current one, every frame
}

//Boot sweep : one Knight Rider bounce across the sixteen step LEDs, played while the
//version stays on screen.
//The LEDs are plain on/off - there is no PWM anywhere on this board - so the comet tail
//is dithered in time instead. Within one position the head is lit on every frame, the
//LED just behind it on three frames out of eight, the one behind that on one out of
//eight. At 4 ms a frame the eye integrates the three duty cycles into a gradient, and
//that is what makes a tail rather than a hard block of three.
//30 positions : 0 up to 15, then 14 back down to 1, so neither end is held twice.
void BootSweep()
{
  for (byte pos = 0; pos < 30; pos++){
    byte head = (pos < NBR_BTN_STEP) ? pos : (30 - pos);
    int  back = (pos < NBR_BTN_STEP) ? -1 : 1;//the tail trails behind the head

    for (byte frame = 0; frame < 8; frame++){
      unsigned int leds = 1U << head;
      if (frame < 3){
        int t = head + back;
        if (t >= 0 && t < NBR_BTN_STEP) leds |= 1U << t;
      }
      if (frame < 1){
        int t = head + 2 * back;
        if (t >= 0 && t < NBR_BTN_STEP) leds |= 1U << t;
      }
      SetDoutLed(leds, 0, 0);
      delay(4);
    }
  }
  SetDoutLed(0, 0, 0);
}

void SetLeds()
{
  //------------------------------Menu LEDS-------------------------------
  shiftLed = shiftBtn.pressed;
  instLed = instBtn;
  clearLed = clearBtn.pressed;
  shufLed = shufBtn.pressed;
  scaleLed = scaleBtn.pressed;
  lastStepLed = lastStepBtn.pressed ;
  if (isRunning){
    menuLed = LED_PLAY | shiftLed<<1 | instLed<<2 | clearLed<<3 | shufLed<<4 | lastStepLed<<5 | scaleLed<<6 ;
  }
  else{
    menuLed = (LED_PLAY*blinkTempo) | LED_STOP | shiftLed<<1 | instLed<<2 | clearLed<<3 | shufLed<<4 | lastStepLed<<5 | scaleLed<<6;
  }

  //------------------------------Config LEDS-------------------------------
  trackLed =((curSeqMode == TRACK_WRITE)? blinkTempo:((curSeqMode == TRACK_PLAY)? 1:0));
  ptrnLed =((curSeqMode == PTRN_STEP)? blinkTempo:((curSeqMode == PTRN_PLAY)? 1:0));
  tapLed =((curSeqMode == PTRN_TAP)? blinkTempo:((tapBtn.pressed) ? 1:0));
  if (keyboardMode){
    backLed = HIGH;
    fwdLed = HIGH;
  }
  else{
    backLed = backBtn.pressed;
    fwdLed = fwdBtn.pressed;
  }

  dirLed = dirBtn.pressed;
  scaleLeds = 1 << scaleBtn.counter;
  guideLed =  guideBtn.counter;
  if (curSeqMode == PTRN_STEP && curInst == EXT_INST) guideLed = blinkTempo;
  bankLed = bankBtn.pressed;

  if (instBtn && curInst == GLOBAL_ACC) enterLed = HIGH;
  else if (patternNeedSaved || trackNeedSaved || seq.setupNeedSaved) enterLed = blinkTempo;
  else enterLed = LOW;

  if (curSeqMode == MUTE) muteLed = HIGH;
  else muteLed = LOW;

  if (seq.configMode) tempoLed = blinkTempo;
  else  tempoLed = tempoBtn.pressed;

  configLed = numLed | scaleLeds<<1 | trackLed<<5 | backLed<<6 | fwdLed<<7 | enterLed<<8 | ptrnLed<<9 | tapLed<<10 | dirLed<<11 |guideLed<<12 | bankLed<<13 | muteLed<<14 | tempoLed<<15;

  //------------------------------Step LEDS-------------------------------
  //Update inst selected Leds
  instSlctLed = (1 << curInst);


  //Update step Leds depends sequencer Mode
  switch (curSeqMode){
    //------------------------------
    unsigned int temp;// tmep data of stepLeds;
  case TRACK_PLAY:
    if (isRunning) stepLeds = (1 << trk.current) ^ (1 << curStep[longerInstLength]);
    else stepLeds = blinkTempo << trk.current;
    break;
  case TRACK_WRITE:
    if (bankBtn.pressed){
      stepLeds = BankLeds();
    }
    else if (numBtn.pressed){
      stepLeds = 1 << trk.current;
    }
    else{
      if (isRunning) stepLeds = (1 <<(curPattern % NBR_PATTERN)) ^ (1 << curStep[longerInstLength]);
      else stepLeds = blinkTempo << (curPattern % NBR_PATTERN); 
    }
    break;
    //------------------------------
  case PTRN_PLAY:
    if (bankBtn.pressed){
      stepLeds = BankLeds();
    }
    else{
      if (isRunning){
        if (group.length){
          temp = 0;
          for (int a = 0; a <= group.length; a++){
            bitSet(temp,(group.firstPattern % NBR_PATTERN) + a);
          }
          stepLeds = (temp & ~(!blinkTempo << (nextPattern % NBR_PATTERN))) ^ (1 << curStep[longerInstLength]);
        }
        else {
          stepLeds = (blinkTempo <<(nextPattern % NBR_PATTERN)) ^ (1 << curStep[longerInstLength]);
        }
      }
      else if (!isRunning){
        temp = 0;
        for (int a = 0; a <= group.length; a++){
          bitSet(temp,(group.firstPattern % NBR_PATTERN) + a);
        }
        stepLeds = temp & ~(!blinkTempo << (nextPattern % NBR_PATTERN));
        /* }
         else {
         stepLeds = blinkTempo << (curPattern % NBR_PATTERN); 
         }*/
      }
    }
    break;
    //------------------------------
  case PTRN_STEP:
    //display selected inst on steps Leds
    if (instBtn){
      if(flagLedIntensity >= 8){
        stepLeds = instSlctLed ;
        flagLedIntensity = 0;
      }
      else{
        stepLeds = instSlctLed & LED_MASK;
        flagLedIntensity++;
      }
    }
    else if(isRunning && !instBtn){
      stepLedsHigh = stepLedsLow = 0;//initialize step Leds variable 
      for (int stp = 0; stp < NBR_STEP; stp++){
        if (pattern[ptrnBuffer].velocity[curInst][stp] > instVelLow[curInst] && bitRead(pattern[ptrnBuffer].inst[curInst],stp)) bitSet(stepLedsHigh, stp);
        else if (pattern[ptrnBuffer].velocity[curInst][stp] <= instVelLow[curInst] && bitRead(pattern[ptrnBuffer].inst[curInst],stp)) bitSet(stepLedsLow,stp);
        else { 
          bitClear(stepLedsHigh, stp);   
          bitClear(stepLedsLow, stp); 
        }
      }
      //this function is to fade low velocity leds
      if(flagLedIntensity >= 8){
        //Parenthese telle que le compilateur la lisait deja : H | (L ^ tete de lecture).
        //La ligne soeur juste en dessous et l'intention suggeraient (H | L) ^ tete, ce
        //qui inverserait la tete de lecture sur TOUS les pas, y compris ceux allumes a
        //pleine intensite.
        //JUGE A L'OEIL LE 13/09/2026 en edition de pas : la tete passe visiblement sur
        //les pas allumes. Le comportement actuel est donc garde, et ce n'est plus une
        //dette - c'est un choix. Ne pas "corriger" sans regarder l'ecran d'abord.
        stepLeds = stepLedsHigh | (stepLedsLow ^ (blinkFast[curInst] << curStep[curInst]));
        flagLedIntensity = 0;
      }
      else{
        stepLeds = stepLedsHigh ^ blinkFast[curInst] << curStep[curInst];//B1010101010101010;
        flagLedIntensity++;
      }
    }
    else if (!isRunning && !instBtn){
      //Display Bank number
      if (bankBtn.pressed){
        stepLeds = BankLeds();
      }
      //display selected pattern
      else{
        // if (group.length)
        temp = 0;
        for (int a = 0; a <= group.length; a++){
          bitSet(temp,(group.firstPattern % NBR_PATTERN) + a);
        }
        stepLeds = temp & ~(!blinkTempo << (curPattern % NBR_PATTERN));
      }
    }
    break;
  case PTRN_TAP:
    stepLeds = stepValue;//(pattern[ptrnBuffer].step[globalStepCount] * blinkFast) ;
    break;
    //------------------------------
  case MUTE:
    stepLeds = muteInst ^ stepValue;
    break;
  }
  //Send OUTPUTS now !
  //Only push to the shift registers when something actually changed. SetLeds() runs on
  //every pass of loop() - about 2800 times a second, measured on the scope - and each
  //SetDoutLed() is five SPI bytes with the interrupts masked throughout, some 20 us.
  //Most of those passes resend a word identical to the one already latched.
  //This does not touch the frame rate, so the low velocity dithering of flagLedIntensity
  //above keeps its cadence : when it is active the value alternates every frame and the
  //write happens anyway. What disappears is the idle traffic.
  //The initial values are 0 because that is what is really in the registers : setup()
  //writes SetDoutLed(0, 0, 0) and BootSweep() ends on the same.
  static unsigned int prevStepLeds = 0, prevConfigLed = 0;
  static byte prevMenuLed = 0;
  if (stepLeds != prevStepLeds || configLed != prevConfigLed || menuLed != prevMenuLed){
    prevStepLeds  = stepLeds;
    prevConfigLed = configLed;
    prevMenuLed   = menuLed;
    SetDoutLed(stepLeds, configLed , menuLed);
  }
}



































