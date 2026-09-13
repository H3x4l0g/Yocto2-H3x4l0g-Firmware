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
  if (!seq.configMode) seq.setupNeedSaved = FALSE;
    
}




