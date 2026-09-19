//======================================================================================
//
//   B U T T O N . I N O                        d'un etat de bus a un geste
//
//   Les boutons arrivent en cinq octets bruts sur le SPI. Ce fichier les transforme en
//   quelque chose d'utilisable : appuye, relache, vient d'etre appuye, maintenu depuis
//   deux secondes. Il tient aussi les seize touches de pas et leur compteur de velocite
//   a trois etats - eteint, demi, plein - qui fait l'edition de pattern.
//
//======================================================================================

////////////////////////Function//////////////////////

void ButtonGet()
{
  ScanDin();
  byte firstByte = dinSr[2];
  byte secondByte = dinSr[3];
  byte thirdByte = dinSr[4];

  instBtn =  ((firstByte & BTN_INST) ? 1:0);
  ButtonGet  (&shiftBtn, firstByte & BTN_SHIFT);
  ButtonGet (&clearBtn, firstByte & BTN_CLEAR);
  ButtonGet (&trkBtn, secondByte & BTN_TRK);
  ButtonGet (&ptrnBtn, secondByte & BTN_PTRN);
  ButtonGet (&tapBtn, secondByte & BTN_TAP);
  ButtonGet (&dirBtn, secondByte & BTN_DIR);
  ButtonGet (&playBtn, firstByte & BTN_PLAY);
  ButtonGet (&stopBtn, firstByte & BTN_STOP);
  ButtonGet (&guideBtn, secondByte & BTN_GUIDE);
  ButtonGet(&scaleBtn, firstByte & BTN_SCALE);
  ButtonGet(&lastStepBtn, firstByte & BTN_LASTSTEP);
  ButtonGet(&shufBtn, firstByte & BTN_SHUF);
  ButtonGet(&backBtn, secondByte & BTN_BACK);
  ButtonGet(&fwdBtn, secondByte & BTN_FWD);
  ButtonGet(&tempoBtn, thirdByte & BTN_TEMPO);
  ButtonGet(&numBtn, secondByte & BTN_NUM);
  ButtonGet(&muteBtn, thirdByte & BTN_MUTE);
  ButtonGet(&bankBtn, thirdByte & BTN_BANK);
  ButtonGet(&encBtn, encSwState);
  ButtonGet(&stepsBtn, ((dinSr[1] <<8) | dinSr[0]));

  //Enter button-----------------------------------------------
  enterBtn.justRelease = 0;
  enterBtn.justPressed = 0;
  enterBtn.curState = ((thirdByte & BTN_ENTER) ? 1:0);
  if (enterBtn.curState != enterBtn.prevState){
    if (enterBtn.pressed == LOW && enterBtn.curState == HIGH){
      enterBtn.justPressed = HIGH;
      enterBtn.curTime = millis();
    }
    if (enterBtn.pressed == HIGH && enterBtn.curState == LOW){
      enterBtn.justRelease = HIGH;
      enterBtn.hold = LOW;
    }
    enterBtn.pressed = enterBtn.curState;
  }
  enterBtn.prevState = enterBtn.curState;

  if (enterBtn.pressed){
    if (millis() - enterBtn.curTime > HOLD_TIME) enterBtn.hold = HIGH;
  }

  //init step button------------------------------------------------
  for (byte a = 0; a < NBR_STEP_BTN; a++){//a = step button number
    stepBtn[a].justPressed = 0;
    stepBtn[a].justRelease = 0;
  }
}

//==========================================================================================
//==========================================================================================

//Init buttons state
void ButtonGet( Button *button, unsigned int pin)
{
  button->justRelease = 0;
  button->justPressed = 0;
  button->curState = ((pin) ? 1:0);
  if (button->curState != button->prevState){
    if (button->pressed == LOW && button->curState == HIGH) button->justPressed = HIGH;
    if (button->pressed == HIGH && button->curState == LOW) button->justRelease = HIGH;
    button->pressed = button->curState;
  }
  button->prevState = button->curState;
}

//Les seize touches de pas, telles que ScanDin() vient de les lire-------------------
//La fonction prenait un mode, TOGGLE ou MOMENTARY. La branche TOGGLE - trente-cinq
//lignes, un compteur par touche et un etat static que personne ne relisait - n'avait
//plus aucun appelant : les trois appels du depot demandaient tous MOMENTARY, et
//l'edition de pas passe par InstValueGet(), qui tient son propre cycle a trois etats.
//Le parametre part avec la branche, et les deux constantes qui n'existaient que pour lui.
//L'explication qu'elle portait - la touche qui devenait muette pour de bon - a demenage
//dans InstValueGet(), ou vit le code qu'elle decrit.
unsigned int StepButtonGet()
{
  return (dinSr[1] << 8) | dinSr[0];
}


unsigned int InstValueGet(unsigned int value)
{
  unsigned int reading = (dinSr[1] <<8) | dinSr[0];
  for (byte a = 0; a < NBR_STEP_BTN; a++){//a = step button number
    stepBtn[a].justPressed = 0;
    stepBtn[a].curState = bitRead(reading,a);
    if (stepBtn[a].curState != stepBtn[a].prevState){
      //Le front montant, c'est (curState != prevState) et curState == HIGH. Le
      //"pressed == LOW" qui s'y ajoutait lisait un champ que cette fonction n'ecrit
      //jamais : le seul ecrivain est le mode clavier (key.ino), qui en a besoin pour
      //apparier un note-off a son note-on. Il valait donc LOW pour toujours - sauf apres
      //avoir quitte le clavier EN TENANT une touche de pas, ce qui laissait celle-la
      //bloquee a HIGH et son front montant inatteignable pour de bon. La touche ne
      //repondait alors plus nulle part - edition de pas, TAP, selection de pattern - et
      //on n'en sortait qu'en repassant par le clavier pour l'y appuyer.
      //Ce texte vivait dans la branche TOGGLE de StepButtonGet(), retiree le 17/09/2026 :
      //la branche etait morte, l'explication ne l'est pas.
      if (stepBtn[a].curState == HIGH){
        patternWasEdited = TRUE;
        stepBtn[a].justPressed = 1;

        //two button state
        //EXT_INST etait ici : un pas y etait present ou absent, et sa velocite toujours
        //instVelHigh. Il passe au cycle a trois etats des voix, eteint > normal > accent
        //> eteint, parce que sa velocite VA QUELQUE PART - elle part dans le note-on MIDI
        //(cf Clock.ino), la ou celle de l'accent global et des deux trigs n'est lue par
        //personne. Une x0xb0x en n0nx0x2 lit l'accent dans la velocite : le cycle du
        //panneau devient le geste d'accentuation de la piste externe.
        //Le prix est celui du cycle : effacer un pas demande desormais deux pressions de
        //plus, comme sur les douze voix.
        if(curInst == GLOBAL_ACC || curInst == TR1 || curInst == TR2){
          if (bitRead(value,a)) stepBtn[a].counter = 2;
          else stepBtn[a].counter = 1;
          switch (stepBtn[a].counter){
          case 1:
            bitSet (value, a);
            pattern[ptrnBuffer].velocity[curInst][a] = instVelHigh[curInst];//HIGH_VEL;
            break;
          case 2:
            stepBtn[a].counter = 0;
            bitClear (value, a);
            //LA CONDITION PART AVEC LE TRIG. La velocite est deja remise a sa valeur
            //basse dans le cas jumeau du bloc suivant : garder la condition serait
            //incoherent avec ce que fait l'autre attribut du meme pas. Et surtout, une
            //condition qui survit a l'effacement rend le trig SUIVANT conditionne sans
            //que rien ne le dise - on reecrit un pas, il ne joue qu'une mesure sur
            //quatre, et on cherche pourquoi. L'etat invisible qui change le comportement
            //d'un geste neuf est ce qu'on ferme partout depuis la reprise.
            //CLEAR efface deja les conditions de tout le pattern, pour cette raison-la.
            pattern[ptrnBuffer].probality[curInst][a] = 0;
            break;
          }
        }
        //three button state
        else{
          if (bitRead(value,a)){
            //if vel > MID_VEL
            if (pattern[ptrnBuffer].velocity[curInst][a] > instVelLow[curInst]) stepBtn[a].counter = 3;
            else stepBtn[a].counter = 2;
          }
          else {
            stepBtn[a].counter = 1;
          }
          switch (stepBtn[a].counter){
            //half velocity
          case 1:
            bitSet (value, a);
            pattern[ptrnBuffer].velocity[curInst][a] = instVelLow[curInst];
            break;
            //full velocity
          case 2:
            pattern[ptrnBuffer].velocity[curInst][a] = instVelHigh[curInst];
            break;
            //Off
          case 3:
            stepBtn[a].counter = 0;
            bitClear (value, a);
            pattern[ptrnBuffer].velocity[curInst][a] = instVelLow[curInst];
            pattern[ptrnBuffer].probality[curInst][a] = 0;//cf le bloc du dessus
            break;
          }
        }

      }  
      stepBtn[a].prevState = stepBtn[a].curState;    
    }
  }
  return value;
}

//Mute steps buttons -----------------------------------------------------------
void MuteButtonGet()
{
  unsigned int data = (dinSr[1] <<8) | dinSr[0];
  for (byte a = 0; a < NBR_STEP_BTN; a++){
    muteStepBtn[a].justPressed = 0;
    muteStepBtn[a].curState = bitRead(data,a);
    if (muteStepBtn[a].curState != muteStepBtn[a].prevState){
      if ((muteStepBtn[a].pressed == LOW) && (muteStepBtn[a].curState == HIGH)){
        muteStepBtn[a].justPressed = 1;
        //SOLO : SHIFT + touche de pas, et c'est un interrupteur----------------------
        //Plusieurs voix peuvent etre soloees ensemble. Appuyer a nouveau sur une voix
        //soloee la retire, et retirer la derniere sort du solo et rend les mutes : il
        //n'y a donc pas de touche de sortie a apprendre.
        if (shiftBtn.pressed){
          //UN EMPLACEMENT IMMUNISE NE SE SOLO PAS. Il sonne deja sous tous les solos,
          //donc l'operation ne pourrait rien exprimer - et sur l'accent global elle
          //donnerait le silence complet : tout mute, et rien de ce qui reste ne fait
          //de son. C'est le cul-de-sac que le firmware MSS a laisse sur son CLEAR+MUTE,
          //et c'est la meme classe que le champ "ins" de l'ecran des conditions.
          if (!bitRead(SOLO_IMMUNE, a)) soloInst ^= ((unsigned int)1 << a);
        }
        //MUTE : hors solo seulement--------------------------------------------------
        //Pendant un solo, un pad nu ecrirait dans muteInst, qu'ActiveMute() recouvre :
        //ni le son ni les LED ne bougeraient, et on decouvrirait le resultat en sortant
        //du solo - une voix manquante ressemble exactement a une voix cassee. Rien ne
        //doit modifier un etat qu'on ne voit pas.
        //Le compteur n'avance donc que quand la pression veut dire quelque chose : le
        //faire avancer dans le vide le decalerait de muteInst, et la voix suivante
        //partirait sur le mauvais des deux crans.
        else if (!soloInst){
          muteStepBtn[a].counter++;
          switch (muteStepBtn[a].counter){
          case 1:
            bitSet (muteInst,a);
            break;
          case 2:
            muteStepBtn[a].counter = 0;
            bitClear (muteInst,a);
            break;
          }
        }
      }      
    }
    muteStepBtn[a].prevState = muteStepBtn[a].curState;
  }
}

//return value of first pressed step button
byte FirstBitOn()
{
  //Une locale, pas un static : elle est ecrite puis lue dans la foulee, et rien n'en
  //attend la valeur au prochain appel.
  unsigned int tempStepBtnValue = StepButtonGet();
  for (int a = 0; a < NBR_BTN_STEP; a++){
    if (bitRead (tempStepBtnValue, a)){
      return a;
    }
  }
  //No button held : the callers read the step buttons again after justPressed, and the
  //button can have been released in between. Returning nothing left the bank, the
  //pattern or the track number set from an indeterminate value.
  return 0;
}

//return value of second pressed step button
byte SecondBitOn()
{
  unsigned int tempStepBtnValue = StepButtonGet();//locale, cf FirstBitOn()
  byte counter = 0;
  byte value = 0;
  for (int a = 0; a < NBR_BTN_STEP; a++){
    if (bitRead (tempStepBtnValue, a)){
      counter++;
      if (counter == 2){
        value = a;
        break;
      }
    }
  }
  return value;
}

//return value of first just pressed step button 
//MODE => JUSTPRESSED or JUSTRELEASE

//Init buttons counter----------------------------------------------------
void InitButtonCounter()
{
  for (byte a = 0; a < NBR_STEP_BTN; a++){
    stepBtn[a].counter = 0;
  }
  stopBtn.counter = 0;
  guideBtn.counter = 0;
  scaleBtn.counter = 0;
  encBtn.counter = 0;
  muteBtn.counter = 0;
}

//Init  mute buttons counter----------------------------------------------------
void InitMuteBtnCounter()
{
  for (byte a=0; a<NBR_STEP_BTN; a++){
    muteStepBtn[a].counter = 0;
  }
}






































