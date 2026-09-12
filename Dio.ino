//-------------------------------------------------
//                  Yocto v2.x
//                  Dio utility
//-------------------------------------------------

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
  SPI.usingInterrupt(255);
  I2cBusRecover();//must run before Wire.begin() takes the pins over
  Wire.begin();
  //Never wait forever on the bus either. The biggest transaction here is 130 bytes,
  //about 12 ms at 100 kHz, so 50 ms is twenty times the "forever" this used to be and
  //still four times any legitimate transfer. On timeout the peripheral is reset, which
  //releases the lines.
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

//Scan DIN shift register with debounce time-------------------------------
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
void SetDoutTrig(unsigned int value)
{
  //No analog voices doesn't have to be trigged by the shift register
  //It prevent "TICK" sound when muting
  value &= ~((1 << TOTAL_ACC) | (1 << EXT_INST) | (1 << TR1) | (1 << TR2));
  SPI.beginTransaction(SPIset);
  TRIG_CS_LOW;
  SPI.transfer(byte(value >> 8));
  SPI.transfer(byte(value));
  TRIG_CS_HIGH;
  SPI.endTransaction(); 
}

//Reset Dout Trig---------------------------------------------------------
void ResetDoutTrig()
{
  SPI.beginTransaction(SPIset);
  TRIG_CS_LOW;
  SPI.transfer(0);
  SPI.transfer(0);
  TRIG_CS_HIGH;
  SPI.endTransaction();
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






