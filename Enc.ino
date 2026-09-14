//======================================================================================
//
//   E N C . I N O                              une molette, dix significations
//
//   Tourner la molette ne veut rien dire en soi : ce fichier decide ce que ca veut dire
//   MAINTENANT, selon ce qui est maintenu et le mode en cours. EncRead() lit les deux
//   canaux et valide un cran ; EncGet() distribue ce cran a un seul destinataire.
//
//--------------------------------------------------------------------------------------
//   LA CHAINE, DANS L'ORDRE - ET L'ORDRE EST LA PRIORITE DES GESTES
//
//   SHUFFLE maintenu           -> le shuffle
//   sinon, Global Accent visee -> la valeur d'accent global, molette nue
//   sinon, TRACK WRITE         -> les champs de la piste
//   sinon, mode clavier        -> la pile de notes externes
//   sinon, mode config         -> le parametre affiche
//   sinon, ecran d'edition     -> inerte, volontairement
//   sinon                      -> le tempo
//
//--------------------------------------------------------------------------------------
//   AVANT D'Y TOUCHER
//
//   1. C'EST UNE CHAINE EXCLUSIVE. Deplacer un maillon change la priorite d'un geste du
//      panneau, donc ce que la main trouve sous ses doigts. Ca ne se fait pas en
//      passant.
//
//   2. L'INERTIE SUR LES ECRANS D'EDITION EST UN CHOIX. La molette n'y touche PAS au
//      tempo, qui n'y est meme pas affiche : on ne le deplace plus en croyant regler
//      autre chose. TEMPO maintenu reste le chemin vers le tempo, depuis n'importe ou.
//
//   3. UN CRAN = UN PAS. Le decodage accumule les transitions du cycle de Gray et ne
//      valide qu'au retour sur l'etat de repos. La table d'origine comptait les quatre
//      transitions : la molette avancait de quatre.
//
//======================================================================================

////////////////////////Function//////////////////////

void EncGet()
{
  //Sample the encoder ONCE per loop, the branches below only consume the notch
  EncRead();

  //////////////////////////////////SHUFFLE////////////////////////////////////
  //PTRN_TAP was missing from this test while LcdUpdate() offers the shuffle screen in
  //both editing modes - the two case labels are shared. So in TAP the screen showed a
  //shuffle bargraph while the knob moved something else entirely : the BPM before the
  //instrument selector existed, curInst after. A screen promising a parameter the knob
  //does not serve.
  boolean shufHeld = shufBtn.pressed && (curSeqMode == PTRN_STEP || curSeqMode == PTRN_TAP);
  if (shufHeld){
    if (shiftBtn.pressed){
      pattern[ptrnBuffer].globalShuffle = EncGet(pattern[ptrnBuffer].globalShuffle, 1);
      if(pattern[ptrnBuffer].globalShuffle <= 1) pattern[ptrnBuffer].globalShuffle = 1;
      if(pattern[ptrnBuffer].globalShuffle >= MAX_SHUF_TYPE) pattern[ptrnBuffer].globalShuffle = MAX_SHUF_TYPE;
      static byte prevShuf;
      if (pattern[ptrnBuffer].globalShuffle != prevShuf){
        prevShuf = pattern[ptrnBuffer].globalShuffle;
        needLcdUpdate = TRUE;
        patternWasEdited = TRUE;
        for (byte z = 0; z < NBR_INST; z++){
          pattern[ptrnBuffer].shuffle[z ] = pattern[ptrnBuffer].globalShuffle;
        }
      }
    }
    else{
      pattern[ptrnBuffer].shuffle[curInst] = EncGet(pattern[ptrnBuffer].shuffle[curInst], 1);
      if(pattern[ptrnBuffer].shuffle[curInst] <= 1) pattern[ptrnBuffer].shuffle[curInst] = 1;
      if(pattern[ptrnBuffer].shuffle[curInst] >= MAX_SHUF_TYPE) pattern[ptrnBuffer].shuffle[curInst] = MAX_SHUF_TYPE;
      static byte prevShufInst[NBR_INST];
      if (pattern[ptrnBuffer].shuffle[curInst] != prevShufInst[curInst]){
        prevShufInst[curInst] = pattern[ptrnBuffer].shuffle[curInst];
        needLcdUpdate = TRUE;
        patternWasEdited = TRUE;
      }
    }
  }
  //////////////////////////////////GLOBAL ACCENT////////////////////////////////////
  //A partir d'ici et jusqu'au bout, c'est UNE seule chaine else if, SHUFFLE compris.
  //SHUFFLE etait un if separe pose a cote : rien de structurel n'empechait deux branches
  //de reclamer le meme cran, la seule chose qui les separait etait EncGet() remettant le
  //cran a zero - autrement dit l'ordre des branches arbitrait en silence. Chainees,
  //exactement une branche possede la molette par passe, et c'est la condition qui le dit.
  //
  //Molette nue, comme depuis toujours. Elle est passee derriere INST maintenu en v2.013,
  //quand elle est devenue selecteur d'instrument - pour eviter qu'arriver sur ACC en
  //tournant ne la piege sur la valeur. Le selecteur est parti, le geste revient.
  else if (curInst == GLOBAL_ACC && curSeqMode == PTRN_STEP){
    pattern[ptrnBuffer].globalAcc = EncGet(pattern[ptrnBuffer].globalAcc, 1);
    //MIN_GLOBAL_ACC / MAX_GLOBAL_ACC sat in define.h without a single user, the bound
    //being written out here and a third time as the width of the bargraph. One source.
    pattern[ptrnBuffer].globalAcc = constrain(pattern[ptrnBuffer].globalAcc, MIN_GLOBAL_ACC, MAX_GLOBAL_ACC);
    static byte prevGlobalAcc;
    if (pattern[ptrnBuffer].globalAcc != prevGlobalAcc){
      prevGlobalAcc = pattern[ptrnBuffer].globalAcc;
      patternWasEdited = TRUE;
      needLcdUpdate = TRUE;
    }
  }
  ///////////////////////////////////TRACK WRITE////////////////////////////////////
  else if (curSeqMode == TRACK_WRITE && !tempoBtn.pressed && !seq.configMode){
    switch (curIndex){
      //track position
    case 0://track position
      if(instBtn) trk.pos = EncGet(trk.pos, 10);
      else trk.pos = EncGet(trk.pos, 1);
      //Meme borne que FWD dans Seq.ino, et pour la meme raison : sans elle la molette
      //creusait exactement le meme trou, ce qui aurait rendu l'autre correctif inutile.
      //track[].length est le premier emplacement libre, celui ou l'on ajoute.
      trk.pos = constrain(trk.pos, 0, (int)track[trkBuffer].length);
      trk.pos = constrain(trk.pos, 0, MAX_TRACK_POS);//garde-fou si la piste est corrompue
      static unsigned int prevTrkPos;
      if (trk.pos != prevTrkPos){
        prevTrkPos = trk.pos;
        TrackPosToPattern();
        needLcdUpdate = TRUE;
        break;
      case 1:// track pattern
        if(instBtn) nextPattern = EncGet(nextPattern, 16);
        else nextPattern = EncGet(nextPattern, 1);
        nextPattern = constrain(nextPattern, 0, MAX_PTRN - 1);
        static unsigned int prevNextPattern;
        if (nextPattern != prevNextPattern){
          prevNextPattern = nextPattern;
          if(curPattern != nextPattern) selectedPatternChanged = TRUE;
          needLcdUpdate = TRUE;
        }
        break;
      case 2://track length
        if(instBtn) track[trkBuffer].length = EncGet(track[trkBuffer].length, 10);
        else track[trkBuffer].length = EncGet(track[trkBuffer].length, 1);
        track[trkBuffer].length = constrain(track[trkBuffer].length, 0, MAX_TRACK_POS);
        static unsigned int prevTrkLength;
        if (track[trkBuffer].length != prevTrkLength){
          prevTrkLength = track[trkBuffer].length;
          trackNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
        break;
      case 3://track number
        //toDo
        break;
      }
    }
  }

  ///////////////////////////////////KEYBOARD MODE////////////////////////////////////
  else if (keyboardMode){
    switch (curIndex){
      //track position
    //Le "break" de ce premier cas etait ENFERME dans le if qui suit, et l'accolade de ce
    //if manquait : les cas 1, 2 et 3 etaient donc imbriques DEDANS. Ca compilait et ca
    //marchait, un "case" pouvant se placer n'importe ou dans le corps d'un switch et le
    //saut ignorant la condition - mais toute ligne ajoutee a la fin du premier cas serait
    //tombee dans la queue du quatrieme. Remis d'aplomb.
    case 0://l'index de la note qu'on edite
      if(instBtn) noteIndex = EncGet(noteIndex, 10);
      else noteIndex = EncGet(noteIndex, 1);
      //L'index s'arretait a 99 au lieu de s'arreter a la pile. On pouvait donc amener le
      //curseur bien au-dela de [len], editer une note qui ne serait jamais jouee, et se
      //demander pourquoi on ne l'entend pas. Meme defaut, meme forme et meme correctif
      //que la position de piste dans TRACK WRITE, et que la longueur de pile dans
      //key.ino : on ne se promene que sur ce qui existe. Les notes s'ajoutent aux touches
      //de pas, qui allongent la pile d'elles-memes.
      noteIndex = constrain(noteIndex, 0, pattern[ptrnBuffer].extLength);
      static unsigned int prevNoteIndex;
      if (noteIndex != prevNoteIndex){
        prevNoteIndex = noteIndex;
        needLcdUpdate = TRUE;
      }
      break;

    case 1://la note elle-meme
      if(instBtn) pattern[ptrnBuffer].extNote[noteIndex] = EncGet(pattern[ptrnBuffer].extNote[noteIndex], 12);
      else pattern[ptrnBuffer].extNote[noteIndex] = EncGet(pattern[ptrnBuffer].extNote[noteIndex], 1);
      //115 et non 127 : au-dela, note + 12 depasse 127 a l'envoi et reboucle dans
      //l'octave du bas. Cf MAX_EXT_NOTE dans define.h.
      pattern[ptrnBuffer].extNote[noteIndex] = constrain(pattern[ptrnBuffer].extNote[noteIndex], 0, MAX_EXT_NOTE);
      static unsigned int prevExtNote;
      if (pattern[ptrnBuffer].extNote[noteIndex] != prevExtNote){
        prevExtNote = pattern[ptrnBuffer].extNote[noteIndex];
        //LE DRAPEAU MANQUAIT, et c'est le meme oubli que pour la direction. Editer une
        //note a la molette ne posait rien : ENTER ne clignotait pas, l'ecriture n'etait
        //pas autorisee, et le travail partait en silence au chargement suivant. La meme
        //donnee posee aux touches de pas, elle, est bien marquee (key.ino).
        patternWasEdited = TRUE;
        needLcdUpdate = TRUE;
      }
      break;

    case 2://la longueur de la pile
      if(instBtn) pattern[ptrnBuffer].extLength = EncGet(pattern[ptrnBuffer].extLength, 10);
      else pattern[ptrnBuffer].extLength = EncGet(pattern[ptrnBuffer].extLength, 1);
      pattern[ptrnBuffer].extLength = constrain(pattern[ptrnBuffer].extLength, 0, MAX_EXT_INST_NOTE);
      static unsigned int prevExtLength;
      if (pattern[ptrnBuffer].extLength != prevExtLength){
        prevExtLength = pattern[ptrnBuffer].extLength;
        //patternNeedSaved etait pose en direct ici, seul site du depot a le faire : tous
        //les autres passent par patternWasEdited, que Seq.ino convertit. Le resultat
        //etait le meme, mais deux drapeaux pour une seule idee finissent toujours par
        //diverger. Et 99 en dur devient MAX_EXT_INST_NOTE, qui vaut 99 et dit pourquoi.
        patternWasEdited = TRUE;
        needLcdUpdate = TRUE;
        //Raccourcir la pile sous le curseur laisserait celui-ci dans le vide.
        if (noteIndex > pattern[ptrnBuffer].extLength) noteIndex = pattern[ptrnBuffer].extLength;
      }
      break;

    case 3://l'octave des touches de pas - PAS celle de la note affichee
      //keybOct ne transpose QUE la saisie aux seize touches (key.ino) : il decide dans
      //quelle octave elles ecrivent. Il ne touche ni a la note sous le curseur, ni a ce
      //qui sort en lecture, et il ne fait pas partie du pattern - d'ou l'absence de
      //drapeau, qui est correcte ici. C'est pourquoi le tourner ne semble rien faire
      //quand on regarde [not] : l'effet ne se voit qu'a la prochaine note jouee.
      keybOct = EncGet(keybOct, 1);
      keybOct = constrain(keybOct, 0, MAX_OCT - 1);
      static unsigned int prevKeybOct;
      if (keybOct != prevKeybOct){
        prevKeybOct = keybOct;
        needLcdUpdate = TRUE;
      }
      break;
    }
  }
  ///////////////////////////////////CONFIG MODE////////////////////////////////////
  else if (seq.configMode){
    switch (curIndex){
      //track position
    case 0:
      if(seq.configPage == 1){//sync select
        seq.sync = EncGet(seq.sync, 1);
        seq.sync = constrain(seq.sync, 0, 1);
        static boolean prevSeqSync;
        if (seq.sync != prevSeqSync){
          prevSeqSync = seq.sync;
          seq.syncChanged = TRUE;
          seq.setupNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
      }
      else if(seq.configPage == 2){//pattern change sync select
        seq.ptrnChangeSync = EncGet(seq.ptrnChangeSync, 1);
        seq.ptrnChangeSync = constrain(seq.ptrnChangeSync, 0, 1);
        static boolean prevPtrnSyncChange;
        if (seq.ptrnChangeSync != prevPtrnSyncChange){
          prevPtrnSyncChange = seq.ptrnChangeSync;
          // seq.syncChanged = TRUE;
          seq.setupNeedSaved = TRUE;
          needLcdUpdate = TRUE;
        }
      }
      break;
    case 1://external instrument note

      seq.defaultBpm = EncGet(seq.defaultBpm, 1);
      seq.defaultBpm = constrain(seq.defaultBpm, MIN_BPM, MAX_BPM);
      static unsigned int prevDefaultBpm;
      if (seq.defaultBpm != prevDefaultBpm){
        prevDefaultBpm = seq.defaultBpm;
        seq.setupNeedSaved = TRUE;
        needLcdUpdate = TRUE;
      }
      break;
    case 2://external instrument notes stack length

      seq.TXchannel = EncGet(seq.TXchannel, 1);
      seq.TXchannel = constrain(seq.TXchannel, 1, 16);
      static unsigned int prevTX;
      if (seq.TXchannel != prevTX){
        prevTX = seq.TXchannel;
        seq.setupNeedSaved = TRUE;
        needLcdUpdate = TRUE;
      }
      break;
    case 3://octave
      seq.RXchannel = EncGet(seq.RXchannel, 1);
      seq.RXchannel = constrain(seq.RXchannel, 1, 16);
      static unsigned int prevRX;
      if (seq.RXchannel != prevRX){
        prevRX = seq.RXchannel;
        MIDI.setInputChannel(seq.RXchannel);
        seq.setupNeedSaved = TRUE;
        needLcdUpdate = TRUE;
      }
      break;
    }
  }
  ////////////////////////PATT EDIT ET PATT TAP : LA MOLETTE EST INERTE/////////////
  //Elle tombait sur le BPM, qui n'est meme pas affiche sur ces ecrans : on reglait le
  //tempo en croyant regler autre chose. Elle ne fait plus rien ici.
  //Les deux modes ensemble parce que c'est le meme ecran - LcdUpdate() les sert sur des
  //etiquettes de case collees - et qu'une molette inerte d'un cote et vive de l'autre
  //serait une incoherence de plus a retenir.
  //Elle a brievement choisi l'instrument (v2.013 a v2.017). Retire : le seul moyen de
  //l'empecher de se piner sur le slot ACC etait de deplacer la valeur du Global Accent
  //derriere INST maintenu, et ce geste-la est dans les doigts depuis des annees.
  //Le cran est consomme quand meme. Sans ca il reste en attente et part sur le BPM des
  //qu'on change d'ecran : un tour de molette en edition se retrouverait applique au tempo
  //bien plus tard, sans rien pour faire le lien.
  //TEMPO maintenu reste le chemin vers le tempo, et LcdUpdate() en affiche la valeur
  //pendant que la touche est tenue.
  else if ((curSeqMode == PTRN_STEP || curSeqMode == PTRN_TAP) && !tempoBtn.pressed){
    encNotch = 0;
  }
  else{
    seq.bpm = EncGet(seq.bpm,1);
    if (seq.bpm <= MIN_BPM) seq.bpm = MIN_BPM;
    if (seq.bpm >= MAX_BPM) seq.bpm = MAX_BPM;
    static unsigned int curBpm;
    if( seq.bpm != curBpm)
    {
      curBpm = seq.bpm;
      TimerSetFrequency();
      if (curSeqMode == PTRN_PLAY || tempoBtn.pressed) needLcdUpdate = TRUE;
    }
  }
}


//Read the encoder and validate ONE notch per detent-----------------------
//A mechanical encoder runs a complete quadrature cycle (4 transitions)
//between two detents. Counting every transition made the edited value jump
//by 4 at each detent, so the transitions are accumulated instead and a notch
//is emitted only when the knob settles back on its detent.
void EncRead()
{
  byte thisState = digitalRead(ENC_PINB) | (digitalRead(ENC_PINA) << 1);

  if (thisState == oldStateEnc) return;

  encAccu += knobDir[thisState | (oldStateEnc << 2)];
  oldStateEnc = thisState;

  if (thisState == ENC_DETENT_STATE){
    //Contact bounce sums back to 0, only a real rotation reaches the threshold
    if (encAccu >= ENC_NOTCH_THRESHOLD) encNotch = 1;
    else if (encAccu <= -ENC_NOTCH_THRESHOLD) encNotch = -1;
    encAccu = 0;
  }
}

//Apply the pending notch to a value--------------------------------------
//Zeroing the notch here makes a detent single use across loop passes. It no longer
//arbitrates between parameters : the chain in EncGet() above is exclusive, so at most
//one branch is ever a candidate in a given pass.
int EncGet(int value, int dif)
{
  if (encNotch > 0) value += dif;
  else if (encNotch < 0){
    value -= dif;
    if (value < 0) value = 0;// every edited parameter is positive, no wrap around
  }
  encNotch = 0;
  return value;
}





















