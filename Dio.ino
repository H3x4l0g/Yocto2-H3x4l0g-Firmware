//======================================================================================
//
//   D I O . I N O                              les entrees-sorties, au ras du silicium
//
//   Rien ici ne sait ce qu'est un pattern. Ce fichier ne connait que des bus : le SPI
//   qui porte les boutons a l'entree et les LED, les trigs et le DAC a la sortie, et le
//   I2C des deux EEPROM. Tout le reste de la machine passe par ces quelques fonctions.
//
//--------------------------------------------------------------------------------------
//   SORT VERS      treize registres a decalage sur le meme bus SPI - c'est pourquoi le
//                  panneau s'illumine quand on programme la puce par l'ICSP
//
//   POSSEDE        dinSr[]  doutTrigState  le miroir du registre de trigs
//
//--------------------------------------------------------------------------------------
//   AVANT D'Y TOUCHER
//
//   1. SPI.usingInterrupt(255) COUPE TOUTES LES INTERRUPTIONS a chaque transaction.
//      255 ne correspond a aucun vecteur, la bibliotheque tombe donc dans son cas par
//      defaut et fait un noInterrupts() global. C'est VOULU : SetDoutTrig() et SetMux()
//      sont appeles depuis l'interruption Timer1.
//
//   2. LE REGISTRE DE TRIGS NE SE LIT PAS. On ne peut qu'ecrire les seize bits d'un
//      coup, d'ou le miroir logiciel. SetDoutTrig() remplace, AddDoutTrig() ajoute -
//      confondre les deux, c'est tronquer le declenchement d'un pas.
//
//   3. LE "DEBOUNCE" DE ScanDin() N'EN EST PAS UN. DEBOUNCE_TIME ne limite que la
//      frequence de rafraichissement ; les deux echantillons compares sont separes
//      d'une iteration de boucle, pas de 5 ms. Ca marche, mais ne compte pas dessus.
//
//======================================================================================

/////////////////////Function//////////////////////
//Initialise IO PORT and libraries
void InitIO()
{
  DDRB = DDRB | B11000;
  DDRD = DDRD | B11110000;
  DDRA = DDRA | B11101100;
  TR1_HIGH;
  pinMode(DIN_START_PIN, OUTPUT);
  pinMode(DIN_CLK_PIN, OUTPUT);
  pinMode (ENC_PINA, INPUT);
  pinMode (ENC_PINB, INPUT);
  SPI.begin();
  //255 ne correspond a aucun vecteur d'interruption : la bibliotheque tombe dans son
  //cas par defaut et pose interruptMode = 2, ou beginTransaction() fait un
  //noInterrupts() GLOBAL rendu par endTransaction(). C'est volontaire et c'est le bon
  //choix ici - SetDoutTrig() et SetMux() sont appeles depuis l'interruption Timer1,
  //donc une transaction SPI de la boucle principale doit etre protegee d'elles.
  //Mais ca ne se lit nulle part dans l'appel : d'ou ce commentaire.
  SPI.usingInterrupt(255);
  I2cBusRecover();//must run before Wire.begin() takes the pins over
  Wire.begin();
  //Le bus tournait a 100 kHz, la valeur par defaut de la bibliotheque. Un pattern fait
  //1280 octets, chacun coutant 9 bits sur le bus avec son acquittement : 115 ms de boucle
  //principale bloquee a chaque chargement, plus l'adressage des pages.
  //Ce n'est pas qu'un confort d'affichage. Le changement de pattern d'une chaine est
  //declenche A la barre de mesure, et SwitchPatternBuffer() ne remet les compteurs de pas
  //a zero qu'APRES le chargement : les premiers pas de la mesure sont donc joues par
  //l'ancien pattern avant que le nouveau ne prenne la main. 118 ms font 1,8 pas a 230 BPM,
  //et ca s'entend comme une marche - les deux premiers pas joues deux fois.
  //A 400 kHz : ~30 ms, soit 0,46 pas, sous le seuil.
  //Les deux M24M02 sont specifiees a 400 kHz sur toute leur plage d'alimentation, et les
  //tirages du bus (R78, R79) sont des 4,7 kohms. La limite pour tenir les 300 ns de front
  //montant est Rp = 300ns / (0,85 x Cbus), soit environ 7 kohms a 50 pF : il y a de la
  //marge. Se verifie au scope sur SCL.
  //Le vrai correctif reste le prechargement avant la barre : celui-ci ne
  //fait que raccourcir la fenetre.
  Wire.setClock(400000);
  //Ne jamais attendre indefiniment sur le bus non plus. La plus grosse transaction fait
  //130 octets, soit environ 3 ms a 400 kHz : 50 ms reste plus de quinze fois n'importe
  //quel transfert legitime, et l'infini que c'etait avant n'a pas sa place ici. Au dela
  //du delai, le peripherique est reinitialise, ce qui relache les lignes.
  Wire.setWireTimeout(50000, true);
}

//I2C bus recovery, run before Wire.begin().
//A slave interrupted in the middle of a byte - which is literally what cutting the
//power during a page read does - keeps driving SDA low and holds the whole bus. Nothing
//in twi_init() can undo that : the master never gets its start condition through, and
//the first transaction of setup() waits on a state change that will never come. Inside
//an interrupt driven state machine with no timeout, that is a dead machine. Two power
//cycles used to clear it because the EEPROM eventually gave up.
//The cure is the textbook one : clock SCL by hand until the slave lets go of SDA, then
//place a stop condition so everyone agrees the bus is idle.
void I2cBusRecover()
{
  pinMode(SDA, INPUT_PULLUP);
  pinMode(SCL, INPUT_PULLUP);
  delayMicroseconds(10);
  if (digitalRead(SDA) == HIGH) return;//bus already free, nothing to do

  pinMode(SCL, OUTPUT);
  for (byte i = 0; i < 9 && digitalRead(SDA) == LOW; i++){
    digitalWrite(SCL, LOW);
    delayMicroseconds(5);
    digitalWrite(SCL, HIGH);
    delayMicroseconds(5);
  }
  //Stop condition by hand : SDA rises while SCL is high.
  pinMode(SDA, OUTPUT);
  digitalWrite(SDA, LOW);
  delayMicroseconds(5);
  pinMode(SCL, INPUT_PULLUP);
  delayMicroseconds(5);
  pinMode(SDA, INPUT_PULLUP);
  delayMicroseconds(5);
}

//Pre init IO for Test MODE
// Init Test mode pins as Input PA3 (27) adn PA4 (28)
void InitIOTestMode()
{ 
 pinMode(27, INPUT);           // set pin to input
digitalWrite(27, HIGH);       // turn on pullup resistors
pinMode(28, INPUT);           // set pin to input
digitalWrite(28, HIGH);       // turn on pullup resistors
}

//Lire les registres a decalage des boutons--------------------------------------------
//Le nom d'origine disait "with debounce time", ce qui n'est pas ce que le code fait.
//DEBOUNCE_TIME ne fait que limiter la FREQUENCE de rafraichissement de dinSr - il ne
//separe pas deux echantillons de 5 ms. Les deux lectures qu'on compare pour valider un
//etat sont separees d'UNE iteration de boucle principale, quelques centaines de
//microsecondes, pas de 5 ms. Et le second jeu de lectures SPI a lieu a CHAQUE tour de
//boucle, hors de la temporisation.
//Ca fonctionne - aucun rebond signale en jeu - mais ce n'est pas un anti-rebond
//temporise, et compter dessus comme tel serait une erreur. Si un jour un bouton se met a
//doubler, c'est ici qu'il faut regarder, et la vraie reponse serait de comparer deux
//echantillons REELLEMENT espaces.
void ScanDin()
{
  if ((millis() - debounceTimer) > DEBOUNCE_TIME)
  {
    debounceTimer = millis();
    //shiftregisted switches
    SPI.beginTransaction(SPIset); 
    SW_CS_HIGH;
    tempDin[0][0] = SPI.transfer(0);
    tempDin[1][0] = SPI.transfer(0);
    tempDin[2][0] = SPI.transfer(0);
    tempDin[3][0] = SPI.transfer(0);
    tempDin[4][0] = SPI.transfer(0);
    SW_CS_LOW;
    SPI.endTransaction();

    //µC direct pin switches
    encSwValue[0] = ENC_SW_GET;

    //Compare
    for (int a = 0; a < 5; a++){
      if (tempDin[a][0] == tempDin[a][1]) dinSr[a] = tempDin[a][0];
    }
    if (encSwValue[0] == encSwValue[1]) encSwState = encSwValue[0];
  }

  //shiftregisted switches
  SPI.beginTransaction(SPIset);
  SW_CS_HIGH;
  tempDin[0][1] = SPI.transfer(0);
  tempDin[1][1] = SPI.transfer(0);
  tempDin[2][1] = SPI.transfer(0);
  tempDin[3][1] = SPI.transfer(0);
  tempDin[4][1] = SPI.transfer(0);
  SW_CS_LOW; 
  SPI.endTransaction();

  //µC direct pin switches
  encSwValue[1] = ENC_SW_GET;
}

//Set Dout LED------------------------------------------------------------
void SetDoutLed(unsigned int stepLed, unsigned int configLed, byte menuLed)
{
  SPI.beginTransaction(SPIset);
  LED_CS_LOW;
  SPI.transfer(byte(stepLed >> 8));
  SPI.transfer(byte(stepLed));
  SPI.transfer(byte(configLed >>8));
  SPI.transfer(byte(configLed));
  SPI.transfer(byte(menuLed));
  LED_CS_HIGH;
  SPI.endTransaction(); 
}

//Set Dout Trig------------------------------------------------------------
//Le registre a decalage des trigs n'a pas de lecture : on ne peut qu'ecrire les seize
//bits d'un coup. Sans miroir logiciel, "armer un instrument de plus" est impossible -
//on ne peut qu'ecraser les quinze autres.
//C'est exactement ce que faisait MidiTrigOn() : SetDoutTrig(1 << inst) pendant une
//impulsion du sequenceur remplacait son masque par le sien et TRONQUAIT le
//declenchement en cours. Meme famille que la course TRIG-1.
//Le miroir est partage entre l'interruption Timer1 (le sequenceur), l'interruption
//Timer2 (la fin d'impulsion) et la boucle principale (le MIDI), et une ecriture 16 bits
//n'est pas atomique sur AVR : d'ou le SREG sauve et restaure autour de chaque acces.
static void WriteDoutTrig(unsigned int value)
{
  SPI.beginTransaction(SPIset);
  TRIG_CS_LOW;
  SPI.transfer(byte(value >> 8));
  SPI.transfer(byte(value));
  TRIG_CS_HIGH;
  SPI.endTransaction();
}

//Remplace : le masque du sequenceur, c'est "les instruments de CE pas".
//No analog voices doesn't have to be trigged by the shift register, it prevent "TICK"
//sound when muting - d'ou DOUT_TRIG_MASK.
void SetDoutTrig(unsigned int value)
{
  value &= DOUT_TRIG_MASK;
  byte sreg = SREG;
  cli();
  doutTrigState = value;
  WriteDoutTrig(value);
  SREG = sreg;
}

//Ajoute sans effacer : une note MIDI s'arme EN PLUS de ce qui joue deja.
//Consequence assumee : Timer2 relache tout d'un coup, donc une note arrivant au milieu
//d'une impulsion du sequenceur ALLONGE cette impulsion jusqu'a sa propre echeance au
//lieu de la tronquer. Les voix se declenchent sur le front montant - un etat haut plus
//long ne s'entend pas, un front coupe si.
void AddDoutTrig(unsigned int value)
{
  byte sreg = SREG;
  cli();
  doutTrigState |= (value & DOUT_TRIG_MASK);
  WriteDoutTrig(doutTrigState);
  SREG = sreg;
}

//Reset Dout Trig---------------------------------------------------------
void ResetDoutTrig()
{
  byte sreg = SREG;
  cli();
  doutTrigState = 0;
  WriteDoutTrig(0);
  SREG = sreg;
}

//Set Dac voltage---------------------------------------------------------
void SetDacA(byte velocity)
{
  if (velocity >= 127) velocity = 127;
  //map(velocity, 0, 127, 0, 4090) was a 32 bit division, called 12 times per SetMux().
  //4090/127 = 32.2047 and 515/16 = 32.1875, so the output is at most 3 mV low at
  //velocity 127 - 0.07 % on a DAC whose LSB is 1 mV. velocity is clamped above, so
  //127*515 = 65405 still fits in 16 bits.
  unsigned int milliVolt = ((unsigned int)velocity * 515) >> 4;
  unsigned int command = 0;
  command |= 0x1000;// Active DAC out A
  command |= (milliVolt & 0x0FFF);
  //CS asserted and settled before the transaction : with SPI.usingInterrupt(255) the
  //library masks every interrupt between begin and endTransaction, no need to spend
  //the 5 us settling time in there.
  DAC_CS_LOW;
  delayMicroseconds(5);
  SPI.beginTransaction(SPIset);
  SPI.transfer(command >> 8);
  SPI.transfer(command & 0xFF);
  DAC_CS_HIGH;
  SPI.endTransaction();
}

//Scan Din bootloader
void ScanDinBoot()
{
  SPI.beginTransaction(SPIset); 
  SW_CS_HIGH;
  SPI.transfer(0);
  SPI.transfer(0);
  btnPlayStopByte = SPI.transfer(0);
  SPI.transfer(0);
  btnEnterByte = SPI.transfer(0);
  SW_CS_LOW;
  SPI.endTransaction();
}






