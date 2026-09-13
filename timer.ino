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

//Trigger pulse one shot------------------------------------------------------------
//The sequencer interrupt used to hold the shift register outputs high with a
//delayMicroseconds(2000) - 2 ms of busy wait inside the ISR, more than the whole tick
//period at 250 BPM. Timer2 carries the pulse instead and fires a tiny interrupt that
//just releases the outputs.
//Timer2 rather than an OCR1B compare on Timer1 : in SLAVE sync Timer1 is stopped and
//CountPPQN is driven by the incoming MIDI clock, so a Timer1 compare would never fire
//and the trigger would stay latched.
//Prescaler 128 -> 125 kHz, one tick = 8 us, so 2 ms = 250 ticks.
#define TRIG_PULSE_TICKS 250

void StartTrigPulse()
{
  TCCR2A = _BV(WGM21);              //CTC, OCR2A is TOP
  TCNT2  = 0;
  OCR2A  = TRIG_PULSE_TICKS - 1;
  TIFR2  = _BV(OCF2A);              //drop a compare match left pending
  TIMSK2 = _BV(OCIE2A);
  TCCR2B = _BV(CS22) | _BV(CS20);   //start, prescaler 128
}

//Armer l'impulsion sans relancer celle qui court deja---------------------------------
//Le declenchement MIDI s'AJOUTE a ce qui est arme (AddDoutTrig). S'il relancait le
//compte a chaque note, un flux plus serre que 2 ms tiendrait le registre haut
//indefiniment - et comme les voix se declenchent sur le FRONT montant, un bit qui ne
//redescend jamais fait taire les notes SUIVANTES du meme instrument. Le note-off remet
//instWasMidiTrigged a FALSE, mais le bit serait deja a 1 : pas de nouveau front, pas de
//son.
//On laisse donc courir l'echeance en cours. Une note arrivant a 1,9 ms n'a que 0,1 ms
//d'etat haut, ce qui suffit : elle a eu son front, et c'est tout ce que la voix regarde.
//Le sequenceur, lui, garde un StartTrigPulse() inconditionnel - il REMPLACE le masque,
//donc ses voix ont besoin d'un front neuf et d'une impulsion pleine.
//TCCR2B non nul = Timer2 en marche ; l'interruption de fin le remet a zero.
void StartTrigPulseIfIdle()
{
  if (TCCR2B) return;
  StartTrigPulse();
}

ISR(TIMER2_COMPA_vect)
{
  TCCR2B = 0;                       //one shot : stop the timer
  TIMSK2 = 0;
  ResetDoutTrig();
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

