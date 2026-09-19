//======================================================================================
//
//   L C D . I N O                              les deux lignes de seize signes
//
//   LcdUpdate() redessine l'ecran quand needLcdUpdate a ete pose - jamais autrement,
//   un rafraichissement permanent couterait trop cher a la boucle. Le reste du fichier
//   n'est que de petites fonctions d'impression : un tempo, une gamme, une etiquette
//   prise en PROGMEM.
//
//--------------------------------------------------------------------------------------
//   A SAVOIR
//
//   Seize colonnes, pas dix-sept. Les chaines trop longues ne debordent pas : elles
//   repassent en debut de ligne et effacent le champ d'avant. Pour la version du
//   firmware, un static_assert dans define.h attrape le depassement a la compilation.
//
//======================================================================================

#include "lcdstrings.h"

/////////////////////Function//////////////////////
//Initialise IO PORT and libraries
void LcdUpdate()
{
  //LE TEMPO PAR-DESSUS, PENDANT QUE TEMPO EST TENU------------------------------------
  //Ce bloc redessinait a CHAQUE TOUR DE BOUCLE, seul du fichier a ignorer needLcdUpdate -
  //l'en-tete ci-dessus enonce pourtant la regle qu'il enfreignait.
  //Six octets vers le LCD : un positionnement de curseur et cinq caracteres. En mode 4
  //bits - c'est le cablage, lcd(18,19,20,21,22,23) - la bibliotheque envoie deux quartets
  //par octet et temporise 100 us apres chacun, soit ~200 us l'octet et ~1,3 ms la passe.
  //La boucle tombait de ~2800 a ~600 tours par seconde TANT QUE LA TOUCHE ETAIT TENUE.
  //Or EncRead() n'echantillonne la molette qu'une fois par tour, et valider un cran
  //demande au moins deux de ses quatre transitions : un saut de deux etats vaut 0 dans
  //knobDir[]. A 600 Hz on en rate trois sur quatre et le cran est perdu - pile pendant le
  //geste ou l'on tient TEMPO pour regler le tempo. Le tempo qui ne suivait pas la main,
  //c'etait l'ecran qui mangeait la boucle.
  //needLcdUpdate est pose a l'appui (Seq.ino) pour que la valeur apparaisse, et a chaque
  //cran par Enc.ino. Tenue sans rien tourner, la touche n'ecrit plus rien du tout.
  if (needLcdUpdate && tempoBtn.pressed && curSeqMode != PTRN_PLAY && !shiftBtn.pressed){
    needLcdUpdate = FALSE;
    lcd.setCursor(11,1);
    LcdPrintTempo();
  }
  //L'ecran du dessous, tempo relache----------------------------------------------------
  if(needLcdUpdate && !tempoBtn.pressed){
    needLcdUpdate = FALSE;
    if (seq.configMode)
    {
      lcd.setCursor(0,0);
      switch (seq.configPage){
      case 1:// first page
        lcd.print("syn bpm mTX mRX ");
        lcd.setCursor(cursorPos[curIndex],0);
        lcd.print(letterUpConfPage1[curIndex]);
        lcd.setCursor(0,1);
        LcdClearLine();
        lcd.setCursor(0,1);
        LcdPrintLabel(nameSync, seq.sync);
        lcd.setCursor(4,1);
        lcd.print(seq.defaultBpm);
        lcd.setCursor(9,1);
        lcd.print(seq.TXchannel);
        lcd.setCursor(13,1);
        lcd.print(seq.RXchannel);
        break;
      case 2:// second page
        lcd.print("pCh nc. nc. nc. ");
        lcd.setCursor(cursorPos[curIndex],0);
        lcd.print(letterUpConfPage2[curIndex]);
        lcd.setCursor(0,1);
        LcdClearLine();
        lcd.setCursor(0,1);
        LcdPrintLabel(namePtrnChange, seq.ptrnChangeSync);
        lcd.setCursor(4,1);
        lcd.print("xxx");
        lcd.setCursor(8,1);
        lcd.print("xxx");
        lcd.setCursor(12,1);
        lcd.print("xxx");
        break;
      }

    }
    else{
      //MUTE only overlays the step buttons, it is not a display mode of its own :
      //keep showing the screen of whatever is actually playing underneath, so the
      //pattern readout follows the chain instead of freezing.
      SeqMode dispMode = (curSeqMode == MUTE) ? prevSeqMode : curSeqMode;
      switch (dispMode){
      case MUTE:
        break;//never reached : dispMode resolves MUTE to the mode underneath
      case PTRN_PLAY:
        lcd.setCursor(0,0);
        lcd.print("  Pattern Play  ");
        lcd.setCursor(0,1);
        LcdClearLine(); 
        lcd.setCursor(2,1);
        lcd.print(char(PlayingBank()+65));
        lcd.print(curPattern % NBR_PATTERN + 1);
        lcd.setCursor(9,1);
        LcdPrintTempo();  
        break;
      case PTRN_STEP:
      case PTRN_TAP:
        //SHUFFLE TENU PASSE DEVANT TOUT, parce que la molette fait pareil : shufHeld
        //est la PREMIERE branche de EncGet(). Cette chaine-ci le mettait en troisieme,
        //donc avec l'ecran des conditions ouvert on tenait SHUFFLE, on tournait, le
        //shuffle bougeait vraiment - et l'ecran montrait toujours les conditions.
        //Regler a l'aveugle un parametre qu'on n'a pas demande a regler.
        //
        //C'EST LA DEUXIEME FOIS SUR CET ECRAN. La v2.058 avait deja du remettre
        //condMode devant l'accent global DANS LES DEUX chaines, pour la meme raison.
        //La lecon ne porte pas sur le shuffle : DEUX CHAINES DE PRIORITE QUI DECRIVENT
        //LE MEME ARBITRAGE FINISSENT PAR DIVERGER. Celle de la molette fait foi, parce
        //que c'est elle qui agit ; l'ecran ne fait que dire ce qu'elle sert.
        //Un ecran qu'on a ouvert possede la molette - sauf contre une touche qu'on
        //TIENT, qui est un geste plus fort qu'un mode qu'on a laisse ouvert.
        if (shufBtn.pressed){
          //DEUX GESTES, DEUX ECRANS. SHUFFLE seul regle la voix courante, SHIFT+SHUFFLE
          //regle les seize d'un coup - et les deux dessinaient le MEME ecran, titre
          //compris, avec un bargraphe qui lisait TOUJOURS shuffle[curInst].
          //Le geste global affichait donc la valeur de l'instrument courant, jamais la
          //sienne. On entrait sur une valeur, le premier cran en posait une autre -
          //globalShuffle partant de son propre coin - et le curseur sautait sans
          //prevenir. D'ou "il faut repasser par la valeur pour que ca bouge" : il
          //fallait surtout revenir a la main sur celle qu'on croyait editer.
          //Le titre nomme desormais ce qu'on regle, et la voix par son nom : quand seize
          //shuffles peuvent differer, savoir lequel est sous la molette n'est pas un
          //luxe.
          boolean shufGlobal = shiftBtn.pressed;
          lcd.setCursor(0,0);
          if (shufGlobal) lcd.print(" Shuffle  GLOBAL");
          else {
            lcd.print(" Shuffle        ");
            lcd.setCursor(13,0);
            LcdPrintLabel(selectInstString, curInst);//trois caracteres, colonnes 13 a 15
          }
          lcd.setCursor(0,1);
          LcdClearLine();
          lcd.setCursor(0,1);
          lcd.print("-");
          lcd.setCursor(2,1);
          LcdPrintLine(MAX_SHUF_TYPE);//columns 2 to 12
          lcd.setCursor(14,1);
          lcd.print("+");
          byte shufShown = shufGlobal ? pattern[ptrnBuffer].globalShuffle
                                      : pattern[ptrnBuffer].shuffle[curInst];
          lcd.setCursor(1 + shufShown,1);
          lcd.print((char)219);
        }
        //L'ECRAN SUIT LA MEME PRIORITE QUE LA MOLETTE. L'ecran des conditions passait
        //derriere celui de l'accent global : amener le champ "ins" sur le slot ACC
        //faisait basculer les deux d'un coup, et on se retrouvait devant un bargraphe
        //d'accent sans avoir rien demande. Cf la chaine de EncGet().
        else if (condMode){
          //La condition du pas vise, pour l'instrument courant. Le pas et l'instrument
          //sont des champs a part entiere : on arrive ici par SHIFT + pas, mais on doit
          //pouvoir se promener sans relacher la molette.
          //Deux pages, et le pas comme l'instrument restent aux memes colonnes sur les
          //deux : ce sont les reperes, ils ne doivent pas bouger sous l'oeil.
          lcd.setCursor(0,0);
          lcd.print(condPage ? "pas rtg ins     " : "pas cnd prb ins ");
          lcd.setCursor(cursorPos[curIndex],0);
          lcd.print(condPage ? letterUpCondPage2[curIndex] : letterUpCond[curIndex]);
          lcd.setCursor(0,1);
          LcdClearLine();
          lcd.setCursor(0,1);
          lcd.print(condStep + 1);
          if (condPage){
            lcd.setCursor(4,1);
            LcdPrintRetrig(pattern[ptrnBuffer].retrig[curInst][condStep] & 0x0F);
            lcd.setCursor(8,1);
            LcdPrintLabel(selectInstString, curInst);
          }
          else{
            byte cond = pattern[ptrnBuffer].probality[curInst][condStep];
            lcd.setCursor(4,1);
            LcdPrintCond(cond >> 4);
            lcd.setCursor(8,1);
            LcdPrintProb(cond & 0x0F);
            lcd.setCursor(12,1);
            LcdPrintLabel(selectInstString, curInst);
          }
        }
        else if (curInst == GLOBAL_ACC){
          LcdPrintGlobalAcc();
        }
        else if (keyboardMode){
          lcd.setCursor(0,0);
          lcd.print("idx not len oct ");
          lcd.setCursor(cursorPos[curIndex],0);
          lcd.print(letterUpExtInst[curIndex]);
          lcd.setCursor(0,1);
          LcdClearLine();
          lcd.setCursor(1,1);
          lcd.print(noteIndex + 1);
          lcd.setCursor(4,1);
          LcdPrintLabel(nameNote, pattern[ptrnBuffer].extNote[noteIndex] % 12);
          lcd.print(pattern[ptrnBuffer].extNote[noteIndex] / 12);//note octave
          lcd.setCursor(9,1);
          lcd.print(pattern[ptrnBuffer].extLength + 1);
          lcd.setCursor(13,1);
          LcdPrintLabel(nameOct, keybOct);
        }
        else{
          lcd.setCursor(0,0);
          lcd.print("ptr len scl ins ");
          lcd.setCursor(0,1);
          LcdClearLine(); 
          lcd.setCursor(0,1);
          lcd.print(char(PlayingBank()+65));
          lcd.print(curPattern % NBR_PATTERN + 1);
          lcd.setCursor(5,1);
          lcd.print(pattern[ptrnBuffer].length[curInst]+1);
          lcd.setCursor(8,1);
          LcdPrintScale();
          lcd.setCursor(12,1);
          LcdPrintLabel(selectInstString, curInst);
        }
        break;
      case TRACK_WRITE:
        lcd.setCursor(0,0);
        lcd.print("pos ptr len trk ");
        lcd.setCursor(cursorPos[curIndex],0);
        lcd.print(letterUpTrackWrite[curIndex]);
        lcd.setCursor(0,1);
        LcdClearLine();
        lcd.setCursor(0,1);
        lcd.print(trk.pos + 1);
        lcd.setCursor(4,1);
        lcd.print((char)(PlayingBank() + 65));
        lcd.print(curPattern % NBR_PATTERN + 1); 
        lcd.setCursor(8,1);
        lcd.print(track[trkBuffer].length);
        lcd.setCursor(13,1);
        lcd.print(trk.current + 1);
        break;
      case TRACK_PLAY:
        //LE TITRE RESTE SEUL EN HAUT. La piste y a d'abord ete collee a cote, en colonnes
        //13 a 15 : "Track PlayT1" est laid et l'information etait mal placee. Elle
        //descend sur la seconde ligne, a gauche, exactement la ou le pattern se trouvait.
        //
        //LE PATTERN COURANT PART AVEC, et c'est raisonne : PATT PLAY n'affiche pas les
        //instruments qui jouent, cet ecran-ci n'a pas a afficher le pattern engage. En
        //mode piste, ce qu'on suit est LA PISTE ET LA POSITION DEDANS - le pattern est un
        //detail d'execution, et TRACK EDIT le montre pour qui le veut.
        //
        //D'ou l'ordre de lecture : quelle piste a gauche, ou l'on en est a droite.
        //Base 1 des deux cotes, comme partout ailleurs.
        lcd.setCursor(0,0);
        lcd.print("   Track Play   ");
        lcd.setCursor(0,1);
        LcdClearLine();
        lcd.setCursor(0,1);
        lcd.print("trk:");
        lcd.print(trk.current + 1);
        lcd.setCursor(8,1);
        lcd.print("pos:");
        //Le +1 manquait ici : la meme variable etait affichee en base 0 en lecture et en
        //base 1 en edition, donc la premiere mesure s'appelait 0 d'un cote et 1 de
        //l'autre. Base 1 partout - une piste commence a la mesure 1.
        lcd.print(trk.pos + 1);
        break;
      }

      //MUTE, en grand et par-dessus--------------------------------------------------
      //Posee APRES le switch : elle recouvre la premiere ligne quel que soit l'ecran du
      //dessous, dont la seconde ligne reste entiere et continue de dire ce qui joue.
      //Le caractere 219 est le pave plein, celui du bargraphe de shuffle - cinq de
      //chaque cote tiennent les seize colonnes et ne ressemblent a aucun autre ecran.
      //La premiere ligne portait les etiquettes de champ en mode edition ; on ne les
      //perd pas pour rien, la molette y etant desormais inerte sous MUTE : il n'y a
      //plus aucun champ a designer. Oublier qu'on est en MUTE et s'en apercevoir au
      //changement de pattern, c'est une mesure de perdue - et ca n'arrive qu'une fois
      //de trop.
      if (curSeqMode == MUTE){
        lcd.setCursor(0,0);
        for (byte a = 0; a < 5; a++) lcd.print((char)219);
        lcd.print(" MUTE ");
        for (byte a = 0; a < 5; a++) lcd.print((char)219);
      }
    }
  }
}

//print a label straight out of one of string.h's PROGMEM tables----------------
//These were copied into local char buffers first, and all five buffers were declared
//one or two bytes short of the strings they received (" BD", "MST" and "FRE" need 4
//bytes with the terminator), so strcpy_P wrote past the stack frame on every refresh.
//Printing from flash needs no buffer at all.
void LcdPrintLabel(const char * const *table, byte index)
{
  lcd.print((const __FlashStringHelper *)pgm_read_word(&(table[index])));
}

//clear 16 character line-------------------------------------
void LcdClearLine()
{
  lcd.print("                ");//16 empty space to clear a line
}

//print special character for scale moniotring----------------
void LcdPrintScale()
{
  switch (pattern[ptrnBuffer].scale[curInst]){
  case SCALE_32:
    lcd.write(byte(0));//1/
    lcd.write(byte(4));//32
    break;
  case  SCALE_16t:
    lcd.write(byte(0));//1/
    lcd.write(byte(1));//16
    lcd.write(byte(2));//t
    break;
  case  SCALE_16:
    lcd.write(byte(0));//1/
    lcd.write(byte(1));//16
    break;
  case  SCALE_8t:
    lcd.write(byte(0));//1/
    lcd.write(byte(3));//8
    lcd.write(byte(2));//t
    break;
  }
}

//La condition cyclique, en trois signes---------------------------------------------
//"---" pour toujours, sinon "X:Y" - jouer a la Xieme mesure de chaque groupe de Y.
//Les index hors table s'affichent comme "toujours", ce qu'ils font reellement.
void LcdPrintCond(byte c)
{
  if (!c || c >= COND_COUNT){
    lcd.print("---");
    return;
  }
  lcd.print((int)pgm_read_byte(&condXY[c][0]));
  lcd.print(":");
  lcd.print((int)pgm_read_byte(&condXY[c][1]));
}

//La probabilite, en pour cent, cadree a droite sur trois colonnes-------------------
//Zero veut dire toujours, donc cent. Les quinze autres valeurs sont des seiziemes :
//1 donne 6 %, 8 en donne 50, 15 en donne 93. L'echelle se lit dans le sens de la
//densite - plus le chiffre est gros, plus la note tombe souvent.
//Le taux de repetition : "x1" a "x8", et x1 veut dire un seul coup - le comportement
//d'avant. On ecrit le nombre de coups plutot que la fraction du pas : "1/3" et "x3"
//designent la meme chose, mais c'est le nombre de coups qu'on entend.
void LcdPrintRetrig(byte r)
{
  if (r >= RETRIG_COUNT) r = 0;
  lcd.print("x");
  lcd.print(pgm_read_byte(&retrigDiv[r]));
  lcd.print(" ");
}

void LcdPrintProb(byte p)
{
  //Zero vaut seize seiziemes : c'est le haut de l'echelle, pas un trou au milieu.
  //La molette travaille sur la meme course de 1 a 16, cf Enc.ino.
  byte pc = p ? (byte)((p * 100U) / 16U) : 100;
  if (pc < 100) lcd.print(" ");
  if (pc < 10) lcd.print(" ");
  lcd.print((int)pc);
}

//Print tempo---------------------------------------------------------
void LcdPrintTempo()
{
  lcd.write(byte(5));
  lcd.print("-");
  if (seq.sync == MASTER) lcd.print(seq.bpm);
  //En esclave c'est le tempo du MAITRE, mesure sur son horloge (cf SlaveBpmMeasure).
  //Les trois paves pleins ne restent que tant qu'il n'y a rien a mesurer : pas encore
  //quatre noires recues, ou pas d'horloge du tout. Ils veulent donc dire quelque chose
  //maintenant - avant, ils etaient l'aveu qu'on ne cherchait pas.
  else if (slaveBpm) lcd.print(slaveBpm);
  else {
    lcd.print((char)219);
    lcd.print((char)219);
    lcd.print((char)219);
  }
}

//print special line--------------------------------------------------
void LcdPrintLine (byte lineSize)
{
  for (int a = 0; a < lineSize; a++){
    lcd.print((char) 161);
  }
}

//Lcd print saved !!!!
void LcdPrintSaved()
{
  lcd.setCursor(4,0);
  lcd.print("[SAVED!]");
}

//Le contraire, pour la sortie du mode config sans ENTER. Douze caracteres, colonnes 2
//a 13 : la ligne du haut en fait seize, et le message se lit d'un coup d'oeil.
void LcdPrintNotSaved()
{
  lcd.setCursor(2,0);
  lcd.print("[NOT SAVED!]");
}

//Lcd print Global Accent
void LcdPrintGlobalAcc()
{
  lcd.setCursor(0,0);
  lcd.print("Global Acc value");
  lcd.setCursor(0,1);
  LcdClearLine();
  lcd.setCursor(0,1);
  lcd.print("-");
  lcd.setCursor(1,1);
  LcdPrintLine(MAX_GLOBAL_ACC + 1);//one cell per value, 0 included
  lcd.setCursor(15,1);
  lcd.print("+");
  lcd.setCursor(1 + pattern[ptrnBuffer].globalAcc, 1);
  lcd.print((char)219);
}

//Lcd print initialize EEprom
void  LcdPrintEEpromInit()
{
  lcd.setCursor(0,0);
  lcd.print(" EEprom is being");//seize signes pile : le dix-septieme partait hors ecran
  lcd.setCursor(0,1);
  LcdClearLine();
  lcd.setCursor(0,1);
  lcd.print("  initializing  ");

}

//Lcd print TM2 adjustement
void  LcdPrintTM2Adjust()
{
  lcd.setCursor(0,0);
  lcd.print("Adjust TM2 until");
  lcd.setCursor(0,1);
  LcdClearLine();
  lcd.setCursor(0,1);
  lcd.print("TP1 is +13V...");
}










