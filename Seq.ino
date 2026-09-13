//======================================================================================
//
//   S E Q . I N O                              ce que font les touches, partout
//
//   Le plus gros fichier du depot, et une seule fonction : SeqParameter(). Elle decide
//   de tout ce qu'un bouton declenche - sauf le clavier, qui vit dans key.ino. Neuf
//   cents lignes, mais une charpente simple qu'il faut avoir en tete avant d'entrer.
//
//--------------------------------------------------------------------------------------
//   LA CHARPENTE
//
//   if (shiftBtn.pressed)  ... les gestes a deux touches : TAP, copie, direction
//   else                   ... les gestes nus : TRK, PATT, decalage, GUIDE, MUTE
//     puis, par mode       ... PTRN_STEP / PTRN_TAP / TRACK_WRITE / TRACK_PLAY ...
//   et en fin de fonction  ... la bascule de buffer, les drapeaux, les sauvegardes
//
//--------------------------------------------------------------------------------------
//   AVANT D'Y TOUCHER
//
//   1. LA COUPURE SHIFT EST LA STRUCTURE, pas un detail de mise en page. Un geste place
//      du mauvais cote ne se declenchera jamais - c'est ce qui a tenu le changement de
//      direction enferme en PATT EDIT pendant des annees.
//
//   2. MUTE N'EST PAS UN MODE, c'est une surcouche des touches de pas. D'ou l'idiome
//      "SeqMode from = (curSeqMode == MUTE) ? prevSeqMode : curSeqMode" partout ou un
//      geste doit continuer de marcher pendant un mute. L'oublier, c'est faire
//      disparaitre le geste des qu'on coupe une voix.
//
//   3. CHANGER DE FAMILLE TRACK <-> PATT DEMANDE L'ARRET DU TRANSPORT. Les deux ne
//      jouent pas la meme chose. La regle est portee par IS_TRACK_MODE() et se teste
//      sur "from", jamais sur curSeqMode brut.
//
//   4. LES DRAPEAUX SONT LA MEMOIRE DE CE QUI N'EST PAS ENCORE SAUVE.
//      patternWasEdited et trackNeedSaved allument la LED ENTER et autorisent
//      l'ecriture. Modifier une donnee sans poser son drapeau, c'est un travail perdu
//      en silence au prochain chargement - deux bugs de ce type ont ete corriges ici.
//
//======================================================================================

/////////////////////Function//////////////////////
void SeqParameter()
{  

  readButtonState = StepButtonGet(MOMENTARY);
  //can not access config when isRunning
  if (isRunning && seq.configMode){
    seq.configMode = FALSE;
    needLcdUpdate = TRUE;
  }
  if (curSeqMode != MUTE) muteBtn.counter = 0;
  if (!seq.configMode) seq.configPage = 0;

  //-------------------Encoder button---------------------------
  if(encBtn.justPressed){
    curIndex++;
    if (curIndex >= MAX_CUR_POS)  curIndex = 0;
    needLcdUpdate = TRUE;
  }
  //-------------------play button---------------------------
  if (playBtn.justPressed || midiStart){
    isRunning = TRUE;
    isStop = FALSE;
    ppqn = 0;
    globalStepCount = 0;
    for(byte z = 0; z < NBR_INST; z++){
      stepCount[z] = 0;
      changeDir[z] = 1;//restart Forward direction on each instrument
    }
    tapStepCount = 0;
    stopBtn.counter = 0;
    noteIndexCpt = 0;//init ext instrument note index counter
    MIDI.sendRealTime(midi::Start);//;MidiSend(START_CMD);
    DIN_START_HIGH;
    dinStartState = HIGH;
    DIN_CLK_HIGH;
    dinClkState = HIGH;
  }

  //-------------------stop button------------------------------
  if ((stopBtn.justPressed && !instBtn) || midiStop || midiContinue){
    //Init Midi note off
    SendAllNoteOff();//InitMidiNoteOff();
    if (midiStop) stopBtn.counter = 0;
    else if (midiContinue) stopBtn.counter = 1;
    stopBtn.counter++;
    switch (stopBtn.counter){
    case 1:
      isStop = TRUE;
      isRunning = FALSE;
      MIDI.sendRealTime(midi::Stop);//;MidiSend(STOP_CMD);
      DIN_START_LOW;
      dinStartState = LOW;
      break;
    case 2:
      isStop = FALSE;
      isRunning = TRUE;
      stopBtn.counter = 0;
      ppqn = 0;
      MIDI.sendRealTime(midi::Continue);//MidiSend(CONTINU_CMD);
      DIN_START_HIGH;
      dinStartState = HIGH;
      break;
    }
  }

  //-------------------Shift button pressed------------------------------
  if (shiftBtn.pressed){
    //--------------------------------sequencer run direction-----------------------
    //Le manuel documente le changement de direction DANS LES DEUX MODES : section 2
    //"Pattern play mode" - "Change pattern direction (forward > backward > ping-pong >
    //random)" - et section 3 "pattern edit step mode", qui ajoute seulement la variante
    //SHIFT + INST pour l'instrument courant.
    //Le code ne l'autorisait qu'en PATT EDIT : le handler etait enferme dans
    //if (curSeqMode == PTRN_STEP). Or son jumeau, le decalage gauche/droite, est
    //documente en mode PLAY avec la mention "work in edit mode too" et n'a jamais eu de
    //filtre de mode. Les deux devaient etre symetriques ; l'une l'etait restee, l'autre
    //non. C'est un geste de jeu : il a sa place sur le pattern qui tourne.
    //MUTE n'est qu'une surcouche des touches de pas, d'ou le meme idiome "from" que les
    //trois autres handlers : muter ne doit pas avaler le geste.
    if (dirBtn.justPressed){
      SeqMode from = (curSeqMode == MUTE) ? prevSeqMode : curSeqMode;
      if (from == PTRN_STEP || from == PTRN_PLAY) ChangePatternDirection();
    }
    if (tapBtn.justPressed){
      //Same transport rule as TRK and PATT below : tap writing is a pattern job, so it
      //cannot be reached from a running track either. Without this the rule had a side
      //door - Shift+TAP then PATT crossed the two families in two presses.
      SeqMode from = (curSeqMode == MUTE) ? prevSeqMode : curSeqMode;
      if (!IS_TRACK_MODE(from) || !isRunning){
        curSeqMode = PTRN_TAP;
        needLcdUpdate = TRUE;
        keyboardMode = FALSE;
        seq.configMode  = FALSE;
        //trackNeedSaved deliberately left alone : cf the ptrnBtn handler below.
      }
    }
    if (bankBtn.justPressed){
      CopyPatternToBuffer();
    } // copy current pattern to the buffer
    if (muteBtn.justPressed){
      PasteBufferToPattern();
      patternWasEdited = TRUE;
    } //paste copy buffered pattern to the current pattern number

    //sequencer configuration page
    if (tempoBtn.justPressed && !isRunning){
      seq.configMode  = TRUE;
      seq.configPage++;
      curIndex = 0;
      if (seq.configPage > MAX_CONF_PAGE){
        seq.configPage = 0;
        seq.configMode  = FALSE;
      }
      needLcdUpdate = TRUE;
    }
  }
  //-------------------Shift button released------------------------------
  else {
    if (trkBtn.justPressed){
      //Each button toggles inside its own family, but crossing from one family to the
      //other used to land on the edit screen whatever you came from : Patt play + TRK
      //gave Track edit, Track play + PATT gave Patt edit. The two lines that decided
      //this did not know about each other, and each only tested its own mode.
      //The play/edit character of the mode being left is now carried across.
      SeqMode from = (curSeqMode == MUTE) ? prevSeqMode : curSeqMode;
      //And crossing over is refused while the transport runs. The two families do not
      //play the same thing - one pattern on one side, a chain of them on the other - so
      //crossing over swapped what the sequencer was playing under your fingers, with no
      //say in where it landed in the chain. You stop first, deliberately.
      //The toggle inside a family stays free, running or not : it changes what the step
      //buttons and the screen do, never what is being played.
      if (IS_TRACK_MODE(from) || !isRunning){
        if (from == TRACK_WRITE)     curSeqMode = TRACK_PLAY;//toggle inside the family
        else if (from == TRACK_PLAY) curSeqMode = TRACK_WRITE;
        else curSeqMode = (from == PTRN_PLAY) ? TRACK_PLAY : TRACK_WRITE;//crossing over
        //En arrivant dans la famille TRACK, le pattern courant doit devenir celui que la
        //piste contient a la position courante. Sans ce recalage, apres un cycle
        //d'allumage, TRACK EDIT affichait le pattern charge au boot - A1 - la ou la piste
        //contient autre chose, et il fallait CLEAR, BACK, un tour de molette ou un cycle
        //de lecture pour que l'ecran dise la verite. Les donnees en EEPROM etaient
        //bonnes : c'est l'affichage qui mentait.
        //Tous les autres chemins qui bougent trk.pos faisaient deja ce recalage - BACK,
        //FWD, CLEAR, la molette, l'insertion, ENTER, le changement de piste. Il manquait
        //celui de l'entree dans le mode, le seul ou trk.pos ne bouge pas mais devient
        //soudain ce qui compte.
        TrackPosToPattern();
        needLcdUpdate = TRUE;
        keyboardMode = FALSE;
        seq.configMode  = FALSE;
      }
    }
    // if (backBtn.justPressed) ;//back  track postion
    //if (fwdBtn.justPressed) ;//foward track postion
    if (numBtn.pressed) ;//select Track number
    if (ptrnBtn.justPressed){
      SeqMode from = (curSeqMode == MUTE) ? prevSeqMode : curSeqMode;//cf trkBtn above
      if (!IS_TRACK_MODE(from) || !isRunning){//transport rule : cf trkBtn above
        if (from == PTRN_STEP)      curSeqMode = PTRN_PLAY;//toggle inside the family
        else if (from == PTRN_PLAY) curSeqMode = PTRN_STEP;
        else curSeqMode = (from == TRACK_PLAY) ? PTRN_PLAY : PTRN_STEP;//crossing over
        needLcdUpdate = TRUE;
        keyboardMode = FALSE;
        seq.configMode  = FALSE;
      }
      //trackNeedSaved was cleared here. It is the flag that blinks the ENTER LED and
      //gates SaveTrack(), so leaving TRACK through this button silently dropped pending
      //track edits : the LED went dark and holding ENTER saved nothing. The data was
      //still in RAM but destroyed for good by the next LoadTrack().
    }
    if (tapBtn.justPressed) ShiftLeftPattern();
    if (dirBtn.justPressed) ShiftRightPattern();
    if (guideBtn.justPressed){
      guideBtn.counter++;
      switch (guideBtn.counter){
      case 1:
        Metronome(TRUE);
        break;
      case 2:
        Metronome(FALSE);
        guideBtn.counter = 0;
        break;
      }
    }
    if (muteBtn.justPressed){
      muteBtn.counter++;
      switch (muteBtn.counter){
      case 1:
        prevSeqMode = curSeqMode;
        curSeqMode = MUTE; //paste copy buffered pattern to the current pattern number
        break;
      case 2:
        curSeqMode = prevSeqMode;
        muteBtn.counter = 0;
        break;
      }
    }
    if (curSeqMode != MUTE) muteBtn.counter = 0;
    if (!seq.configMode) seq.configPage = 0;
    if (tempoBtn.justRelease) needLcdUpdate = TRUE;
  }

  //==============================================================================
  //////////////////////////MODE PATTERN EDIT/////////////////////////////////

  if (curSeqMode == PTRN_STEP || curSeqMode == PTRN_TAP){

    static boolean curInstChanged;//flag that curInstchanged to not update LCD more than one time

    //-------------------Select instrument------------------------------
    //Match with trig shift register out (cf schematic)
    if(readButtonState == 0) doublePush = 0; //init double push if all step button  released 

    if (instBtn && readButtonState){
      curInstChanged = TRUE;
      keyboardMode = FALSE;
      curInst = FirstBitOn();
    }
    if (curInstChanged && stepsBtn.justRelease){
      needLcdUpdate = TRUE;
      curInstChanged = FALSE;
    }
    if (instBtn && enterBtn.justPressed){
      curInst = GLOBAL_ACC;
      needLcdUpdate = TRUE;
    }
    if (shiftBtn.pressed && guideBtn.justPressed){
      curInst = EXT_INST;
      needLcdUpdate = TRUE;
    }

    //-------------------Clear Button------------------------------
    if (clearBtn.pressed && !keyboardMode && curSeqMode != PTRN_TAP && isRunning){
      bitClear (pattern[ptrnBuffer].inst[curInst], curStep[curInst]);
      pattern[ptrnBuffer].velocity[curInst][curStep[curInst]] = instVelLow[curInst];
      //if (curInst == CH) pattern[ptrnBuffer].velocity[CH][curStep] = instVelHigh[HH];//update HH velocity that OH is trigged correctly   

      patternWasEdited = TRUE;   
    }
    if (clearBtn.justPressed && !keyboardMode && curSeqMode != PTRN_TAP && !isRunning){
      //clear full pattern
      for (int a = 0; a < NBR_INST; a++){
        pattern[ptrnBuffer].inst[a] = 0;
        pattern[ptrnBuffer].shuffle[a] = DEFAULT_SHUF;
        pattern[ptrnBuffer].length[a] = NBR_STEP - 1;
        pattern[ptrnBuffer].scale[a] = SCALE_16;
        pattern[ptrnBuffer].dir[a] = 0;//0 is forward
      }
      ////////////////////////////////////////////////////////////////////////
      //init all intrument velocity ||   PEUT ETRE SUPPRIMER, A TESTER !!!  ||
      ////////////////////////////////////////////////////////////////////////

      keybOct = DEFAULT_OCT;
      patternWasEdited = TRUE;
      needLcdUpdate = TRUE;
    }


    //-------------------shuffle Button------------------------------
    if (shufBtn.justPressed || shufBtn.justRelease) needLcdUpdate = TRUE;

    //-------------------scale button------------------------------
    if (scaleBtn.justPressed && !keyboardMode){
      byte a;
      needLcdUpdate = TRUE;
      patternWasEdited = TRUE;
      scaleBtn.counter++;
      if (scaleBtn.counter == 4) scaleBtn.counter = 0;
      a = scaleValue[scaleBtn.counter];
      if (shiftBtn.pressed){
        pattern[ptrnBuffer].globalScale = a;
        //init all scale of each instrument when globalscale changed
        for (byte z = 0; z < NBR_INST; z++){
          pattern[ptrnBuffer].scale[z] = pattern[ptrnBuffer].globalScale;
        }
      }
      else pattern[ptrnBuffer].scale[curInst] = a;
    } 
    //Updte scaleBtn counter if pattern change with a different scale
    if (pattern[ptrnBuffer].scale[curInst] != scaleValue[scaleBtn.counter]){
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

    //-------------------last step button------------------------------
    if (lastStepBtn.pressed && stepsBtn.justPressed){//readButtonState) {
      if (!shiftBtn.pressed){
        pattern[ptrnBuffer].length[curInst] = FirstBitOn();
        pattern[ptrnBuffer].globalLength = 0;// init global length
        //set the highest length value as global length and remember which instrument is the longer length
        for (byte z = 0; z < NBR_INST; z++){
          if (pattern[ptrnBuffer].length[z] > pattern[ptrnBuffer].globalLength){
            pattern[ptrnBuffer].globalLength = pattern[ptrnBuffer].length[z];
            longerInstLength = z;
          }
        }
        UpdateLongerInst();
        ResyncStepCount(curInst);//keep it in phase with the grid
      }
      else if (shiftBtn.pressed){
        pattern[ptrnBuffer].globalLength = FirstBitOn();
        for (byte z = 0; z < NBR_INST; z++){
          pattern[ptrnBuffer].length[z] = pattern[ptrnBuffer].globalLength;
          ResyncStepCount(z);
        }
        UpdateLongerInst();
      }
      needLcdUpdate = TRUE;
      patternWasEdited = TRUE;
    }
    //-------------------Steps buttons------------------------------
    /////////////////////////////STEP EDIT///////////////////////////
    if (curSeqMode == PTRN_STEP){
      if (stepsBtn.justRelease) doublePush = FALSE;
      if(!lastStepBtn.pressed && !instBtn && !keyboardMode){
        if (isRunning)
        {
          pattern[ptrnBuffer].inst[curInst] = InstValueGet(pattern[ptrnBuffer].inst[curInst]);//cf InstValueGet()
          nextPattern = curPattern;//To prevent undesired poattern change during switching PLAY and STEP mode
        }
        else if (!isRunning)
        {//Return pattern number
          if (stepsBtn.pressed){
            if (bankBtn.pressed){
              //There are NBR_BANK = 8 banks for 16 step buttons, so half of them are not
              //banks at all. What used to sit here rounded the button number DOWN to the
              //last bank, so buttons 9 to 16 all selected bank H instead of doing
              //nothing : the panel answered sixteen buttons and eight of them lied.
              //The step LEDs now show the eight candidates (cf BankLeds() in Led.ino),
              //so the gesture and the display have to agree.
              //FirstBitOn() is also called once instead of twice : it re-reads the step
              //buttons on each call, and the two reads could disagree.
              //MAX_BANK is gone with this - one constant that says how many, rather than
              //two that differ by one and get confused for each other.
              byte bank = FirstBitOn();
              if (bank < NBR_BANK){
                curBank = bank;
                nextPattern = curBank * NBR_PATTERN + (curPattern % NBR_PATTERN);
                if(curPattern != nextPattern) selectedPatternChanged = TRUE;
                group.length = 0;
              }
            }
            else{//pattern group edit------------------------------------------------------
              if (SecondBitOn())
              {
                trackPosNeedIncremante = FALSE;                                                               // [zabox] fixes group bug
                group.length = SecondBitOn() - FirstBitOn();
                nextPattern = group.firstPattern = FirstBitOn() + curBank * NBR_PATTERN;
                group.pos = 1;//first pattern loaded right away, next measure plays index 1
                doublePush = TRUE;
                group.priority = TRUE;
                //Store groupe in eeprom
                if(enterBtn.justPressed){
                  group.priority = FALSE;
                  byte tempLength;
                  byte tempPos;
                  //Test if one the  selected pattern is already in a Group
                  for (int a = 0; a <= group.length; a++){
                    tempLength = LoadPatternGroup(group.firstPattern + a, LENGTH);
                    if (tempLength){
                      tempPos = LoadPatternGroup(group.firstPattern + a, POSITION);
                      ClearPatternGroup(group.firstPattern + a - tempPos, tempLength);
                    }
                  }
                  SavePatternGroup(group.firstPattern, group.length);
                }
              }
              else if (!doublePush){
                group.priority = FALSE;
                nextPattern = FirstBitOn() + curBank * NBR_PATTERN;
                if(enterBtn.justPressed){
                  ClearPatternGroup(nextPattern - pattern[ptrnBuffer].groupPos, pattern[ptrnBuffer].groupLength);
                  group.length = 0;
                }
                group.pos = pattern[ptrnBuffer].groupPos;
              }
              if(curPattern != nextPattern) selectedPatternChanged = TRUE;
            }
          }
        }
      }
      //pattern chain advance : cf PatternChainUpdate(), called from the common part

      //La direction du sequenceur vivait ici, enfermee dans PATT EDIT. Elle est remontee
      //dans la branche SHIFT du haut, ou PATT PLAY l'atteint aussi : cf
      //ChangePatternDirection().
    }
    //////////////////////////////TAP EDIT///////////////////////////////
    else if (curSeqMode == PTRN_TAP)
    {
      //static byte lastHHtrigged;// remmeber last OH or CH was trigged to prevent OH noise when trigget other instruments

      if (clearBtn.pressed){
        bitClear (pattern[ptrnBuffer].inst[curInst], curStep[curInst]);
        // if (curInst == CH) pattern[ptrnBuffer].velocity[CH][curStep[curInst]] = HIGH_VEL;//update HH velocity that OH is trigged correctly
        patternWasEdited = TRUE;
      } 
      if (!lastStepBtn.pressed && !instBtn)
      {


        for (byte a = 0; a < NBR_STEP_BTN; a++){
          stepBtn[a].curState = bitRead(readButtonState,a);


          if (stepBtn[a].curState != stepBtn[a].prevState){
            //pressed dropped from the guard : cf StepButtonGet() in Button.ino
            if (stepBtn[a].curState == HIGH){
              static byte tempVel[16];//store temp instrument velocity
              switch (a){
              case BD_BTN:
                if (isRunning) pattern[ptrnBuffer].velocity[instOut[a]][tapStepCount] = instVelHigh[BD];
                tempVel[BD] = instVelHigh[BD];
                break;              
              case SD_BTN:
                if (isRunning) pattern[ptrnBuffer].velocity[instOut[a]][tapStepCount] = instVelHigh[SD];
                tempVel[SD] = instVelHigh[SD];
                break;
              case LT_BTN:
                if (isRunning) pattern[ptrnBuffer].velocity[instOut[a]][tapStepCount] = instVelHigh[LT];
                tempVel[LT] = instVelHigh[LT];
                break;
              case MT_BTN:
                if (isRunning) pattern[ptrnBuffer].velocity[instOut[a]][tapStepCount] = instVelHigh[MT];
                tempVel[MT] = instVelHigh[MT];
                break;
              case HT_BTN:
                if (isRunning) pattern[ptrnBuffer].velocity[instOut[a]][tapStepCount] = instVelHigh[HT];
                tempVel[HT] = instVelHigh[HT];
                break;
              case PC2_BTN:
                if (isRunning) pattern[ptrnBuffer].velocity[instOut[a]][tapStepCount] = instVelHigh[PC2];
                tempVel[PC2] = instVelHigh[PC2];
                break;
              case CH_BTN:
                if (isRunning) pattern[ptrnBuffer].velocity[instOut[a]][tapStepCount] = instVelHigh[CH];
                tempVel[CH] = instVelHigh[CH];
                break;
              case RM_BTN:
                if (isRunning) pattern[ptrnBuffer].velocity[instOut[a]][tapStepCount] = instVelHigh[RM];
                tempVel[RM] = instVelHigh[RM];
                break;
              case HC_BTN:
                if (isRunning) pattern[ptrnBuffer].velocity[instOut[a]][tapStepCount] = instVelHigh[HC];
                tempVel[HC] = instVelHigh[HC];
                break;
              case CY_BTN:
                if (isRunning) pattern[ptrnBuffer].velocity[instOut[a]][tapStepCount] = instVelHigh[CY];
                tempVel[CY] = instVelHigh[CY];
                break;
              case OH_BTN:
                if (isRunning) pattern[ptrnBuffer].velocity[instOut[a]][tapStepCount] = instVelHigh[OH];
                tempVel[OH] = instVelHigh[OH];
                break;
              case CB_BTN:
                if (isRunning) pattern[ptrnBuffer].velocity[instOut[a]][tapStepCount] = instVelHigh[CB];
                tempVel[CB] = instVelHigh[CB];
                break;
              }


              //-----SET Velocity Values-----//
              //Set first multiplexer IC111 value
              bitWrite(MUX_INH_PORT, MUX_INH1_BIT, 0);
              bitWrite(MUX_INH_PORT, MUX_INH2_BIT, 1);   
              SetDacA(tempVel[CY]);//
              PORTA = dinStartState | dinClkState << 1 | 1 << 2 | B111 << 5;
              SetDacA(tempVel[OH]);//
              PORTA = dinStartState | dinClkState << 1 | 1 << 2 | B11 << 5;
              SetDacA(tempVel[CH]);//
              PORTA = dinStartState | dinClkState << 1 | 1 << 2 | B101 << 5;
              SetDacA(tempVel[PC2]);//
              PORTA = dinStartState | dinClkState << 1 | 1 << 2 | B1 << 5;
              ////////////////////////////////////////////
              delayMicroseconds(100);//Coudl BE REMOVE !!!!
              ////////////////////////////////////////////
              //Set Second multiplexer values
              bitWrite(MUX_INH_PORT, MUX_INH1_BIT, 1);
              bitWrite(MUX_INH_PORT, MUX_INH2_BIT, 0);   
              SetDacA(tempVel[BD]);//
              PORTA = dinStartState | dinClkState << 1 | 1 << 2 | B111 << 5;
              SetDacA(tempVel[SD]);//
              PORTA = dinStartState | dinClkState << 1 | 1 << 2 | B11 << 5;
              SetDacA(tempVel[LT]);//
              PORTA = dinStartState | dinClkState << 1 | 1 << 2 | B101 << 5;
              SetDacA(tempVel[MT]);//
              PORTA = dinStartState | dinClkState << 1 | 1 << 2 | B1 << 5;
              SetDacA(tempVel[HT]);//
              PORTA = dinStartState | dinClkState << 1 | 1 << 2 | B110 << 5;
              SetDacA(tempVel[RM]);//
              PORTA = dinStartState | dinClkState << 1 | 1 << 2 | B10 << 5;
              SetDacA(tempVel[HC]);//
              PORTA = dinStartState | dinClkState << 1 | 1 << 2 | B100 << 5;
              SetDacA(tempVel[CB]);//
              PORTA = dinStartState | dinClkState << 1 | 1 << 2 | B0 << 5;

              SetDoutTrig(1 << instOut[a]);
              if(isRunning) bitSet(tempInst[instOut[a]],tapStepCount);
              delayMicroseconds(2000);
              SetDoutTrig(0);// Reset Trig
            }      
          }
          stepBtn[a].prevState = stepBtn[a].curState;
        }//END FOR LOOP
      }
      if (endMeasure){//Update pattern at the end of measure to not get a double trig
        for (int inst = 0; inst < NBR_INST; inst++){
          if (tempInst[inst])//if instruments was edited
          {
            pattern[ptrnBuffer].inst[inst] |= tempInst[inst];
            tempInst[inst] = 0;// init tempInst
            patternWasEdited = TRUE;
          }
        }
      }
    }//END IF PTRN_TAP MODE
  }//ENDIF MODE EDIT

  //=======================================================================================
  //////////////////////////MODE PATTERN PLAY.../////////////////////////////////

  if (curSeqMode == PTRN_PLAY){
    //-------------------------------select pattern-----------------------------------
    if (stepsBtn.justRelease) doublePush = FALSE;
    if (readButtonState){
      if (bankBtn.pressed){
        byte bank = FirstBitOn();//9 to 16 are not banks : cf the PTRN_STEP site above
        if (bank < NBR_BANK){
          curBank = bank;
          nextPattern = curBank * NBR_PATTERN + (curPattern % NBR_PATTERN);
          group.length = 0;//should be 0 to play the right next pattern
          if((curPattern != nextPattern) && !isRunning) selectedPatternChanged = TRUE;
        }
      }
      else{
        //Group selected
        if (SecondBitOn())
        {
          group.length = SecondBitOn() - FirstBitOn();
          nextPattern = group.firstPattern = FirstBitOn() + curBank * NBR_PATTERN;
          //SYNC while running : the current measure finishes first. If it is already
          //the chain's first pattern it counts as the chain's first step, otherwise
          //it would be played twice. Outside SYNC the first pattern is loaded right
          //away, so the next index to play is 1 either way.
          if (isRunning && (seq.ptrnChangeSync == SYNC))
            group.pos = (curPattern == group.firstPattern) ? 1 : 0;
          else group.pos = 1;
          doublePush = TRUE;
          group.priority = TRUE;
        }
        //Only one pattern selected
        else if (!doublePush){
          group.priority = FALSE;
          group.length = 0;//should be 0 to play the right next pattern
          nextPattern = FirstBitOn() + curBank * NBR_PATTERN;
          group.pos = pattern[ptrnBuffer].groupPos;
        }
        if(curPattern != nextPattern && stepsBtn.justPressed) {  // [zabox] [1.027] fixes pattern change bug in slave mode
          if(seq.ptrnChangeSync == SYNC){
            if (isRunning) needLcdUpdate = TRUE;//selected pattern changed so we need to update display 
            else selectedPatternChanged = TRUE;
          }
          if(seq.ptrnChangeSync == FREE){
            selectedPatternChanged = TRUE;
          }
        }
      }
    }

    //pattern next update : cf PatternChainUpdate(), called from the common part

  }//END IF MODE PLAY PATTERN

  //===================================================================================================================================

  //===================================================================================================================================

  //////////////////////////MODE TRACK WRITE/////////////////////////////////

  if (curSeqMode == TRACK_WRITE){
    //-------------------------------select pattern-----------------------------------
    if (stepsBtn.justPressed){

      if (bankBtn.pressed){
        byte bank = FirstBitOn();//9 to 16 are not banks : cf the PTRN_STEP site above
        if (bank < NBR_BANK){
          curBank = bank;
          nextPattern = curBank * NBR_PATTERN + (curPattern % NBR_PATTERN);
          if(curPattern != nextPattern) selectedPatternChanged = TRUE;
        }
      }
      else if(numBtn.pressed){
        trk.next = FirstBitOn();
        selectedTrackChanged = TRUE;
        needLcdUpdate = TRUE;
      }
      else{
        nextPattern = FirstBitOn() + curBank * NBR_PATTERN;
        if(curPattern != nextPattern) selectedPatternChanged = TRUE;
      }
    }
    //decremente track position
    //SHIFT + BACK is the "delete" shortcut : it must not move the position here
    if (backBtn.justPressed && !shiftBtn.pressed){
      if (trk.pos) trk.pos--;//trk.pos is unsigned, never let it wrap under 0
      TrackPosToPattern();
      needLcdUpdate = TRUE;
    }
    //incremente track position
    if (fwdBtn.justPressed && !shiftBtn.pressed){
      trk.pos++;
      //La position s'arrete au premier emplacement vide, pas a 999. Sans cette borne on
      //pouvait aller en position 12 d'une piste de 8 et y faire ENTER : la longueur
      //passait a 12 et les positions 9 a 11 gardaient ce qui trainait en memoire. Un trou,
      //la ou la troncature corrigee le 13/09 coupait.
      //La borne est track[].length et non length - 1 : les positions remplies vont de 0 a
      //length - 1, donc length est le premier emplacement libre, celui ou l'on ajoute.
      //C'est exactement la ou SHIFT + NUM emmene, "go to the last measure" : les deux
      //gestes disent maintenant la meme chose.
      if (trk.pos > track[trkBuffer].length) trk.pos = track[trkBuffer].length;
      if (trk.pos > MAX_TRACK_POS) trk.pos = MAX_TRACK_POS;//garde-fou si la piste est corrompue
      TrackPosToPattern();
      needLcdUpdate = TRUE;
    }
    //go to first measure
    if (clearBtn.justPressed){
      trk.pos = 0;
      TrackPosToPattern();
      needLcdUpdate = TRUE;
    }
    if(shiftBtn.pressed){
      //go to last measure
      if (numBtn.pressed){
        trk.pos = track[trkBuffer].length;
        if (trk.pos > MAX_TRACK_POS) trk.pos = MAX_TRACK_POS;
        TrackPosToPattern();
        needLcdUpdate = TRUE;
      }
      //delete the pattern at the current position
      //Vider une piste depuis la fin demandait une pression gaspillee a chaque
      //suppression. Deux causes qui se cumulaient, signalees en jouant :
      //  1. ENTER laisse le curseur sur l'emplacement d'AJOUT, celui qui suit la derniere
      //     position remplie. Il n'y a rien a y supprimer, donc la touche ne faisait rien.
      //  2. Apres une suppression, le curseur retombait sur le NOUVEL emplacement
      //     d'ajout, et la suppression suivante ne trouvait rien non plus.
      //Trois regles individuellement justes - ENTER avance, supprimer refuse le vide,
      //supprimer ramene au vide - qui se rencontraient mal. Le curseur reste desormais
      //sur une position reelle avant comme apres, donc SHIFT+BACK repete vide la piste
      //depuis la fin, une pression par position.
      //Au milieu de la piste rien ne change : supprimer en position 2 d'une piste de 8
      //laissait deja le curseur sur une position remplie.
      if (backBtn.justPressed && track[trkBuffer].length){
        //Sur l'emplacement d'ajout, reculer d'abord : "supprimer ici" n'a de sens que sur
        //une position reelle. C'est exactement le BACK que la main faisait avant.
        if (trk.pos >= track[trkBuffer].length) trk.pos = track[trkBuffer].length - 1;
        //Every index is unsigned here : the loop can never walk below
        //patternNbr[0], just under which the two length bytes live (cf SaveTrack)
        for (unsigned int a = trk.pos; a < track[trkBuffer].length - 1; a++){
          track[trkBuffer].patternNbr[a] = track[trkBuffer].patternNbr[a + 1];
        }
        track[trkBuffer].length--;//decremente length by 1 due to deleted pattern
        //Et rester sur une position reelle apres coup, plutot que de retomber sur le vide.
        if (!track[trkBuffer].length) trk.pos = 0;
        else if (trk.pos >= track[trkBuffer].length) trk.pos = track[trkBuffer].length - 1;
        TrackPosToPattern();
        trackNeedSaved = TRUE;
        needLcdUpdate = TRUE;
      }
      //insert a pattern at the current position
      if (fwdBtn.justPressed && trk.pos <= track[trkBuffer].length
          && track[trkBuffer].length < MAX_TRACK_POS){
        for (unsigned int a = track[trkBuffer].length; a > trk.pos; a--){
          track[trkBuffer].patternNbr[a] = track[trkBuffer].patternNbr[a - 1];
        }
        track[trkBuffer].patternNbr[trk.pos] = curPattern;
        track[trkBuffer].length++;//incremente length by 1 due to inserted pattern
        TrackPosToPattern();
        trackNeedSaved = TRUE;
        needLcdUpdate = TRUE;
      }
    }//end shift

    //write selected pattern in the current track position
    if (enterBtn.justRelease && !trackJustSaved){
      track[trkBuffer].patternNbr[trk.pos] = curPattern;
      trk.pos++;
      if (trk.pos > MAX_TRACK_POS) trk.pos = MAX_TRACK_POS;
      TrackPosToPattern();
      //Le manuel dit, pour ENTER en TRACK EDIT : "store current to the position and
      //increase Track position". Rien sur la longueur - et len est justement un des
      //quatre champs qu'on edite soi-meme a la molette (pos, ptr, len, num), donc
      //raccourcir une piste est une action deliberee, pas un effet de bord.
      //Assignee sans condition, elle ramenait la piste a la position courante : reecrire
      //en position 4 d'une piste de 20 la coupait a 4, et les seize suivantes partaient
      //en EEPROM comme ca au premier ENTER maintenu. Elle ne fait plus que s'etendre.
      if (trk.pos > track[trkBuffer].length) track[trkBuffer].length = trk.pos;
      trackNeedSaved = TRUE;
      needLcdUpdate = TRUE;
    }
  }//END IF MODE TRACK WRITE

    //////////////////////////MODE TRACK PLAY/////////////////////////////////
  if (curSeqMode == TRACK_PLAY)
  {
    //track position update : cf TrackPlayUpdate(), called from the common part
    //go to first measure
    if (clearBtn.justPressed){
      trk.pos = 0;
      TrackPosToPattern();
      needLcdUpdate = TRUE;
    }
    if(readButtonState){
      trk.next = FirstBitOn();
      selectedTrackChanged = TRUE;
      needLcdUpdate = TRUE;
    }
  }

  //=======================================================================================
  //------------------------------------Update pattern track load/save-----------------------
  if (selectedTrackChanged)
  {
    selectedTrackChanged = FALSE;
    needLcdUpdate = TRUE;
    trackNeedSaved = FALSE;
    LoadTrack(trk.next);
    trk.current = trk.next;
    TrackPosToPattern();
    //trkBuffer = !trkBuffer;
  }
  if (trackNeedSaved && enterBtn.hold)
  {
    trackNeedSaved = FALSE;
    SaveTrack(trk.current);
    LcdPrintSaved();
    trackJustSaved = TRUE;
    timeSinceSaved = millis();
  }
  //this function is to not incremente trk.pos when released enterBtn after Saved track
  if (millis() - timeSinceSaved > HOLD_TIME){
    trackJustSaved = FALSE;
  }

  //--------------------------------playback advance---------------------------------
  //MUTE only takes over the step buttons, it is NOT a transport mode. Gating the
  //advance on curSeqMode froze playback on the pattern that was running when MUTE
  //was entered, so drive it from the mode that is actually playing instead.
  SeqMode playMode = (curSeqMode == MUTE) ? prevSeqMode : curSeqMode;
  if (playMode == PTRN_STEP || playMode == PTRN_PLAY) PatternChainUpdate(playMode);
  else if (playMode == TRACK_PLAY) TrackPlayUpdate();

  if (selectedPatternChanged)
  {
    selectedPatternChanged = FALSE;
    needLcdUpdate = TRUE;//selected pattern changed so we need to update display
    patternNeedSaved = FALSE;
    LoadPattern(nextPattern);
    curPattern = nextPattern;
    //curBank est a la fois la banque affichee et celle qui sert a la selection au pas
    //(nextPattern = touche + curBank * NBR_PATTERN). Elle n'etait ecrite que par le geste
    //BANK, donc elle restait en arriere des que le pattern changeait par un autre chemin :
    //une position de piste, une chaine, un bouton de pas. Arriver sur D13 depuis TRACK
    //EDIT puis passer en PATT EDIT affichait "A61" - la lettre venait d'un curBank reste
    //a 0, et le numero valait 60 - 0 + 1.
    //Les ecrans de TRACK ne tombaient pas dedans parce qu'ils derivent la banque du
    //pattern au lieu de la lire. Ici on recale la source, une seule fois, a l'unique
    //endroit du firmware ou curPattern est ecrit - donc l'affichage ET la selection au
    //pas repartent du meme etat. Appuyer sur la touche 3 en etant sur D13 donne D3, plus
    //A3.
    curBank = curPattern / NBR_PATTERN;
    nextPatternReady = TRUE;
  }

  if (nextPatternReady) SwitchPatternBuffer();

  if (patternWasEdited)
  {//update Pattern
    patternWasEdited = FALSE;
    InstToStepWord();
    patternNeedSaved = TRUE;
  }

  if (patternNeedSaved && enterBtn.justPressed && !instBtn)
  {
    patternNeedSaved = FALSE;
    SavePattern(curPattern);//pattern saved
    LcdPrintSaved();
  }

  if (enterBtn.justRelease) needLcdUpdate = TRUE;

  //////////////////////////MODE MUTE//////////////////////////////////////

  if (curSeqMode == MUTE){
    MuteButtonGet();
    if (encBtn.pressed){
      muteInst = 0;
      InitMuteBtnCounter();
    }
  }  
}










































































