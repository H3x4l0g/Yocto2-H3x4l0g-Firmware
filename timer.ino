//-------------------------------------------------
//                  Yocto v2.x
//             Timer Configuration
//-------------------------------------------------

void TimerStart()
{
  TCCR1A = TCCR1B = 0;
  //prescale 8 => 16000000/8 = 2000000 Hz by tick
  TCCR1B |= _BV (CS11) | _BV (WGM12);
  TimerSetFrequency();
  TIMSK1 |= _BV(OCIE1A);

}

void TimerStop(void)
{
  bitWrite(TIMSK1, OCIE1A, 0);
  TCCR1A = TCCR1B = OCR1A = 0;
}

//Tick rate for Timer1 = (BPM * PPQN) / 60 Hz
//Kept in 32 bits and divided once. The previous two step integer division,
//(bpm*PPQN)/60 then (F_CPU/8)/that, truncated twice and hit the exact tempo for only
//7 of the 221 settings - up to 0.54 BPM off, ie 0.75 s adrift over four minutes at
//121 BPM. CTC counts OCR1A + 1 ticks per period, hence the -1.
void TimerSetFrequency()
{
  unsigned int bpm = constrain(seq.bpm, MIN_BPM, MAX_BPM);
  unsigned long ticks = ((unsigned long)(F_CPU / 8) * 60UL) / ((unsigned long)bpm * PPQN);

  uint8_t sreg = SREG;
  cli();
  OCR1A = (unsigned int)(ticks - 1);
  //A new OCR1A below the count already reached would simply be missed : the counter
  //would climb to 0xFFFF before wrapping, a 32 ms hiccup. Restart the period instead.
  if (TCNT1 > OCR1A) TCNT1 = 0;
  SREG = sreg;
}

