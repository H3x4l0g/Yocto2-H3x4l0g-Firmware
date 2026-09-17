//======================================================================================
//
//   K E Y . I N O                              les seize touches en clavier
//
//   Dans ce mode les touches de pas ne sont plus des pas mais des notes, et BACK / FWD
//   parcourent la pile de notes de l'instrument externe en la faisant sonner.
//
//--------------------------------------------------------------------------------------
//   AVANT D'Y TOUCHER
//
//   Toute note envoyee doit etre relachee, et avec SA valeur. Le note-off relisait la
//   note a l'index COURANT au moment du lacher : maintenir BACK puis appuyer sur FWD
//   coupait donc une autre note, et celle de BACK sonnait indefiniment. La note est
//   desormais memorisee a l'appui et relue telle quelle.
//
//======================================================================================

/////////////////////Function//////////////////////
void KeyboardUpdate()
{
  if (isRunning && keyboardMode){
    keyboardMode = 0;
    needLcdUpdate = TRUE;
  }
  
  if (numBtn.justPressed && curInst == EXT_INST && curSeqMode == PTRN_STEP){
    keyboardMode = !keyboardMode;
    //stop sequencer when keyboard mode ON
    if (keyboardMode){
      //stepBtn[].pressed belongs to this mode alone now : sole reader, sole writer, and
      //it needs it to pair a note off with its note on. Initialised on entry rather than
      //cleaned up on exit - the mode is left from four different places, the entry is
      //single. A stale HIGH here would swallow the first note on and emit a note off
      //for a note that never started.
      for (byte a = 0; a < NBR_STEP_BTN; a++) stepBtn[a].pressed = LOW;
      InitMidiNoteOff();
      isStop = TRUE;
      isRunning = FALSE;
      MIDI.sendRealTime(midi::Stop);//;MidiSend(STOP_CMD);
      stopBtn.counter = 1;//like a push on stop button
    }
    needLcdUpdate = TRUE;
  }

  /////////////////////////////KeyboardMode//////////////////////////////
  if(keyboardMode)
  {
    if (scaleBtn.justPressed){
      keybOct++;
      if (keybOct >= MAX_OCT) keybOct = MAX_OCT - 1;
      needLcdUpdate = TRUE;
    }
    if (lastStepBtn.justPressed){
      keybOct--;
      //keybOct est un byte : "< 0" ne peut pas etre vrai. Le decompte sous zero
      //reboucle vers 255, et c'est la borne HAUTE qui le rattrapait - correct, mais par
      //accident. Ecrit ici comme il fonctionne.
      if (keybOct > MAX_OCT - 1) keybOct = 0;
      needLcdUpdate = TRUE;
    }
    //BACK et FWD ecoutent la pile en la parcourant. Le note-off relisait
    //extNote[noteIndex] AU RELACHEMENT, donc changer d'index entre l'appui et le lacher -
    //maintenir BACK puis appuyer sur FWD - envoyait le note-off sur une autre note, et
    //celle de BACK sonnait indefiniment. La note envoyee est memorisee a l'appui et
    //relue telle quelle au lacher : une note-on, une note-off, toujours la meme.
    static byte backNote, fwdNote;
    //Et une note par touche de pas : le note-off recalculait a + 12*keybOct au
    //relachement. Tenir une touche, monter l'octave a SCALE, relacher - le note-off
    //partait une octave plus haut et la note tenue sonnait indefiniment. Troisieme
    //exemplaire du meme defaut apres BACK/FWD juste en dessous et le note-off de la piste
    //EXT : on relache ce qu'on a envoye, jamais ce qu'on recalcule.
    static byte keyNote[NBR_STEP_BTN];
    if (backBtn.justPressed){
      noteIndex--;
      //Meme chose : noteIndex est un byte, le reboucle vers 255 est rattrape par la
      //borne haute.
      if (noteIndex > MAX_EXT_INST_NOTE - 1) noteIndex = 0;
      backNote = pattern[ptrnBuffer].extNote[noteIndex];
      MidiSendNoteOn(seq.TXchannel, backNote, HIGH_VEL);
      needLcdUpdate = TRUE;
    }
    if (backBtn.justRelease)MidiSendNoteOff(seq.TXchannel, backNote);

    if (fwdBtn.justPressed){
      noteIndex++;
      //FWD S'ARRETE A L'EMPLACEMENT D'AJOUT, comme sur les pistes. Il montait jusqu'a 99
      //quelle que soit la pile : poser une note a l'index 40 d'une pile de cinq portait
      //extLength a 39 et laissait les index 5 a 38 pleins de ce qui trainait en memoire.
      //C'est le "trou au bout de la piste" du 14/09, sur l'autre pile.
      //extLength est l'index de la DERNIERE note, donc l'emplacement d'ajout est le
      //suivant - la ou une touche de pas allonge la pile d'un cran.
      byte addSlot = ExtAddSlot();
      if (noteIndex > addSlot) noteIndex = addSlot;
      fwdNote = pattern[ptrnBuffer].extNote[noteIndex];
      MidiSendNoteOn(seq.TXchannel, fwdNote, HIGH_VEL);
      needLcdUpdate = TRUE;
    }
    if (fwdBtn.justRelease)MidiSendNoteOff(seq.TXchannel, fwdNote);

    if(clearBtn.justPressed){
      noteIndex = 0;
      needLcdUpdate = TRUE;
    }

    if (isStop){
      for (byte a = 0; a < NBR_STEP_BTN; a++){
        stepBtn[a].curState = bitRead(readButtonState,a);
        if (stepBtn[a].curState != stepBtn[a].prevState){
          if ((stepBtn[a].pressed == LOW) && (stepBtn[a].curState == HIGH)){
            keyNote[a] = a + (12 * keybOct);
            pattern[ptrnBuffer].extNote[noteIndex] = keyNote[a];
            MidiSendNoteOn(seq.TXchannel, keyNote[a], HIGH_VEL);
            noteIndex++;
            patternWasEdited = TRUE;
            if(noteIndex > MAX_EXT_INST_NOTE) noteIndex = MAX_EXT_INST_NOTE;
            //Meme defaut, meme forme, meme correctif que la longueur de piste : le
            //manuel dit "Store played note in the sequence, send Midi note On and
            //Increase note index", rien sur la longueur, et len est un des quatre champs
            //edites a la molette (idx, not, len, oct). Revenir a la note 3 d'une pile de
            //vingt et en rejouer une la coupait a 4.
            if (noteIndex - 1 > pattern[ptrnBuffer].extLength) pattern[ptrnBuffer].extLength = noteIndex - 1;
            if(pattern[ptrnBuffer].extLength > MAX_EXT_INST_NOTE)pattern[ptrnBuffer].extLength = 0;
            needLcdUpdate = TRUE;
          }  
          if ((stepBtn[a].pressed == HIGH) && (stepBtn[a].curState == LOW)){
            MidiSendNoteOff(seq.TXchannel, keyNote[a]);//la note envoyee, pas une recalculee    
          }
          stepBtn[a].pressed = stepBtn[a].curState;
        }
        stepBtn[a].prevState = stepBtn[a].curState;
      }  
    }
  }
}








