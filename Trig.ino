//======================================================================================
//
//   T R I G . I N O                            la sortie de declenchement TRIG-1
//
//   Trente lignes, un seul role : rendre les sorties de trig au repos quand leur duree
//   est ecoulee. Appelee a chaque tour de boucle principale.
//
//======================================================================================

////////////////////////Function//////////////////////

void SetTrigTime()
{
   if (trig1State && !trigTimeOk){
    trigTime = millis();
    trigTimeOk = HIGH;
  }
  //This test used to run unguarded, and that was two bugs in one line.
  //
  //One : trig1State is raised by the sequencer ISR (Clock.ino). When the ISR fired
  //between the two ifs, trigTime still held the previous pulse's timestamp, so
  //millis() - trigTime was already past TRIG_TIME_MS and the brand new pulse was ended
  //on the spot - the occasional runt pulse heard on TRIG-1. Diagnosed independently by
  //zabox on the NAVA firmware, "[1.027] solves short trig 1 pulse".
  //
  //Two : MIDI note 60 drives TR1 as a gate, TR1_LOW on note on and TR1_HIGH on note off
  //(Midi.ino). Unguarded, this ran on every pass of loop() and pulled TR1 back high at
  //once, so that gate never outlived a single iteration.
  //
  //Requiring trigTimeOk means only a pulse this function actually started can be ended
  //by it, and only once its timestamp has been taken.
  if (trigTimeOk && (millis() - trigTime >= TRIG_TIME_MS)){
    TR1_HIGH;//Reset trig1 out
    trig1State = trigTimeOk = LOW;
  }
}
