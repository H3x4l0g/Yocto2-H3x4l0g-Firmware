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

  readButtonState = StepButtonGet();
  //can not access config when isRunning
  if (isRunning && seq.configMode){
    seq.configMode = FALSE;
    needLcdUpdate = TRUE;
  }
  if (curSeqMode != MUTE) muteBtn.counter = 0;
  if (!seq.configMode) seq.configPage = 0;

  //-------------------Encoder button---------------------------
  if(encBtn.justPressed){
    //LA PAGE 2 DU MODE CONFIG N'A QU'UN CHAMP. Elle affiche "pCh nc. nc. nc." : un seul
    //reglage, la synchro des changements de pattern, et trois emplacements vides que
    //l'amont a laisses a l'ecran. La molette y est inerte depuis la v2.053, donc s'y
    //arreter ne casse plus rien - mais le curseur disparaissait trois fois sur quatre
    //sous un "nc.", ce qui se lit comme une page en panne plutot que comme une page
    //sans champs. Trois arrets qui ne menent nulle part restent trois arrets.
    //MAX_CUR_POS est partage avec TRACK WRITE, le clavier et l'ecran des conditions :
    //la borne se pose donc ICI, la ou le curseur avance, et pas dans la table.
    //Changer de page remet deja curIndex a zero (branche SHIFT + TEMPO), donc rien ne
    //peut rester hors plage en arrivant sur la page 2.
    //La page 2 des conditions n'a que trois champs - pas, taux, instrument - tant que le
    //micro-timing n'agit pas. Meme raison que la page 2 du config juste au-dessus.
    byte fields = (seq.configMode && seq.configPage == 2) ? 1 : MAX_CUR_POS;
    curIndex++;
    if (curIndex >= fields)  curIndex = 0;
    needLcdUpdate = TRUE;
  }
  //-------------------play button---------------------------
  if (playBtn.justPressed || midiStart){
    isRunning = TRUE;
    isStop = FALSE;
    ppqn = 0;
    SlaveClockIdle();//les sous-ticks armes appartenaient a la phase qu'on vient d'annuler
    //Les conditions de trig repartent de la premiere mesure du cycle. Sans ca, un 1:4
    //tomberait a un endroit different a chaque PLAY - la meme chose a l'oreille qu'une
    //machine qui ne repete pas. Le masque est refait dans la foulee, pour que la mesure
    //qui demarre soit deja jugee. Sous cli() : condPassMask fait deux octets et
    //l'interruption le lit.
    {
      uint8_t sregC = SREG;
      cli();
      measureCount = 0;
      CondUpdateMask();
      //Les sous-coups encore dus appartenaient au pas qu'on vient d'abandonner : les
      //laisser ferait partir une queue de roulement sur le premier temps.
      for (byte z = 0; z < NBR_INST; z++) subTrig[z] = 0;
      SREG = sregC;
    }
    globalStepCount = 0;
    for(byte z = 0; z < NBR_INST; z++){
      stepCount[z] = 0;
      changeDir[z] = 1;//restart Forward direction on each instrument
    }
    tapStepCount = 0;
    stopBtn.counter = 0;
    //RELACHER AVANT DE REMETTRE L'INDEX A ZERO. Appuyer sur PLAY pendant qu'une note EXT
    //sonne la laissait en l'air : plus rien ensuite ne savait laquelle couper. C'est
    //ferme des deux cotes maintenant - la hauteur est retenue (cf InitMidiNoteOff), et on
    //relache ici plutot que d'attendre le prochain pas EXT.
    InitMidiNoteOff();
    noteIndexCpt = 0;//init ext instrument note index counter
    MIDI.sendRealTime(midi::Start);//;MidiSend(START_CMD);
    DIN_START_HIGH;
    dinStartState = HIGH;
    DIN_CLK_HIGH;
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
      //Les sous-coups encore dus meurent avec le transport. Sans ca, CONTINUE reprendrait
      //par la queue d'un roulement interrompu - l'interruption ne touche plus subTrig une
      //fois isRunning tombe, mais elle peut etre en train de le faire a cet instant meme.
      {
        uint8_t sregR = SREG;
        cli();
        for (byte z = 0; z < NBR_INST; z++) subTrig[z] = 0;
        SREG = sregR;
      }
      MIDI.sendRealTime(midi::Stop);//;MidiSend(STOP_CMD);
      DIN_START_LOW;
      dinStartState = LOW;
      break;
    case 2:
      isStop = FALSE;
      isRunning = TRUE;
      stopBtn.counter = 0;
      ppqn = 0;
      SlaveClockIdle();//cf le bouton PLAY
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
        keyboardMode = condMode = FALSE;
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
        keyboardMode = condMode = FALSE;
        seq.configMode  = FALSE;
      }
    }
    // if (backBtn.justPressed) ;//back  track postion
    //if (fwdBtn.justPressed) ;//foward track postion
    if (ptrnBtn.justPressed){
      SeqMode from = (curSeqMode == MUTE) ? prevSeqMode : curSeqMode;//cf trkBtn above
      if (!IS_TRACK_MODE(from) || !isRunning){//transport rule : cf trkBtn above
        if (from == PTRN_STEP)      curSeqMode = PTRN_PLAY;//toggle inside the family
        else if (from == PTRN_PLAY) curSeqMode = PTRN_STEP;
        else curSeqMode = (from == TRACK_PLAY) ? PTRN_PLAY : PTRN_STEP;//crossing over
        needLcdUpdate = TRUE;
        keyboardMode = condMode = FALSE;
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
        curSeqMode = MUTE;
        break;
      case 2:
        curSeqMode = prevSeqMode;
        muteBtn.counter = 0;
        break;
      }
      //Ni l'entree ni la sortie ne redessinaient l'ecran : rien n'annoncait le mode, et
      //rien ne l'effacait non plus. C'est ce qui rendait l'oubli possible.
      needLcdUpdate = TRUE;
    }
    //Les deux lignes qui etaient ici repetaient mot pour mot le haut de la fonction,
    //qui tourne a chaque passe AVANT elles. Le cas 2 ci-dessus remet deja le compteur a
    //zero, et configPage n'est incremente que dans la branche SHIFT, hors d'atteinte.
    //L'appui redessine comme le relachement : a l'appui pour que la valeur du tempo
    //apparaisse, au relachement pour rendre l'ecran du dessous. Seul le relachement etait
    //la, le bloc tempo de LcdUpdate() se redessinant alors a chaque passe sans rien
    //demander a personne. Maintenant qu'il respecte le drapeau, l'appui doit le poser.
    if (tempoBtn.justPressed || tempoBtn.justRelease) needLcdUpdate = TRUE;
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
      keyboardMode = condMode = FALSE;
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
      pattern[ptrnBuffer].probality[curInst][curStep[curInst]] = 0;//la condition aussi
      pattern[ptrnBuffer].retrig[curInst][curStep[curInst]] = 0;//et le roulement
      pattern[ptrnBuffer].uTiming[curInst][curStep[curInst]] = 0;//et le decalage
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
        //Les conditions de trig partent avec le reste. Une condition sans trig ne fait
        //rien, donc les laisser ne s'entend pas tout de suite : ca s'entend le jour ou
        //l'on reecrit des pas sur le pattern efface et ou ils sortent conditionnes par un
        //reglage qu'on ne se souvient pas d'avoir pose. C'est le meme raisonnement que les
        //trois champs globaux ci-dessous - ce qui reste d'avant est ce qui piege.
        for (byte j = 0; j < NBR_STEP; j++){
          pattern[ptrnBuffer].probality[a][j] = 0;
          pattern[ptrnBuffer].retrig[a][j] = 0;
          pattern[ptrnBuffer].uTiming[a][j] = 0;
        }
      }
      //Les trois champs "globaux" sont le resume des trois tableaux qu'on vient de
      //reecrire, et ils restaient sur l'ancien pattern.
      //globalLength decide de la FIN DE MESURE (Clock.ino) : effacer un pattern de huit
      //pas donnait seize pas par instrument et une mesure qui finissait toujours au
      //huitieme - donc des chaines et des pistes qui avancaient au milieu du pattern. Et
      //comme CLEAR pose patternWasEdited, la valeur fausse partait en EEPROM au premier
      //ENTER : le pattern restait infirme apres extinction.
      //globalScale fait avancer tapStepCount (Clock.ino) : efface depuis une grille de
      //32e, le compteur du mode TAP gardait cette grille-la alors que les seize
      //instruments etaient repasses en 16e.
      //globalShuffle sert de point de depart a la molette de shuffle global (Enc.ino) :
      //apres un effacement il la faisait repartir de l'ancienne valeur au lieu de
      //DEFAULT_SHUF. Ajoute le 14/09 apres les deux autres - il avait ete manque parce
      //qu'on avait corrige les deux champs trouves sans chercher s'il y en avait un
      //troisieme. La boucle ci-dessus ecrit trois tableaux, il fallait trois resumes.
      //Les memes valeurs que cette boucle, pas d'autres : NBR_STEP - 1, SCALE_16 et
      //DEFAULT_SHUF. UpdateLongerInst() derriere, parce que longerInstLength designait
      //encore l'instrument le plus long d'AVANT - cf son en-tete dans SeqFunc.ino.
      pattern[ptrnBuffer].globalLength = NBR_STEP - 1;
      pattern[ptrnBuffer].globalScale = SCALE_16;
      pattern[ptrnBuffer].globalShuffle = DEFAULT_SHUF;
      UpdateLongerInst();
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
      //CHANGER UNE SCALE DEPLACE LA GRILLE, donc il faut remettre l'instrument dessus.
      //LAST STEP le fait depuis toujours pour la longueur ; la scale, elle, ne
      //resynchronisait rien : le compteur restait ou il etait et se mettait a avancer a
      //une autre cadence, si bien que le pas 1 ne retombait plus sur le temps. Il
      //fallait arreter et refaire PLAY, qui remet les seize compteurs a zero.
      //Les scales s'ecrivent toutes AVANT la resynchro : MeasureTicks() lit celle de
      //l'instrument de reference, et elle vient peut-etre de changer elle aussi.
      if (shiftBtn.pressed){
        pattern[ptrnBuffer].globalScale = a;
        //init all scale of each instrument when globalscale changed
        for (byte z = 0; z < NBR_INST; z++){
          pattern[ptrnBuffer].scale[z] = pattern[ptrnBuffer].globalScale;
        }
        for (byte z = 0; z < NBR_INST; z++) ResyncStepCount(z);
      }
      else {
        pattern[ptrnBuffer].scale[curInst] = a;
        ResyncStepCount(curInst);
      }
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
        //set the highest length value as global length
        //Le "longerInstLength = z" qui vivait dans cette boucle etait mort : la ligne
        //suivante le reecrit, et UpdateLongerInst() est la seule a savoir comment on
        //choisit la reference.
        for (byte z = 0; z < NBR_INST; z++){
          if (pattern[ptrnBuffer].length[z] > pattern[ptrnBuffer].globalLength){
            pattern[ptrnBuffer].globalLength = pattern[ptrnBuffer].length[z];
          }
        }
        UpdateLongerInst();
        ResyncStepCount(curInst);//keep it in phase with the grid
      }
      else if (shiftBtn.pressed){
        pattern[ptrnBuffer].globalLength = FirstBitOn();
        for (byte z = 0; z < NBR_INST; z++){
          pattern[ptrnBuffer].length[z] = pattern[ptrnBuffer].globalLength;
        }
        //LA REFERENCE D'ABORD, LES RESYNCHROS ENSUITE. Elles se faisaient dans la boucle
        //d'ecriture, donc avant qu'UpdateLongerInst() n'ait vu les nouvelles longueurs :
        //les quinze premieres se calaient sur la scale de l'ancienne reference.
        UpdateLongerInst();
        for (byte z = 0; z < NBR_INST; z++) ResyncStepCount(z);
      }
      needLcdUpdate = TRUE;
      patternWasEdited = TRUE;
    }
    //-------------------Steps buttons------------------------------
    /////////////////////////////STEP EDIT///////////////////////////
    if (curSeqMode == PTRN_STEP){
      if (stepsBtn.justRelease) doublePush = FALSE;

      //SHIFT + UNE TOUCHE DE PAS : L'ECRAN DES CONDITIONS DE CE PAS-LA.
      //Le geste etait libre au sens strict : rien ici ne regardait SHIFT, donc SHIFT +
      //pas faisait exactement ce que fait un pas seul - editer le pas en marche, choisir
      //le pattern a l'arret. Personne ne tient SHIFT pour ca.
      //Les trois modificateurs qui donnent DEJA un sens a SHIFT + pas sont exclus, et
      //c'est ce qui rend le geste sur : LAST STEP (la longueur de toutes les voix), INST
      //(le choix de l'instrument) et BANK (SHIFT + BANK est le copier de pattern).
      //Sur le meme pas, le geste referme l'ecran ; sur un autre, il y saute. Il n'y a
      //donc pas de touche de sortie a apprendre - on sort par ou l'on est entre.
      if (shiftBtn.pressed && stepsBtn.justPressed && !lastStepBtn.pressed
          && !instBtn && !bankBtn.pressed && !keyboardMode){
        byte s = FirstBitOn();
        //SUR LE MEME PAS, LE GESTE AVANCE PUIS SORT : conditions, puis retrig, puis rien.
        //C'est la forme de SHIFT + TEMPO sur les pages du mode config, et la machine n'a
        //pas besoin d'une touche de sortie de plus. Sur un AUTRE pas, il y saute comme
        //avant, et repart de la premiere page - on designe un pas pour le regler, pas
        //pour continuer une navigation.
        if (condMode && s == condStep){
          if (condPage == 0) condPage = 1;
          else {
            condMode = FALSE;
            condPage = 0;
          }
        }
        else {
          condMode = TRUE;
          condStep = s;
          condPage = 0;
        }
        curIndex = 1;//le curseur arrive sur la condition, pas sur le pas qu'on vient de designer
        needLcdUpdate = TRUE;
      }
      //SHIFT exclut la suite : sans ca, maintenir SHIFT apres l'ouverture continuerait
      //d'editer le pas sous le doigt a chaque tour de boucle.
      if(!lastStepBtn.pressed && !instBtn && !keyboardMode && !shiftBtn.pressed){
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
              //LE GESTE ARME LA BANQUE, IL NE JOUE RIEN. Il chargeait le pattern de
              //meme rang dans la nouvelle banque - depuis A3, choisir D donnait D3 -
              //et annulait la chaine au passage. Basculer de banque est tout ce qu'il
              //fait : la touche de pas qui suit choisit le pattern, et c'est elle qui
              //decide du sort de la chaine, cf la branche ci-dessous.
              byte bank = FirstBitOn();
              if (bank < NBR_BANK) armedBank = bank;
            }
            else{//pattern group edit------------------------------------------------------
              if (SecondBitOn())
              {
                trackPosNeedIncremante = FALSE;                                                               // [zabox] fixes group bug
                group.length = SecondBitOn() - FirstBitOn();
                nextPattern = group.firstPattern = FirstBitOn() + SelectionBank() * NBR_PATTERN;
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
                nextPattern = FirstBitOn() + SelectionBank() * NBR_PATTERN;
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
              //Le tableau des douze cas ne disait rien : BD_BTN vaut 0 et BD vaut 0,
              //SD_BTN vaut 1 et SD vaut 1, et ainsi de suite jusqu'a PC2. Les douze cas
              //faisaient mot pour mot la meme chose, et instOut[] etait la table identite
              //qui allait avec - partie de define.h avec muxInst[], jamais lue nulle part.
              //Les seize touches portent les seize pistes, "a" etant directement le
              //numero de piste. Le switch n'avait pas de cas pour 12 a 15 : ces
              //quatre-la ne posaient aucune velocite. InstValueGet() en pose une pour
              //elles quand on edite au pas - instVelHigh vaut 80 sur les seize entrees -
              //et un pas pose en TAP doit etre indiscernable d'un pas pose au pas.
              if (isRunning) pattern[ptrnBuffer].velocity[a][tapStepCount] = instVelHigh[a];
              //tapVel[] n'est que la table du multiplexeur : douze voix, pas au-dela.
              if (a < NBR_TAP_INST) tapVel[a] = instVelHigh[a];

              //-----SET Velocity Values-----//
              //C'etait la QUATRIEME copie du balayage des douze adresses, celle que la
              //fusion des trois autres avait ratee - l'en-tete de Mux.ino n'en annoncait
              //que trois, il dit vrai maintenant. Quarante lignes qui refaisaient a la
              //main SelectFirstMux(), SelectSecondMux(), muxAddr[] et MuxAddr(), avec les
              //adresses en dur et les deux commentaires de multiplexeur intervertis.
              //Trois differences s'en vont avec, toutes dans le bon sens :
              //  - Le PORTA brut ECRASAIT TR1 ET TR2, douze fois par frappe. Le "1 << 2"
              //    remettait TR1 au repos (il est actif a l'etat bas, cf TR1_LOW) : une
              //    frappe relachait donc le gate d'une note 60 tenue en MIDI. Et le bit 3
              //    a zero remettait TR2 au repos, lui actif a l'etat haut.
              //    A NOTER : TR2_LOW n'est appele NULLE PART ailleurs dans le depot - le
              //    carnet le dit, l'impulsion monte et ne redescend jamais. Ce balayage
              //    etait donc, par accident, la seule chose qui la faisait retomber. Elle
              //    ne retombe plus. Sans consequence sur cette carte, TR2 s'arretant sur
              //    un cavalier qui ne va nulle part, mais c'est a savoir le jour ou on le
              //    cable.
              //    MuxAddr() ne touche qu'aux trois bits d'adresse, interruptions fermees.
              //  - Les 15 us de stabilisation de multiplexeur, absents ici, reviennent.
              //  - Le delayMicroseconds(100) marque "Coudl BE REMOVE" s'en va. Le compte
              //    total ne bouge pas : +12 x 5 us de charge, +2 x 20 us de selection,
              //    -100 us. La frappe dure autant qu'avant.
              SetMuxTap();

              //LES QUATRE DERNIERES TOUCHES. Elles passaient par instOut[12..15], jamais
              //initialise et valant donc 0 : les touches 13 a 16 tapaient toutes une
              //grosse caisse, et l'enregistraient. Elles jouent desormais les quatre
              //pistes sans voix, qui sont deja les instruments 12 a 15 et que le geste
              //INST + touche atteint par les memes indices.
              //Chacune fait ce que l'interruption du sequenceur fait pour elle, et rien
              //de plus - cf le bloc de declenchement de Clock.ino :
              //
              //  12 GLOBAL_ACC  N'EST PAS UN DECLENCHEMENT. L'interruption l'exclut
              //                 explicitement du mot de trigs ; il est lu par MuxValue()
              //                 pendant le balayage. La touche pose donc l'accent dans le
              //                 pattern, et ne sonne pas d'elle-meme.
              //  13 EXT_INST    une note de la pile part en MIDI, comme au sequenceur.
              //  14 TR1         actif a l'etat BAS. C'est SetTrigTime() qui le relache, a
              //                 chaque tour de boucle principale : rien a faire ici, et
              //                 surtout pas une seconde voie de relachement.
              //  15 TR2         actif a l'etat haut - et il NE RETOMBE PAS, TR2_LOW
              //                 n'etant ecrit nulle part. Le sequenceur est reproduit tel
              //                 quel plutot que repare ici seulement : le jour ou TR2
              //                 apprendra a retomber, ce sera au meme endroit pour les
              //                 deux. Cf carnet.
              if (a != GLOBAL_ACC){
                if (a == TR1){ TR1_LOW;  trig1State = HIGH; }
                if (a == TR2){ TR2_HIGH; trig2State = HIGH; }
                SetDoutTrig(1 << a);
              }
              if (a == EXT_INST){
                InitMidiNoteOff();
                //Meme regle que le tick : on BORNE, on releve l'index, on avance - et le
                //tout d'un bloc. noteIndexCpt est volatile et avance aussi dans
                //l'interruption ; sur AVR un ++ est lire-modifier-ecrire, donc un tick
                //tombant au milieu perdrait un cran. L'emission est faite APRES, hors des
                //interruptions fermees : elle ecrit sur le port serie, ce n'est pas un
                //endroit ou les tenir.
                byte sreg = SREG;
                cli();
                if (noteIndexCpt > pattern[ptrnBuffer].extLength) noteIndexCpt = 0;
                byte extIdx = noteIndexCpt;
                noteIndexCpt++;
                SREG = sreg;
                //instVelHigh[EXT_INST] et non HIGH_VEL : la frappe enregistre un pas
                //plein juste au-dessus, elle doit sonner comme ce qu'elle ecrit. En TAP
                //la piste EXT part donc accentuee, comme les douze voix y partent pleines.
                MidiExtNoteOn(pattern[ptrnBuffer].extNote[extIdx], instVelHigh[EXT_INST]);
              }
              if(isRunning) bitSet(tempInst[a],tapStepCount);
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
        //Armer, rien d'autre : cf le site PTRN_STEP ci-dessus. La chaine n'est plus
        //annulee ici non plus - c'est la touche de pas qui la reprend ou la jette.
        byte bank = FirstBitOn();//9 to 16 are not banks : cf the PTRN_STEP site above
        if (bank < NBR_BANK) armedBank = bank;
      }
      else{
        //Group selected
        if (SecondBitOn())
        {
          group.length = SecondBitOn() - FirstBitOn();
          nextPattern = group.firstPattern = FirstBitOn() + SelectionBank() * NBR_PATTERN;
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
          nextPattern = FirstBitOn() + SelectionBank() * NBR_PATTERN;
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
        //Armer, rien d'autre : cf le site PTRN_STEP ci-dessus.
        byte bank = FirstBitOn();//9 to 16 are not banks : cf the PTRN_STEP site above
        if (bank < NBR_BANK) armedBank = bank;
      }
      else if(numBtn.pressed){
        trk.next = FirstBitOn();
        selectedTrackChanged = TRUE;
        needLcdUpdate = TRUE;
      }
      else{
        nextPattern = FirstBitOn() + SelectionBank() * NBR_PATTERN;
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
    //readButtonState est un NIVEAU, pas un front : tant qu'une touche de pas restait
    //tenue, ce bloc reposait selectedTrackChanged a CHAQUE PASSE DE BOUCLE - donc un
    //LoadTrack() par passe, soit huit pages de 128 octets en I2C, ~23 ms a 400 kHz. Une
    //seconde de pression valait une quarantaine de rechargements : LED figees, molette
    //plus echantillonnee, et le tampon MIDI d'entree en debordement (23 ms a 31250 bauds
    //font 72 octets pour une file de 64). Le sequenceur, lui, continuait sans rien voir :
    //il tourne dans l'interruption.
    //Le niveau est GARDE plutot que remplace par un front, contrairement au site jumeau
    //de TRACK WRITE : il autorise de rouler d'une touche a l'autre sans relacher, et ce
    //geste-la n'a aucune raison de disparaitre. C'est le CHANGEMENT de piste qui
    //declenche la lecture, une seule fois - le handler de selectedTrackChanged pose
    //trk.current = trk.next juste apres, donc la condition retombe d'elle-meme.
    //Effet de bord assume : rappuyer sur la piste ou l'on est deja ne recharge plus rien.
    //Ca ne faisait que jeter en silence les editions de piste non sauvees - LoadTrack()
    //ecrase la RAM et le handler abaisse trackNeedSaved - sans que rien ne l'annonce.
    if (readButtonState){
      byte sel = FirstBitOn();
      if (sel != trk.current){
        trk.next = sel;
        selectedTrackChanged = TRUE;
        needLcdUpdate = TRUE;
      }
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
    //LA BANQUE ARMEE EST CONSOMMEE QUAND LE PATTERN ARRIVE DEDANS. Au-dela elle ferait
    //doublon avec PlayingBank(), et la selection au pas repart de ce que l'ecran montre :
    //appuyer sur la touche 3 en etant sur D13 donne D3, plus A3.
    //Elle SURVIT en revanche a un chargement venu d'ailleurs - une avance de chaine, un
    //pas de piste - sinon la machine qui tourne annulerait le geste en silence, pile
    //quand on s'en sert : a tempo de jeu une chaine charge un pattern toutes les deux
    //mesures.
    //C'est ici et nulle part ailleurs, parce que curPattern n'est ecrit qu'ici. Le
    //recalage d'affichage qui tenait cette ligne - le "A61" du 13/09 - est parti avec la
    //variable : la lettre se derive maintenant, elle ne peut plus rester en arriere.
    if (armedBank == PlayingBank()) armedBank = NO_BANK;
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
    //TOUT REVIENT : LES MUTES ET LE SOLO. C'est la meme question - quelles voix on
    //entend - et laisser l'un des deux debout apres un geste qui veut dire "efface"
    //est la facon de chercher une minute pourquoi la machine reste presque muette.
    //justPressed et non pressed : au niveau, la remise a zero repartait a CHAQUE tour
    //de boucle tant que l'encodeur restait enfonce, donc un pad presse pendant ce
    //temps etait defait aussi vite que pose et la rangee refusait toute edition
    //jusqu'a ce qu'on lache. Le firmware MSS a corrige exactement ce defaut-la.
    if (encBtn.justPressed){
      muteInst = 0;
      soloInst = 0;
      InitMuteBtnCounter();
    }
  }  
}










































































