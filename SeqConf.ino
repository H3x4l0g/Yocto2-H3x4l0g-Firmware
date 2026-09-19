//======================================================================================
//
//   S E Q C O N F . I N O                      les reglages de la machine
//
//   Le mode config, atteint par SHIFT + GUIDE : canaux MIDI, tempo par defaut, source
//   de synchro, synchronisation des changements de pattern. Trois pages, parcourues a
//   la molette. Ces valeurs vivent dans la structure seq et sont enregistrees a part
//   des patterns, dans la zone de setup de l'EEPROM.
//
//======================================================================================

/////////////////////Function//////////////////////
void SeqConfiguration()
{
  if(seq.syncChanged){
    //Sync configuration
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
    seq.syncChanged = FALSE;
  }
  if (seq.setupNeedSaved && enterBtn.justPressed){
    SaveSeqSetup();
    seq.setupNeedSaved = FALSE;
    LcdPrintSaved();
  }
  //QUITTER SANS SAUVER LE DIT MAINTENANT. Les reglages sont appliques en direct, donc
  //tout marche - jusqu'a l'extinction, ou ils disparaissent. La LED d'ENTER clignotait
  //bien tant qu'on etait dans le mode (Led.ino), mais elle s'eteint avec lui :
  //l'avertissement partait a l'instant exact ou il devenait utile.
  //Le message tient jusqu'au prochain redessin, comme "[SAVED!]" - on sort du mode au
  //tour precedent, donc l'ecran du dessous est deja repeint et needLcdUpdate retombe.
  //On n'enregistre PAS d'office : le geste existe, c'est ENTER, et le mode config n'est
  //pas un endroit ou l'on ecrit dans l'EEPROM sans l'avoir demande.
  if (!seq.configMode){
    if (seq.setupNeedSaved) LcdPrintNotSaved();
    seq.setupNeedSaved = FALSE;
  }
    
}




