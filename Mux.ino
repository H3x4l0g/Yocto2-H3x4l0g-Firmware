//-------------------------------------------------
//                  Yocto v2.x
//                  Dio utility
//-------------------------------------------------

/////////////////////Function//////////////////////

//Poser l'adresse du multiplexeur sans ecraser une impulsion---------------------------
//PORTA porte deux choses qui n'ont rien a voir : les trois bits d'adresse du
//multiplexeur (bits 5 a 7), et quatre sorties temps reel ecrites par l'interruption
//Timer1 - DIN_START (PA0), DIN_CLK (PA1), TR1 (PA2), TR2 (PA3).
//
//Le code d'origine posait l'adresse en deux lectures-modifications-ecritures :
//    PORTA &= 0b00011111;   ->  in r24,0x02 / andi r24,0x1F / out 0x02,r24
//    PORTA |= addr << 5;    ->  in r24,0x02 / ori  r24,...  / out 0x02,r24
//Verifie au desassemblage : une interruption tombant entre le "in" et le "out" voit son
//sbi/cbi ECRASE par la valeur lue avant elle. Depuis SetMux() ca ne peut pas arriver,
//il tourne deja dans l'interruption - mais le chemin MIDI passe par la boucle
//principale, et rouvrait la fenetre douze fois par note recue (une seule fois
//aujourd'hui, depuis SetMuxTrigOne). Fenetre ~190 ns : une impulsion DIN-sync ou
//trigger perdue de temps en temps en jeu MIDI dense.
//
//Ici une seule ecriture, interruptions fermees autour du in/out. SREG est sauve et
//restaure plutot que de finir par sei() : SetMux() tourne DANS l'interruption, ou un
//sei() aveugle la rouvrirait au milieu.
static inline void MuxAddr(byte addr)
{
  byte sreg = SREG;
  cli();
  PORTA = (PORTA & 0b00011111) | (addr << 5);
  SREG = sreg;
}

//La valeur envoyee au DAC, selon la source--------------------------------------------
//C'est la SEULE chose qui distinguait SetMux(), SetMuxTest() et SetMuxTrigMidi() - cette
//derniere est partie depuis, cf SetMuxTrigOne(). Les trois parcouraient les memes douze
//adresses, dans le meme ordre, avec les memes
//temporisations de charge des condensateurs. Vingt lignes recopiees trois fois - et le
//correctif de la course ci-dessus aurait du etre applique aux trois, ce qui est
//exactement le schema qui avait produit les bugs de chainage.
static byte MuxValue(byte src, byte inst)
{
  if (src == MUX_TEST) return 127;
  //Accent global. Le test d'origine s'ecrivait "bitRead(...) & ~(bitRead(...))" et ne
  //marchait que parce que bitRead rend exactement 0 ou 1 : ~0 vaut 0xFFFF et ~1 vaut
  //0xFFFE, donc seul le bit 0 decidait. Juste par accident ; ecrit ici comme il se lit.
  boolean acc = bitRead(pattern[ptrnBuffer].inst[GLOBAL_ACC], curStep[GLOBAL_ACC])
             && !bitRead(muteInst, GLOBAL_ACC);
  //127 au plus pour la velocite, 13 * 4 = 52 au plus pour l'accent : 179, pas de
  //debordement d'octet. SetDacA ecrete de toute facon a 127.
  return pattern[ptrnBuffer].velocity[inst][curStep[inst]]
       + (acc ? (pattern[ptrnBuffer].globalAcc * 4) : 0);
}

//Le balayage commun aux trois---------------------------------------------------------
static void MuxSweep(byte src)
{
  //Second multiplexeur IC111 : CY, OH, CH et l'accent PC2
  SelectSecondMux();
  for (byte a = 0; a < 4; a++){
    MuxAddr(muxAddr[a]);
    SetDacA(MuxValue(src, 8 + a));
    delayMicroseconds(5);
  }
  //Premier multiplexeur IC114 : BD, SD, LT, MT, HT, RM, HC et l'accent CB
  SelectFirstMux();
  for (byte a = 0; a < 8; a++){
    MuxAddr(muxAddr[a]);
    SetDacA(MuxValue(src, a));
    delayMicroseconds(5);
  }
}

//Update Velocity of each instruments
void SetMux()
{
  MuxSweep(MUX_PATTERN);
}

//Set ACC to max in Voice test mode
void SetMuxTest()
{
  MuxSweep(MUX_TEST);
}

//Poser la velocite d'UN instrument----------------------------------------------------
//MidiTrigOn() appelait SetMuxTrigMidi(), qui balayait les DOUZE adresses pour en changer
//une seule : douze transactions SPI et douze temporisations de charge, par note recue.
//Les onze autres condensateurs gardent leur charge, et surtout chaque note rafraichit
//la sienne juste avant de declencher - il n'y a donc rien a rafraichir chez les voisins,
//qui de toute facon ne sonnent pas a cet instant.
//SetMuxTrigMidi() et le tableau midiVelocity[] qui n'existait que pour lui sont partis
//avec : le tableau servait a REPOSER les douze velocites MIDI a chaque note, ce qui
//ecrasait au passage les velocites du pattern sur les onze autres voix.
//Instruments 0 a 7 sur le premier multiplexeur, 8 a 11 sur le second, adresses
//muxAddr[0..7] et muxAddr[0..3] respectivement - cf MuxSweep().
void SetMuxTrigOne(byte inst, byte velocity)
{
  if (inst >= 8){
    SelectSecondMux();
    MuxAddr(muxAddr[inst - 8]);
  }
  else{
    SelectFirstMux();
    MuxAddr(muxAddr[inst]);
  }
  SetDacA(velocity);
  delayMicroseconds(5);//charge du condensateur d'echantillonnage
}

void SelectFirstMux(){
  delayMicroseconds(15);                                          //Wait stabilisation  [zabox] moved for better visibility, 15us are enough
  bitWrite(MUX_INH_PORT, MUX_INH2_BIT, 0);
  bitWrite(MUX_INH_PORT, MUX_INH1_BIT, 1);
  delayMicroseconds(5);                                           // [zabox] charge s/h caps
}

void SelectSecondMux(){
  delayMicroseconds(15);                                          //Wait stabilisation  [zabox] moved for better visibility, 15us are enough
  bitWrite(MUX_INH_PORT, MUX_INH1_BIT, 0);
  bitWrite(MUX_INH_PORT, MUX_INH2_BIT, 1);
  delayMicroseconds(5);                                           // [zabox] charge s/h caps
}
