//======================================================================================
//
//   C L O C K . I N O                          le coeur temporel du Yocto
//
//   Ce fichier possede le TEMPS. Timer1 bat a 96 ppqn, son interruption appelle
//   CountPPQN(), et c'est la - et nulle part ailleurs - que chaque pas de chaque
//   instrument decide s'il part. Tout le reste de la machine ne fait que preparer
//   ce qui sera joue ici.
//
//--------------------------------------------------------------------------------------
//   ENTRE PAR      ISR(TIMER1_COMPA_vect) -> CountPPQN()      96 fois par noire
//                  HandleClock() (Midi.ino) en synchro esclave, 4 appels d'affilee
//
//   SORT VERS      SetMux() puis SetDoutTrig() + StartTrigPulse()   les voix analogiques
//                  MidiSendNoteOn() / InitMidiNoteOff()             la piste EXT
//                  UDR1 en direct                                   l'horloge MIDI
//                  SwitchPatternBuffer()                            la bascule de pattern
//
//   TIENT A JOUR   ppqn  stepCount[]  curStep[]  stepChanged[]  globalStepCount
//                  endMeasure  trackPosNeedIncremante  blinkTempo  blinkFast[]
//
//--------------------------------------------------------------------------------------
//   AVANT D'Y TOUCHER, TROIS CHOSES
//
//   1. TOUT ICI TOURNE DANS UNE INTERRUPTION. Pas d'attente active, pas d'appel qui
//      bloque, et surtout pas de division : le 1284P n'a pas de diviseur materiel et
//      une division 16 bits coute 230 cycles. Le budget est un tick, soit 5208 us a
//      120 BPM mais seulement 2500 us a 250.
//
//   2. L'AVANCEMENT DU PAS ET SON DECLENCHEMENT SONT DECOUPLES. Le pas avance quand
//      mod == 0 ; il PART quand mod == d. C'est ce decalage qui rend le swing possible,
//      et c'est la meme porte par ou passerait un micro-timing par pas.
//
//   3. LE CACHE DE MODULOS, en tete de boucle, n'est pas une optimisation gratuite.
//      Il y avait 36 divisions par tick ici. Il y en a zero : 5024 cycles ramenes a 84.
//      Les identites qui le permettent sont demontrees en commentaire sur place - les
//      defaire remettrait l'interruption a genoux au-dessus de 200 BPM.
//
//======================================================================================

/////////////////////Function//////////////////////
//Un pas au hasard, sans division-------------------------------------------------------
//La direction RANDOM appelait random(0, length+1) DANS l'interruption d'horloge.
//random() de l'Arduino passe par une multiplication 32 bits puis __divmodsi4 - une
//division 32 bits logicielle - pour ramener le tirage dans l'intervalle : plusieurs
//centaines de cycles, a chaque pas, pour chaque instrument en RANDOM.
//
//Ici un xorshift 16 bits : trois decalages et trois XOR. La mise a l'echelle se fait en
//multipliant l'octet de poids faible par (maxStep + 1) et en gardant le poids fort -
//mul est une instruction du coeur AVR, deux cycles. Le biais vaut au plus 1/256 sur un
//intervalle de 16 valeurs, ce qui ne s'entend pas sur un tirage de pas.
//
//L'etat initial est fixe, comme l'etait celui de random() sans randomSeed() : la suite
//est identique d'un allumage a l'autre. C'etait deja le cas, ce n'est pas une
//regression - et pour un sequenceur ca a meme un avantage, la meme session rejoue pareil.
static unsigned int rngState = 0xACE1;

byte RandomStep(byte maxStep) {
  rngState ^= rngState << 7;
  rngState ^= rngState >> 9;
  rngState ^= rngState << 8;
  //maxStep + 1 vaut au plus NBR_STEP = 16, l'octet au plus 255 : 4080, ca tient.
  return (byte)((((unsigned int)(rngState & 0xFF)) * ((unsigned int)maxStep + 1)) >> 8);
}

//L'interruption d'horloge
ISR(TIMER1_COMPA_vect) {
  CountPPQN();
}

//Un tick, appele 96 fois par noire
void CountPPQN() {
  //ppqn est volatile, et il etait relu en SRAM a chacun de ses ~40 usages ci-dessous.
  //Une lecture ici, une ecriture en fin de fonction.
  byte p = ppqn;

  if (p % (PPQN / 2) == 0) blinkTempo = !blinkTempo;
  if (p % (pattern[ptrnBuffer].scale[curInst] / 2) == 0) blinkFast[curInst] = LOW;
  //if (p % 4 == 0) MIDI.sendRealTime(midi::Clock);//MidiSend(CLOCK_CMD);//la machine bat a 96 ppqn, l'horloge MIDI se compte en 24 : un envoi sur quatre

  if (p % 4 == 0) {
    //Ecrire UDR1 en direct plutot que de passer par la bibliotheque, c'est le correctif
    //de [zabox] contre le retard de l'horloge MIDI, et il reste. Ce qui part, c'est
    //l'attente non bornee qui le gardait.
    //
    //UDRE1 ne se leve qu'une fois que MIDI.begin() a arme l'emetteur de l'UART1, et
    //setup() demarre Timer1 dans InitSeq() environ 230 ms AVANT cet appel - LoadTrack()
    //et LoadPattern() sont entre les deux, a 100 kHz sur l'I2C. L'interruption part donc
    //une quarantaine de fois alors que le port est encore ferme. Une attente active ici
    //tourne DANS une interruption, drapeaux masques : rien ne peut la preempter, rien ne
    //peut la recuperer, la machine est simplement morte jusqu'a la coupure du courant.
    //
    //Perdre un octet d'horloge qui n'a nulle part ou aller ne coute rien - le sequenceur
    //ne tourne pas encore et personne n'ecoute. Geler coute tout.
    if (UCSR1A & (1 << UDRE1)) UDR1 = CLOCK_CMD;  //le tick
  }
  if (seq.sync == MASTER) {
    if (p % 4 == 0) {
      DIN_CLK_HIGH;
      dinClkState = HIGH;
    } else if (p % 4 == 2) {
      DIN_CLK_LOW;
      dinClkState = LOW;
    }
  }

  if (isRunning) {
    //p % scale ne depend que de p et de la scale, et il n'y a que QUATRE scales possibles.
    //C'etait recalcule pour chacun des seize instruments, deux fois chacun, plus quatre
    //fois hors de la boucle : 36 divisions par tick, dont seize sur 16 bits.
    //Calcule ici une fois par tick, avec des diviseurs constants a la compilation - le
    //compilateur les traduit en multiplication par reciproque au lieu d'appeler
    //__udivmodqi4. Indexe par SCALE_ROW(), le meme rang que la table de swing.
    //Effet de bord bienvenu : une scale corrompue a zero faisait une division par zero,
    //ce qui n'est plus possible.
    //Ecrit en une seule vraie division, et pas quatre :
    //  16 et 32 sont des puissances de deux, donc de simples masques ;
    //  12 se deduit de 24 par une comparaison, puisque p % 24 est deja inferieur a 24 ;
    //  reste 24, la seule qui divise vraiment.
    //Les operandes sont gardes en octets : ecrit tel quel, "p % 24" promeut en entier et
    //sort __divmodhi4, 230 cycles. En octets, avr-gcc emet une multiplication par
    //reciproque, une dizaine de cycles.
    byte m24 = (byte)(p % (byte)SCALE_16);
    byte modBySc[4];
    modBySc[SCALE_ROW(SCALE_16)] = m24;                                       //24
    modBySc[SCALE_ROW(SCALE_32)] = (m24 >= SCALE_32) ? m24 - SCALE_32 : m24;  //12
    modBySc[SCALE_ROW(SCALE_16t)] = p & (SCALE_16t - 1);                      //16
    modBySc[SCALE_ROW(SCALE_8t)] = p & (SCALE_8t - 1);                        //32

    byte gs = pattern[ptrnBuffer].globalScale;
    byte gsMod = modBySc[SCALE_ROW(gs)];

    if (gsMod == (gs / 2)) tapStepCount++;

    if (tapStepCount > pattern[ptrnBuffer].globalLength) tapStepCount = 0;

    //stepValue est volatile et lu par Led.ino, mais il etait aussi accumule au |= dans
    //la boucle des seize instruments - un aller-retour en SRAM par iteration.
    //Construit ici dans une locale, publie une seule fois.
    unsigned int sv = 0;

    for (byte z = 0; z < NBR_INST; z++) {
      byte sc = pattern[ptrnBuffer].scale[z];
      byte mod = modBySc[SCALE_ROW(sc)];
      if (mod == 0) stepChanged[z] = TRUE;  //le pas a change

      //-----------------------------------------------------------------------------
      //Ici se decide tout ce qui arrive a chaque pas
      //-----------------------------------------------------------------------------
      //Le retard de swing : les pas pairs restent sur la grille, les impairs sont
      //repousses de quelques ticks. La parite vient de stepCount, jamais d'un drapeau
      //a elle.
      //shufPolarity etait un booleen bascule a chaque pas joue, et il derivait : sur un
      //pattern au nombre impair de pas, il revenait inverse a chaque tour, et le swing
      //passait donc sur les pas pairs. Il n'etait remis a zero qu'au PLAY - pas au
      //changement de pattern, pas au changement de longueur, pas a une resynchro - si
      //bien que la phase du swing etait arbitraire le reste du temps. stepCount[z] porte
      //deja le numero du pas, et il est reboucle, remis a zero et resynchronise partout
      //ou il faut : lire son bit de poids faible rend la derive structurellement
      //impossible.
      //Ca remplace aussi la table shuffle[MAX_SHUF_TYPE][2], qui ne contenait rien que
      //l'index ne portait deja : la colonne 0 valait toujours 0, la colonne 1 toujours
      //l'oppose du numero de ligne.
      //Le retard vient de la table de swing plutot que de (reglage - 1) ticks, pour qu'un
      //cran veuille dire un ratio sur les quatre resolutions : cf define.h.
      //Le test etait "(p - retard) % scale == 0", une division SUR 16 BITS parce que la
      //soustraction promeut en int : 230 cycles, seize fois par tick. C'est la meme chose
      //que "p % scale == retard", qui ne coute qu'une comparaison sur le modulo deja en
      //main. L'equivalence tient parce que la table garantit retard < scale, et parce que
      //p - retard ne peut jamais tomber sur un multiple negatif de scale : p vaut au
      //moins 1 et le retard au plus 13, donc p - retard >= -12, alors que le premier
      //multiple negatif vaut -12 pour scale 12 - inatteignable, le retard y est borne a 5.
      byte d = (stepCount[z] & 1)
                 ? SWING_TICKS(sc, pattern[ptrnBuffer].shuffle[z])
                 : 0;

      if (mod == d && stepChanged[z]) {  //a chaque pas
        stepChanged[z] = FALSE;          //ce pas est declenche, ne pas y revenir
        blinkFast[z] = HIGH;             // A SORTIR DE CETTE BOUCLE

        //la direction de lecture, instrument par instrument-----------
        switch (pattern[ptrnBuffer].dir[z]) {
          case FORWARD:
            curStep[z] = stepCount[z];
            break;
          case BACKWARD:
            curStep[z] = pattern[ptrnBuffer].length[z] - stepCount[z];
            break;
          case PING_PONG:
            if (curStep[z] == pattern[ptrnBuffer].length[z] && changeDir[z] == 1) changeDir[z] = 0;
            else if (curStep[z] == 0 && changeDir[z] == 0) changeDir[z] = 1;
            if (changeDir[z]) curStep[z] = stepCount[z];
            else curStep[z] = pattern[ptrnBuffer].length[z] - stepCount[z];
            break;
          case RANDOM:
            curStep[z] = RandomStep(pattern[ptrnBuffer].length[z]);
            break;
        }
        //Poser le bit de cet instrument dans le mot de seize bits, selon son compteur
        //de pas et sa direction. L'accent global n'en est pas : il ne se declenche pas,
        //il est lu par MuxValue() au moment du balayage.
        if (z != GLOBAL_ACC) {  //&& z != TR1 && z != TR2){
          sv |= ((bitRead(pattern[ptrnBuffer].inst[z], curStep[z])) << z);
        }

        stepCount[z]++;
      }
    }

    //Avancer le compteur global au rythme de l'instrument le plus long
    if (modBySc[SCALE_ROW(pattern[ptrnBuffer].scale[longerInstLength])] == 0) {
      globalStepCount++;
    }

    //Le metronome, sans toucher a la donnee musicale-----------------------------------
    //Il sonne sur le rimshot. Sa velocite est calculee ici et rendue par MuxValue() a la
    //volee. Metronome() tamponnait 127 et 30 DANS pattern[].velocity[RM][0/4/8/12] en
    //entrant, puis HIGH_VEL en sortant - sans jamais relever ce qu'il ecrasait. Un
    //rimshot a demi sur un temps revenait donc a plein apres un aller-retour sur GUIDE,
    //en silence, patternWasEdited n'etant meme pas pose. Et ces valeurs restaient dans le
    //tableau : elles deteignaient sur les frappes RM hors metronome des que RM avait sa
    //propre longueur, curStep[RM] ne suivant plus globalStepCount.
    //Le -1 est celui d'origine, globalStepCount vient d'etre incremente. Le test de borne
    //evite un decalage negatif au tout premier tick, ou il vaut encore -1.
    metronomeVel = 0;
    if (gsMod == 0) {
      byte metroStep = globalStepCount - 1;
      if (metroStep < NBR_STEP && bitRead(metronome, metroStep)) {
        sv |= (1 << RM);
        metronomeVel = (metroStep == 0) ? MET_VEL_FIRST : MET_VEL_OTHER;
      }
    }

    //Retirer les instruments mutes, puis publier pour Led.ino
    sv &= ~muteInst;
    stepValue = sv;

    //Y a-t-il quelque chose a declencher ?
    if (sv) {
      if (bitRead(sv, TR1)) {
        TR1_LOW;  //impulsion sur la sortie de trig
        trig1State = HIGH;
      }
      if (bitRead(sv, TR2)) {
        TR2_HIGH;  //impulsion sur la sortie de trig
        trig2State = HIGH;
      }
      //Les tensions d'abord, les declenchements ensuite : chaque condensateur doit
      //porter sa velocite avant que la voix ne parte.
      SetMux();
      SetDoutTrig(sv);
      StartTrigPulse();  //Timer2 relache les sorties 2 ms plus tard

      //L'instrument externe------------------------------------------
      if (bitRead(sv, EXT_INST))  //pattern[ptrnBuffer].inst[EXT_INST], curStep[EXT_INST]))
      {
        InitMidiNoteOff();
        MidiSendNoteOn(seq.TXchannel, pattern[ptrnBuffer].extNote[noteIndexCpt], HIGH_VEL);
        midiNoteOnActive = TRUE;
        noteIndexCpt++;  //avancer d'une note dans la pile
      }
      if (noteIndexCpt > pattern[ptrnBuffer].extLength) {
        noteIndexCpt = 0;
      }
    }  //FIN DU BLOC DE DECLENCHEMENT

    //Rebouclage des compteurs de pas. length[] est le numero du DERNIER pas, base zero :
    //une longueur de 15 veut donc dire seize pas, de 0 a 15.
    for (byte z = 0; z < NBR_INST; z++) {
      if (stepCount[z] > pattern[ptrnBuffer].length[z]) {
        stepCount[z] = 0;
      }
    }

    //Fin de mesure : la piste avance d'un cran et le pattern suivant peut prendre la main
    if (globalStepCount > pattern[ptrnBuffer].globalLength) {
      endMeasure = TRUE;
      trackPosNeedIncremante = TRUE;
      globalStepCount = 0;
      //La bascule de buffer etait recopiee ici ; elle ne tournait jamais, la boucle
      //principale ayant deja efface nextPatternReady. Une seule copie desormais, dans
      //SwitchPatternBuffer() - ce qui evite en prime que l'interruption ecrive ptrnBuffer
      //dans le dos de la boucle principale.
    }
    //Le drapeau de fin de mesure se rabaisse quatre ticks plus tard, une fois que la
    //boucle principale a eu le temps de le voir. Valeur heritee, jamais expliquee.
    if (gsMod == 4 && globalStepCount == 0) {
      endMeasure = FALSE;
    }
  }

  if (++p > PPQN) p = 1;
  ppqn = p;
}  //FIN DE CountPPQN

//Le metronome : un masque de pas, rien de plus-----------------------------------------
//Il n'ecrit plus rien dans le pattern. Les quatre temps sont le masque 0x1111, et la
//velocite est posee par CountPPQN() dans metronomeVel puis rendue par MuxValue().
void Metronome(boolean state) {
  metronome = state ? 0x1111 : 0;  //RM sur les quatre temps
}
