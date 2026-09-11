//-------------------------------------------------
//                  Yocto v2.x
//                  LCD update
//-------------------------------------------------

#include "string.h"

/////////////////////Function//////////////////////
//Initialise IO PORT and libraries
void LcdUpdate()
{
  //display tempo
  if (tempoBtn.pressed && curSeqMode != PTRN_PLAY && !shiftBtn.pressed ){
    lcd.setCursor(11,1);
    LcdPrintTempo();
  }
  //display total  accent
  /*if (enterBtn.pressed && curSeqMode == PTRN_PLAY){
   LcdPrintTotalAcc();
   }*/
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
      switch (curSeqMode){
      case PTRN_PLAY:
        lcd.setCursor(0,0);
        lcd.print("  Pattern Play  ");
        lcd.setCursor(0,1);
        LcdClearLine(); 
        lcd.setCursor(2,1);
        lcd.print(char(curBank+65));
        lcd.print(curPattern - (curBank*NBR_PATTERN) + 1);
        lcd.setCursor(9,1);
        LcdPrintTempo();  
        break;
      case PTRN_STEP:
      case PTRN_TAP:
        if (curInst == TOTAL_ACC){
          LcdPrintTotalAcc();
        }
        else if (shufBtn.pressed){
          lcd.setCursor(0,0);
          lcd.print("  Shuffle value ");
          lcd.setCursor(0,1);
          LcdClearLine();
          lcd.setCursor(2,1);
          lcd.print("-");
          lcd.setCursor(5,1);
          LcdPrintLine(7);
          lcd.setCursor(14,1);
          lcd.print("+");
          lcd.setCursor(4 + pattern[ptrnBuffer].shuffle[curInst],1);
          lcd.print((char)219);
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
          lcd.print(char(curBank+65));
          lcd.print(curPattern - (curBank*NBR_PATTERN) + 1);
          lcd.setCursor(5,1);
          lcd.print(pattern[ptrnBuffer].length[curInst]+1);
          lcd.setCursor(8,1);
          LcdPrintScale();
          lcd.setCursor(12,1);
          LcdPrintLabel(selectInstString, curInst);
        }
        break;
        /* case PTRN_TAP:
         lcd.setCursor(0,0);
         lcd.print(" Ptrn Tap edit  ");
         lcd.setCursor(0,1);
         LcdClearLine(); 
         lcd.setCursor(2,1);
         lcd.print(char(curBank+65));
         lcd.print(curPattern - (curBank*NBR_PATTERN));
         lcd.setCursor(9,1);
         LcdPrintTempo();  
         break;*/
      case TRACK_WRITE:
        lcd.setCursor(0,0);
        lcd.print("pos ptr len num ");
        lcd.setCursor(cursorPos[curIndex],0);
        lcd.print(letterUpTrackWrite[curIndex]);
        lcd.setCursor(0,1);
        LcdClearLine();
        lcd.setCursor(0,1);
        lcd.print(trk.pos + 1);
        lcd.setCursor(4,1);
        lcd.print((char)((curPattern / 16) + 65));
        lcd.print(curPattern - ((curPattern / 16)*NBR_PATTERN) + 1); 
        lcd.setCursor(8,1);
        lcd.print(track[trkBuffer].length);
        lcd.setCursor(13,1);
        lcd.print(trk.current + 1);
        break;
      case TRACK_PLAY:
        lcd.setCursor(0,0);
        lcd.print("   Track Play   ");
        lcd.setCursor(0,1);
        LcdClearLine(); 
        lcd.setCursor(0,1);
        lcd.print("pos:");
        lcd.print(trk.pos);
        lcd.setCursor(8,1);
        lcd.print("ptrn:");
        lcd.print((char)((curPattern / 16) + 65));
        lcd.print(curPattern - ((curPattern / 16)*NBR_PATTERN) + 1); 
        break;
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

//Print tempo---------------------------------------------------------
void LcdPrintTempo()
{
  lcd.write(byte(5));
  lcd.print("-");
  if (seq.sync == MASTER)lcd.print(seq.bpm);
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

//Lcd print Total Accent
void LcdPrintTotalAcc()
{
  lcd.setCursor(0,0);
  lcd.print("Total Acc value ");
  lcd.setCursor(0,1);
  LcdClearLine();
  lcd.setCursor(0,1);
  lcd.print("-");
  lcd.setCursor(1,1);
  LcdPrintLine(14);
  lcd.setCursor(15,1);
  lcd.print("+");
  lcd.setCursor(1 + pattern[ptrnBuffer].totalAcc, 1);
  lcd.print((char)219);
}

//Lcd print initialize EEprom
void  LcdPrintEEpromInit()
{
  lcd.setCursor(0,0);
  lcd.print(" EEprom is being ");
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










