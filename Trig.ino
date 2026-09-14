//======================================================================================
//
//   T R I G . I N O                            la sortie de declenchement TRIG-1
//
//   Trente lignes, un seul role : rendre TRIG-1 au repos quand sa duree est ecoulee.
//   Appelee a chaque tour de boucle principale.
//
//--------------------------------------------------------------------------------------
//   POURQUOI CE N'EST PAS DANS timer.ino
//
//   Parce que la machine relache ses declenchements de DEUX facons, et que ce sont deux
//   mecanismes differents pour deux destinations differentes :
//
//     les seize bits des registres a decalage   Timer2, en interruption,  2 ms
//     TRIG-1, la prise du panneau               millis(), a la boucle,   10 ms
//
//   Les deux durees sont heritees et n'ont jamais ete justifiees nulle part.
//
//--------------------------------------------------------------------------------------
//   CE QUE LE SCHEMA DIT DE TRIG-2 ET TRIG-3   (releve le 14/09/2026, docs/eagle/)
//
//     PA2   TRIG_1   -> J2, un connecteur.   La seule qui sorte vraiment.
//     PA3   TRIG_2   -> TEST_MODE.1          une broche du cavalier de test, rien d'autre
//     PA4   TRIG_3   -> TEST_MODE.3          idem
//
//   Autrement dit, sur cette carte, TRIG-2 ne va nulle part. Le pattern porte pourtant
//   une piste TR2 complete - seize pas, enregistree en EEPROM - et Clock.ino fait bien
//   TR2_HIGH quand elle se declenche. Le signal s'arrete sur un cavalier.
//
//   Et il n'en revient jamais : TR2_LOW n'est ecrit NULLE PART dans le depot, TR2 n'est
//   pas remis a son repos par InitIO() (qui ne fait que TR1_HIGH), et trig2State est pose
//   a HIGH sans que personne ne le relise. Le premier pas TR2 leve donc la broche pour de
//   bon. Sans consequence tant qu'elle ne sort pas - mais c'est a savoir avant d'y brancher
//   quoi que ce soit.
//
//======================================================================================

////////////////////////Function//////////////////////

void SetTrigTime()
{
   if (trig1State && !trigTimeOk){
    trigTime = millis();
    trigTimeOk = HIGH;
  }
  //Ce test tournait sans garde, et c'etait deux bugs sur une ligne.
  //
  //UN : trig1State est leve par l'interruption du sequenceur (Clock.ino). Quand elle
  //tombait entre les deux if, trigTime portait encore l'horodatage de l'impulsion
  //precedente, donc millis() - trigTime depassait deja TRIG_TIME_MS et l'impulsion toute
  //neuve etait coupee sur place - le coup de trig ecourte qu'on entendait de temps en
  //temps sur TRIG-1. Diagnostique independamment par zabox sur la NAVA, "[1.027] solves
  //short trig 1 pulse".
  //
  //DEUX : la note MIDI 60 mene TRIG-1 comme une porte, TR1_LOW a l'appui et TR1_HIGH au
  //relacher (Midi.ino). Sans garde, ce test tournait a chaque tour de boucle et ramenait
  //TR1 en l'air aussitot : cette porte ne survivait pas a une iteration.
  //
  //Exiger trigTimeOk fait que seule une impulsion demarree PAR cette fonction peut etre
  //arretee par elle, et seulement une fois son horodatage pris.
  if (trigTimeOk && (millis() - trigTime >= TRIG_TIME_MS)){
    TR1_HIGH;//TRIG-1 revient au repos - cette sortie est active a l'etat bas
    trig1State = trigTimeOk = LOW;
  }
}
