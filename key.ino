//-------------------------------------------------
//                  Yocto v2.x
//                 keyboard mode
//-------------------------------------------------

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
      if( noteIndex >= MAX_EXT_INST_NOTE ) noteIndex = MAX_EXT_INST_NOTE;
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
            pattern[ptrnBuffer].extNote[noteIndex] = a + (12* keybOct);
            MidiSendNoteOn(seq.TXchannel, a + 12*keybOct, HIGH_VEL);
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
            MidiSendNoteOff(seq.TXchannel, a + 12*keybOct);    
          }
          stepBtn[a].pressed = stepBtn[a].curState;
        }
        stepBtn[a].prevState = stepBtn[a].curState;
      }  
    }
  }
}








