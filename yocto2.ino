//======================================================================================
//
//   Y O C T O 2 . I N O                        le point d'entree
//
//   setup() reveille la machine dans le bon ordre - et l'ordre compte : les registres a
//   decalage sortent de la mise sous tension avec n'importe quoi dedans, un trig ouvert
//   suffisant a faire osciller la grosse caisse. loop() ne fait ensuite qu'appeler les
//   neuf etages dans l'ordre, indefiniment.
//
//--------------------------------------------------------------------------------------
//   LA BOUCLE, DANS L'ORDRE
//
//   MIDI.read() en rafale   vider la file d'un coup, puis MidiTrigFlush()
//   ButtonGet()             les cinq octets de boutons -> des gestes
//   EncGet()                un cran de molette -> un destinataire
//   SetLeds()               reconstruire le panneau
//   SeqConfiguration()      le mode config
//   SeqParameter()          tout le reste des touches
//   KeyboardUpdate()        le mode clavier
//   LcdUpdate()             seulement si needLcdUpdate
//   SetTrigTime()           relacher les sorties de trig echues
//
//======================================================================================

/////////////////////Include/////////////////////
#include <SPI.h>
#include <LiquidCrystal.h>
#include <WireN.h>
#include "define.h"
#include "lcdstrings.h"
#include <MIDI.h>
#include <EEPROM.h>

#if DEBUG
#include <MemoryFree.h>
#endif

//MIDI is wired on the second UART. With the v3.2 library this was a USE_SERIAL_PORT
//edit inside MIDI.h ; from 5.x the port belongs to the sketch.
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

LiquidCrystal lcd(18, 19, 20, 21, 22, 23);

////////////////////////Setup//////////////////////
void setup()
{

  InitIOTestMode();
  //Voices test Mode
  if(bitRead(PINA, 3)== LOW){
    voicesTestMode = HIGH;
  }
  //User interface Test Mode
  if(bitRead (PINA, 4) == LOW){
    userInterfaceTestMode = HIGH;
  }

  InitIO();//cf Dio
  InitButtonCounter();//cf Button

  //The shift registers come out of power up holding whatever the bus left in them, so
  //the panel lights at random and the trigger outputs can sit in any state - an open
  //trigger is enough to make the BD oscillate. Drive them to a known state before
  //anything else. Same fix zabox applied to the NAVA, "[1.028] no random trigger pin
  //states at startup".
  SetDoutTrig(0);
  SetDoutLed(0, 0, 0);

#if DEBUG
  Serial.begin(115200);
  Serial.print("freeMemory()=");
  Serial.println(freeMemory());
#endif


  //The HD44780 needs more than 40 ms after Vcc has reached its level before it accepts
  //anything. LiquidCrystal::begin() waits 50 ms, but it counts from the moment it is
  //called, not from power on - and the MCU is up and running long before a slow rail
  //has settled. When begin() lands too early the controller ignores it and the display
  //stays on its power up pattern, the top row all pixels on, whatever is printed after.
  delay(100);
  lcd.begin(16, 2);
  lcd.createChar(0,font0);
  lcd.createChar(1,font1);
  lcd.createChar(2,font2);
  lcd.createChar(3,font3);
  lcd.createChar(4,font4);
  lcd.createChar(5,font5);



  //==================================================================================================
  //Auto Init EEprom
  //if the last (address 262137, 262138, 262139)  bytes of the first EEprom is != to respectivly 0, 1, 2
  // is that the external EEPROMs wasn't init  (Should be execute only ONE time)
  //================================================================================================== 

  WireBeginTX(262137); 
  Wire.endTransmission();
  //Derniere lecture I2C qui calculait son adresse de puce a la main - 0x53 se trouvait
  //juste, 262137 >> 16 valant bien 3, mais c'etait la dixieme copie de la regle que
  //WireRequestFrom() existe pour tenir. L'en-tete d'EEprom.ino promet deux portes
  //d'entree et une seule ; c'est vrai maintenant.
  WireRequestFrom(262137, 3);
  byte data1 = (Wire.read() & 0xFF);
  byte data2 = (Wire.read() & 0xFF);
  byte data3 = (Wire.read() & 0xFF);
  if (data1 + data2 + data3 != 3){
    LcdPrintEEpromInit();
    InitEEprom();
  }

  ScanDinBoot();

  //TM2 adjustement for velocity
  if (btnEnterByte == BTN_ENTER){
    LcdPrintTM2Adjust();
    while (1){
      SetDacA(MAX_VEL);
    }
  }


  InitSeq();// cf Seq
  //Load default track
  LoadTrack(0);
  //Load default pattern
  LoadPattern(0);
  ptrnBuffer = !ptrnBuffer;
  InitPattern();
  InstToStepWord();
  SetDoutTrig(0);

  //OMNI, ET LE FILTRE EST DANS LE HANDLER. setInputChannel() n'etait appele qu'ici, au
  //demarrage : changer le canal de reception dans le mode config ecrivait bien
  //seq.RXchannel (Enc.ino), mais la bibliotheque continuait de filtrer sur la valeur du
  //boot. Les notes du nouveau canal etaient jetees avant d'arriver au handler, celles de
  //l'ancien rejetees par le test du handler - plus une seule note ne passait, jusqu'au
  //prochain allumage.
  //Deux sources pour une meme verite, dont une seule se mettait a jour. HandleNoteOn() et
  //HandleNoteOff() comparent deja channel a seq.RXchannel a chaque note : c'est le filtre,
  //il lit la valeur courante, et il est le seul.
  MIDI.begin(MIDI_CHANNEL_OMNI);//opens Serial1 at 31250 baud
  //Le thru LOGICIEL de la bibliotheque, a ne pas confondre avec le jack THRU cable en
  //dur - celui-la fait son travail et n'est pas concerne.
  //serialMIDI.h pose thruActivated = true par defaut, et MIDI.hpp renvoie vers la SORTIE
  //tout ce qui arrive a l'entree : Clock, Start, Stop et Continue sont explicitement
  //dans la liste. En synchro esclave, OUT portait donc l'horloge fabriquee par
  //CountPPQN() ET celle recue, soit 48 ppqn au lieu de 24 pour tout appareil chaine
  //derriere. La NAVA coupe le thru pour cette raison exacte ("fixes double real time
  //messages on midi out").
  //Ce qu'on perd : les notes entrantes ne sont plus recopiees vers OUT. La sortie ne
  //porte de toute facon que la piste EXT et l'ecoute au clavier - aucune voix analogique
  //n'y est envoyee - donc faire transiter un clavier a travers la machine etait la seule
  //usage de cette recopie.
  MIDI.turnThruOff();
  MIDI.setHandleNoteOn(HandleNoteOn);
  MIDI.setHandleNoteOff(HandleNoteOff);

  sei();

  //Testing Modes -------------------------------
  if(voicesTestMode){
    VoicesTestMode();
  }
  if(userInterfaceTestMode){
    UserInterfaceTestMode();
  }
  //-----------------------------------------------

  lcd.clear();//the EEPROM auto init screen may still be up
  lcd.setCursor(LCD_CENTER(FW_VERSION), 0);
  lcd.print(FW_VERSION);
  lcd.setCursor(LCD_CENTER(FW_AUTHOR), 1);
  lcd.print(FW_AUTHOR);
  delay(1000);
  BootSweep();//one more second of version, spent on a chase light

}

////////////////////////Loop///////////////////////
void loop()
{  
  InitMidiRealTime();
  //Vider la file MIDI en un seul passage. MIDI.read() ne rend qu'UN message par appel :
  //un appel par tour de boucle etalait un accord de trois futs sur trois tours. Les
  //declenchements s'accumulent dans midiTrigPending et partent ensemble juste apres.
  //Borne a 16 messages pour qu'un flux dense ne puisse pas retenir la boucle : le
  //reliquat sera traite au tour suivant.
  byte midiMsg = 0;
  while (MIDI.read() && ++midiMsg < 16);
  MidiTrigFlush();
  ButtonGet(); 
  EncGet();  
  SetLeds(); 
  SeqConfiguration(); 
  SeqParameter();
  KeyboardUpdate();
  LcdUpdate();
  SetTrigTime();
}




































