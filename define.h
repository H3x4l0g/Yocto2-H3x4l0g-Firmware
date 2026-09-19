//======================================================================================
//
//   D E F I N E . H                            le vocabulaire de toute la machine
//
//   Les constantes, les types, les tables en PROGMEM - et TOUTES les variables
//   globales. Numeros d'instruments, masques de boutons, structures Pattern et Track,
//   table de swing, version du firmware.
//
//--------------------------------------------------------------------------------------
//   AVANT D'Y TOUCHER
//
//   1. CE N'EST PAS UN VRAI EN-TETE. Il DEFINIT des variables, il ne les declare pas.
//      Il n'est inclus qu'une fois, depuis yocto2.ino, et le garde #ifndef ne protege
//      que de la double lecture - pas d'une seconde unite de compilation.
//
//   2. FW_VERSION SE BUMPE AVANT CHAQUE ECRITURE DANS LA PUCE. C'est le seul moyen de
//      savoir ce que la machine contient. Un build a deja tourne en annoncant une
//      version de retard, parce qu'un script s'etait arrete avant cette ligne.
//
//   3. TOUCHER A UNE TAILLE DE STRUCTURE OU A UN OFFSET EEPROM DECALE TOUT CE QUI EST
//      DEJA ENREGISTRE. Les patterns et les pistes vivent dans les EEPROM externes et
//      survivent aux mises a jour - a condition que la disposition ne bouge pas.
//
//======================================================================================

#ifndef define_h
#define define_h

//Firmware version - shown on the LCD at boot.
//The display is 16 columns wide : the static_assert below catches a string that
//would not fit, instead of letting it wrap into the second line at runtime.
#define LCD_COLS 16
#define FW_VERSION  "Yocto v2.061"
#define FW_AUTHOR   "h3x4l0g 2026"
static_assert(sizeof(FW_VERSION) <= LCD_COLS + 1, "FW_VERSION depasse les 16 colonnes du LCD");
static_assert(sizeof(FW_AUTHOR)  <= LCD_COLS + 1, "FW_AUTHOR depasse les 16 colonnes du LCD");
//Starting column that centres each line, worked out by the compiler rather than counted
//by hand, so it stays centred when the version string changes. sizeof counts the
//terminator, hence the -1. Odd leftovers land on the left, which reads better.
#define LCD_CENTER(s) ((LCD_COLS - (sizeof(s) - 1)) / 2)

//DEBUG
#define DEBUG 0

//MIDI
#define NOTE_ON_CMD 0x90
#define NOTE_OFF_CMD 0x80
#define CLOCK_CMD 0xF8
#define START_CMD 0xFA
#define STOP_CMD 0xFC
#define ALL_NOTE_OFF 0x7B
#define CONTINU_CMD 0xFB

//Button
#define BTN_PLAY     B10000000
#define BTN_STOP     B1
#define BTN_LASTSTEP B1000000
#define BTN_SCALE    B10
#define BTN_SHUF     B100000
#define BTN_CLEAR    B100
#define BTN_INST     B10000
#define BTN_SHIFT    B1000
#define BTN_TRK  B1000
#define BTN_BACK     B100
#define BTN_FWD      B10
#define BTN_NUM      B1
#define BTN_PTRN B10000000
#define BTN_TAP      B1000000
#define BTN_DIR      B100000
#define BTN_GUIDE    B10000
#define BTN_BANK     B1
#define BTN_MUTE     B10
#define BTN_TEMPO    B100
#define BTN_ENTER    B1000
#define BTN_ENCODER  B100
#define ENC_SW_GET   PINB & BTN_ENCODER
#define ENC_PINA    0
#define ENC_PINB    1
#define JUSTPRESSED 1
#define JUSTRELEASE 0
#define HOLD_TIME 1000 //1s to trig hold 
#define NBR_BTN_STEP 16

//Led
#define LED_STOP  B1
/*#define LED_SHIFT B10
 #define LED_INST  B100
 #define LED_CLR   B1000
 #define LED_SHUF  B10000
 #define LED_LAST  B100000
 #define LED_SCALE B1000000*/
#define LED_PLAY  B10000000
#define LED_MASK  0xDD55 //mask to fade selected inst led
#define LED_MASK_OH  0xFD55 

//LCD
#define MAX_CUR_POS 4
#define MAX_CONF_PAGE 2

//Utility
//TOGGLE et MOMENTARY sont partis avec le parametre de StepButtonGet(), le 17/09/2026 :
//la branche TOGGLE n'avait plus d'appelant, et un mode a deux valeurs dont une seule
//sert n'est plus un mode.
#define TRUE 1
#define FALSE 0
#define ON 1
#define OFF 0
#define BOOTLOADER_TIME 5000 // time staying in bootloader 

//Sequencer
#define NBR_INST 16
#define NBR_STEP_BTN 16
//Seize touches de pas, mais douze voix seulement : en mode TAP les quatre dernieres ne
//portent rien. Cf le bloc TAP de Seq.ino.
#define NBR_TAP_INST 12
#define NBR_STEP 16
#define NBR_PATTERN 16 //Number of pattern in a bank
#define NBR_BANK 8
//Aucune banque armee. NBR_BANK est hors des huit, donc aucune valeur volee a une banque
//reelle : cf armedBank plus bas et SelectionBank() dans SeqFunc.ino.
#define NO_BANK NBR_BANK
#define MID_VEL 40 //velocity 0 to 127
#define HIGH_VEL 80
#define MAX_VEL 127 
//Le metronome sonne sur le rimshot : premier temps accentue, les trois autres en
//retrait. Ces deux valeurs etaient tamponnees DANS pattern[].velocity[RM][], cf
//Metronome() et le commentaire de CountPPQN().
#define MET_VEL_FIRST 127
#define MET_VEL_OTHER 30
#define MAX_SEQ_DIR 3
#define FORWARD 0
#define BACKWARD 1
#define PING_PONG 2
#define RANDOM 3
#define MAX_PTRN 128
#define MAX_TRACK 16
#define MAX_PTRN_TRACK 1024
//patternNbr[1020] and [1021] hold the stored track length (cf SaveTrack),
//so the usable track positions and length stop at 999
#define MAX_TRACK_POS 999
#define MIN_BPM 30
#define MAX_BPM 250
#define DEFAULT_BPM 120
#define PPQN 96
//Parenthesees : sans les parentheses, "p % SCALE_32" se lit "(p % 96) / 8". Le piege ne
//s'etait jamais declenche parce que ces macros ne servaient qu'a initialiser la table des
//scales et comme etiquettes de case, ou la precedence ne change rien. Il se declenche des
//qu'on ecrit une expression avec.
#define SCALE_16 (PPQN/4)
#define SCALE_32 (PPQN/8)
#define SCALE_16t (PPQN/6)
#define SCALE_8t (PPQN/3)
//Swing : the even step stays on the grid, the odd one is pushed a few ticks late.
//Swing ratio = (scale + delay) / (2 * scale).
//The delay used to be (setting - 1) ticks whatever the resolution, so one setting meant
//four different ratios : setting 7 gave 62.5 % on 16ths but 75 % on 32nds and 59.4 % on
//8th triplets. A swing is a ratio, not a tick count. The table below holds the delay that
//yields the SAME ratio on every scale, the 16th grid being the reference :
//  setting    1     2     3     4     5     6     7     8     9    10    11
//  target % 50.0  52.1  54.2  56.3  58.3  60.4  62.5  64.6  66.7  68.8  70.8
//                                                          exact triplet shuffle at 9
//Rounded to whole ticks, which is all the 96 PPQN clock can express. Worst error 2.1
//points, on the 32nd grid where one step is 12 ticks and one tick is already 4.2 points :
//there the eleven settings collapse onto six ratios, two settings each. Nothing to be
//done about that short of raising PPQN.
//The 16th row is 0..10, byte for byte what the machine did before, so no pattern written
//on the 16th grid changes.
#define MAX_SHUF_TYPE 11
//Row = (scale >> 3) - 1, which maps the four legal scales onto 0..3 with one shift :
//12 -> 0, 16 -> 1, 24 -> 2, 32 -> 3. Rows are padded to 16 so the column index is a
//mask and the row stride a shift, no multiply in the interrupt. The padding repeats the
//last value, so a corrupt setting degrades to maximum swing instead of reading garbage.
const byte swingTicks[4][16] PROGMEM = {
  { 0,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  5,  5,  5,  5,  5},//SCALE_32  32nd
  { 0,  1,  1,  2,  3,  3,  4,  5,  5,  6,  7,  7,  7,  7,  7,  7},//SCALE_16t 16th triplet
  { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 10, 10, 10, 10, 10},//SCALE_16  16th
  { 0,  1,  3,  4,  5,  7,  8,  9, 11, 12, 13, 13, 13, 13, 13, 13} //SCALE_8t  8th triplet
};
//Le rang d'une scale : (scale >> 3) - 1 envoie les quatre scales legales sur 0..3 d'un
//seul decalage - 12 vers 0, 16 vers 1, 24 vers 2, 32 vers 3. Le masque borne le resultat
//si un pattern corrompu donne autre chose. Defini une fois, utilise par la table de swing
//et par le cache des modulos de Clock.ino : les deux indexent de la meme facon.
#define SCALE_ROW(scale) ((((scale) >> 3) - 1) & 3)
//Les deux index sont bornes : un pattern corrompu donne un mauvais swing, jamais une
//lecture hors table. shuffle est base 1, d'ou le -1.
#define SWING_TICKS(scale, shuf) pgm_read_byte(&swingTicks[SCALE_ROW(scale)][((shuf) - 1) & 15])
#define DEFAULT_SHUF 1
#define DEFAULT_LEN 16
#define DEFAULT_SCALE 24
#define MASTER 0
#define SLAVE 1
#define MAX_BLOCK_LEN 16
#define POSITION 0
#define LENGTH 1
#define MAX_GLOBAL_ACC 13
#define MIN_GLOBAL_ACC 0
#define SYNC 1
#define FREE 0

//Ext inst
#define MAX_OCT 8
#define DEFAULT_OCT 3 //corresponding to +0
#define MAX_EXT_INST_NOTE 99
//Borne haute de la note externe. MidiSendNoteOn/Off envoie note + 12 : c'est ce decalage
//qui fait que l'ecran et le recepteur disent le meme nom - extNote 0 s'affiche C0 et part
//en MIDI 12, qui EST un C0 dans la convention ou 60 est le do central. Mais au-dela de
//115 la somme depasse 127 et la bibliotheque masque par & 0x7F : les douze dernieres
//valeurs repartaient dans l'octave du bas, une octave sous ce que l'ecran annoncait.
//La molette s'arrete donc a 115. Le clavier, lui, ne peut pas y arriver : a + 12*keybOct
//plafonne a 15 + 12*7 = 99.
#define MAX_EXT_NOTE 115

//trig out  and din synchro
//TR1 EST ACTIVE A L'ETAT BAS SUR LA BROCHE, ET ACTIVE A L'ETAT HAUT A LA PRISE. Releve sur
//le schema le 18/09/2026 : PA2 sort par J2, traverse la nappe, entre par J10 dans une
//resistance de 47k et arrive sur la BASE de Q86, un 2N3906 - un PNP dont l'emetteur est au
//+5V et le collecteur tire au GND par 22k. Emetteur au +5V veut dire qu'il conduit quand
//la base descend : TR1_LOW allume le transistor, et la prise MONTE.
//Consequence pratique : le front que voit l'appareil branche - module de percussion,
//entree d'horloge d'un SH-101, arpegiateur d'un JUNO-60 - est le front MONTANT de la
//prise, donc l'instant exact ou CountPPQN() ecrit TR1_LOW, dans l'interruption. Le front
//descendant, lui, est pose par SetTrigTime() dans la boucle principale et porte toute la
//gigue de celle-ci. C'est le bon partage : ce qui compte est dans l'interruption.
//Qui lirait TR1_LOW sans le schema conclurait l'inverse.
#define TR1_HIGH PORTA |= 1 << 2
#define TR1_LOW  PORTA &= ~(1 << 2)
#define TR2_HIGH PORTA |= 1 << 3
#define TR2_LOW  PORTA &= ~(1 << 3)
//#define TRIG1_PIN 26
//#define TRIG2_PIN 27
#define DIN_START_PIN 24
#define DIN_CLK_PIN 25
#define TRIG_TIME_MS 10

//Dincsync out
#define DIN_START_HIGH PORTA |= 1
#define DIN_START_LOW  PORTA &= ~(1)
#define DIN_CLK_HIGH   PORTA |= 1 << 1
#define DIN_CLK_LOW    PORTA &= ~(1 << 1)

//Dio
#define SW_CS_LOW PORTD&=~(1<<4)
#define SW_CS_HIGH PORTD|=(1<<4) 
#define LED_CS_LOW   PORTB&=~(1<<4)
#define LED_CS_HIGH  PORTB|=(1<<4)
#define TRIG_CS_LOW   PORTB&=~(1<<3)
#define TRIG_CS_HIGH  PORTB|=(1<<3)
#define DAC_CS_HIGH  PORTD|=(1<<5)
#define DAC_CS_LOW  PORTD &=~(1<<5)

#define DEBOUNCE_TIME 5

//Mux
#define MUX_ADDR_PORT PORTA
#define NBR_MUX_OUT 5
#define NBR_MUX 2
#define MUX_INH_PORT PORTD 
#define MUX_INH1_BIT 6
#define MUX_INH2_BIT 7
#define MUX_INH1_HIGH PORTD|=(1<<6)
#define MUX_INH1_LOW PORTD &=~(1<<6)
#define MUX_INH2_HIGH PORTD|=(1<<7)
#define MUX_INH2_LOW PORTD &=~(1<<7)
#define MUX_ADDR_0 PORTA &=~(1<<7)&~(1<<6)&~(1<<5)
#define MUX_ADDR_1 PORTA |=(1<<7)&~(1<<6)&~(1<<5)

//Inst button
#define BD_BTN 0
#define SD_BTN 1
#define LT_BTN 2
#define MT_BTN 3
#define HT_BTN 4
#define RM_BTN 5
#define HC_BTN 6
#define CB_BTN 7
#define CY_BTN 8
#define OH_BTN 9
#define CH_BTN 10
#define PC2_BTN 11

//Inst number  Match with IC113 and IC112 bit shift register out (cf schematic)
#define BD 0
#define SD 1
#define LT 2
#define MT 3
#define HT 4
#define RM 5
#define HC 6
#define CB 7
#define CY 8
#define OH 9
#define CH 10
//avr/io.h defines PC2 as the PORTC bit 2 mask. Nothing here uses the port bit macros,
//so shadow it deliberately rather than rename the instrument in ten call sites.
#undef PC2
#define PC2 11

#define GLOBAL_ACC 12 //
#define EXT_INST 13 //unsused shift register out but used in the pattern
#define TR1 14 //
#define TR2 15


enum SeqMode
{
  TRACK_PLAY, TRACK_WRITE, PTRN_PLAY, PTRN_STEP, PTRN_TAP, MUTE
};
//A mode belongs to one of two families, and the family decides what the transport plays :
//a single pattern on one side, a chain of patterns on the other. MUTE is not a family of
//its own - it overlays another mode, which prevSeqMode carries - so resolve MUTE before
//asking this. PTRN_TAP belongs to the pattern family.
#define IS_TRACK_MODE(m) ((m) == TRACK_PLAY || (m) == TRACK_WRITE)

//////////////////////////////variable////////////////////////////////
//Dio---------------------------------------------
byte dinSr[5]={
  0};//Store Din value of the five shift register
byte tempDin[5][2];//Used for the bounce
unsigned long debounceTimer;

//Buttons-----------------------------------------
typedef struct Button Button;
struct Button
{
  boolean curState;//1 is ON, 0 is OFF
  boolean prevState;
  boolean pressed;
  boolean justPressed;
  boolean justRelease;
  boolean hold;// 1 is holding
  byte counter;//count number of push on the button
  unsigned long curTime;
};
Button stepBtn[NBR_STEP_BTN];
Button muteStepBtn[NBR_STEP_BTN];
Button playBtn;
Button stopBtn;
Button encBtn;
Button guideBtn;
Button scaleBtn;
Button tapBtn;
Button dirBtn;
Button tempoBtn;
Button shufBtn;
Button shiftBtn, enterBtn, numBtn, lastStepBtn, backBtn, fwdBtn, muteBtn, bankBtn, stepsBtn, ptrnBtn, trkBtn, clearBtn;

boolean instBtn;
boolean doublePush = 0; //flag that CH and CH_LOW button are pressed together
unsigned int readButtonState;

//Led-----------------------------------------------
unsigned int stepLeds;
unsigned int stepLedsHigh;
unsigned int stepLedsLow;
byte menuLed;
unsigned int configLed;
volatile boolean blinkTempo = 1;
volatile boolean blinkFast[NBR_INST] = {
  1};//each instrument could blink in different scale
//stopLed est partie le 18/09/2026 : declaree depuis l'amont, jamais ecrite, jamais lue.
//La LED STOP vit dans menuLed, par LED_STOP.
boolean trackLed, backLed, fwdLed, numLed, ptrnLed, tapLed, dirLed, guideLed, bankLed, muteLed, tempoLed, instLed, shiftLed, clearLed, shufLed, scaleLed, lastStepLed;
/*boolean eighttLed;//tempo scale Leds
 boolean sixteentLed;
 boolean sixteenLed;
 boolean threetwoLed;*/
byte scaleLeds;
boolean enterLed;
unsigned int instSlctLed;//[NBR_INST]={0x00, 0x00, 0x300, 0x400, 0x800, 
byte flagLedIntensity;

//Sequencer-----------------------------------------
struct SeqConfig {
  boolean ptrnChangeSync;// 0 = free pattern change  1 = sync end measure pattern change
  boolean sync;//0 = MASTER  1 = SLAVE
  boolean syncChanged;
  byte TXchannel;//MIDI transmit channel
  byte RXchannel;
  unsigned int bpm;
  unsigned int defaultBpm;// stored in the eeprom
  byte dir;
  byte configPage;
  boolean configMode;
  boolean setupNeedSaved;
}
seq;

volatile byte ppqn = 0;
volatile byte curStep[NBR_INST] = {
  0};//current played step of each instrument
volatile int globalStepCount = -1;
volatile int stepCount[NBR_INST] = {
  -1};//counter of each instrument
volatile byte tapStepCount;//this counter is used to get a better tap response
volatile boolean stepChanged[NBR_INST] = {
  FALSE};
volatile unsigned int stepValue = 0;//value of each trig step used in "Clock"
volatile byte noteIndexCpt = 0;
boolean isRunning = FALSE;
boolean isStop = TRUE;
//Boot mode. Was PTRN_STEP, so the machine came up on the pattern edit grid : the step
//buttons toggled steps before you had chosen anything. Pattern Play is the right place
//to land - you pick a pattern, you press play. Nothing in setup() reads curSeqMode,
//every reader lives in the main loop, so this is purely the arrival point.
SeqMode curSeqMode = PTRN_PLAY;
SeqMode prevSeqMode = PTRN_PLAY;//mode overlayed by MUTE ; always written on MUTE entry
byte curInst = BD;//0 is BD trig out shift register
//LA BANQUE NE SE STOCKE PLUS : elle se lit sur curPattern, cf PlayingBank() dans
//SeqFunc.ino. curBank portait deux metiers - la lettre affichee et la base de la
//selection au pas - et le geste BANK les ecrivait ensemble, donc armer une banque
//changeait le pattern qui joue. Le recalage pose pour l'affichage a chaque chargement
//effacait de son cote l'armement des qu'une chaine avancait.
//armedBank ne porte que le geste, et vaut NO_BANK tant qu'aucun n'attend.
byte armedBank = NO_BANK;
byte curPattern = 0;// 0 to 255
byte nextPattern;
boolean changeDir[NBR_INST]; //use to PING PONG change dir
volatile boolean endMeasure;
unsigned int metronome;//masque des pas ou le metronome bat, 0 quand il est coupe
//Velocite du metronome pour le pas en cours, 0 s'il ne bat pas dessus. Posee par le
//tick, lue par MuxValue() : c'est ce qui remplace l'ecriture dans le pattern.
volatile byte metronomeVel = 0;

//Pattern-------------------------------------------
typedef struct Pattern Pattern;
struct Pattern
{
  byte globalLength;//gmlobal length of the pattern
  byte globalScale;
  byte globalDir;//0=>forward, 1=>backword, 2=>ping-pong, 3=>random
  byte globalShuffle;
  unsigned int inst[NBR_INST];
  unsigned int step[NBR_STEP];
  byte velocity[NBR_INST][NBR_STEP];
  byte probality[NBR_INST][NBR_STEP];
  byte uTiming[NBR_INST][NBR_STEP];
  byte retrig[NBR_INST][NBR_STEP];
  byte extNote[128];// DUE to EEPROM 64 bytes PAGE WRITE
  byte extLength;
  byte groupPos;
  byte groupLength;
  byte globalAcc;//Global accent value
  byte dir[NBR_INST];
  byte length[NBR_INST];
  byte scale[NBR_INST];
  byte shuffle[NBR_INST];
};
Pattern pattern[2];//current pattern and next pattern in the buffer
Pattern bufferedPattern;//to copy paste pattern
boolean ptrnBuffer = 0;
boolean patternWasEdited = FALSE;
boolean selectedPatternChanged = FALSE;
boolean nextPatternReady = FALSE;
boolean patternNeedSaved = FALSE;
byte longerInstLength;
//Scale value correspond to the scale button counter
byte scaleValue[]={
  SCALE_16, SCALE_32, SCALE_8t, SCALE_16t};

struct GroupPattern
{
  byte length;//length of the pattern bloc
  byte firstPattern;//first pattern of the bloc
  byte pos;
  boolean priority;//use to know if volatile group have priority on stored pattern group
}
group;

unsigned int tempInst[NBR_INST]={
  0};
//MUTE ET SOLO : DEUX MASQUES, ET UN ARBITRE----------------------------------------
//muteInst est ce qu'on a coupe a la main. soloInst est ce qu'on veut entendre, et il
//RECOUVRE les mutes au lieu de les ecraser : sortir du solo rend donc exactement
//l'etat de mute qu'on avait, sans comptabilite et sans rien a remettre d'accord.
//Avant, le solo posait muteInst = 0xFFFF puis liberait une voix : ce qu'on avait pose
//avant n'existait plus, et il fallait ressortir pad par pad.
//
//CE QUE LE SOLO NE BALAIE JAMAIS. Trois des seize bits ne sont pas des voix :
// - L'ACCENT GLOBAL n'est pas un son, c'est un nombre ajoute a une velocite. Soloer la
//   grosse caisse lui retirait ses accents, donc changeait SA sonorite - un solo qui
//   ment sur ce qu'il isole.
// - TR1 ET TR2 clockent le dehors : un autre sequenceur de pas, un arpege. Soloer la
//   grosse caisse ne doit pas arreter le SH-101.
//EXT_INST n'en est PAS. Chez nous cette piste pilote un instrument qui fait du son,
//donc le solo la coupe comme il coupe les voix. C'est le seul point ou l'on s'ecarte
//du firmware MSS, qui immunise les quatre.
//Le mute explicite, lui, garde les seize : couper TR2 expres pour arreter un arpege
//est un geste utile. C'est LE BALAYAGE qu'on borne, pas le geste qui nomme.
//
//volatile : ecrits dans la boucle principale, lus par l'interruption d'horloge a
//travers ActiveMute(). Sans lui, LTO peut garder la valeur dans un registre. L'acces
//16 bits reste non atomique - un pas peut sortir avec un mot a moitie ecrit, soit une
//frappe fausse a l'instant meme ou l'on appuie sur le pad. Inaudible, et borne a ca.
volatile unsigned int muteInst = 0;
volatile unsigned int soloInst = 0;

#define SOLO_IMMUNE (((unsigned int)1 << GLOBAL_ACC)                    | ((unsigned int)1 << TR1)                    | ((unsigned int)1 << TR2))

//LE SEUL MOT DE MUTE QUE LES CONSOMMATEURS ONT LE DROIT DE LIRE. Trois s'en servent :
//le masque des trigs (Clock.ino), l'accent global (Mux.ino) et les LED de pas en mode
//MUTE (Led.ino) - qui montrent donc exactement ce qui se tait, solo compris.
static inline unsigned int ActiveMute()
{
  if (soloInst) return (unsigned int)(~soloInst) & ~SOLO_IMMUNE;
  return muteInst;
}
//Les deux niveaux de la touche de pas, instrument par instrument----------------------
//Une touche de pas a trois etats : eteint, demi, plein. Ces deux tables disent quelle
//velocite chaque etat tamponne dans le pattern, POUR CHAQUE VOIX separement - le Yocto
//n'a qu'un DAC, mais douze condensateurs de maintien, donc une tension propre par
//instrument, reecrite a chaque pas. Cf Mux.ino.
//
//LE PLEIN EST 80 ET NON 127 parce que l'accent global vient PAR DESSUS : il ajoute
//globalAcc * 4, soit 52 au plus (globalAcc va de 0 a 13, borne dans Enc.ino). Il y a donc
//quatre niveaux par pas et non deux - demi, plein, demi + accent, plein + accent - et les
//deux derniers ne sont pas stockes, ils se calculent dans MuxValue().
//
//Le 80 vient de la table 909, pas d'un calcul de marge, et ce n'est pas tout a fait la
//bonne marge : 80 + 52 = 132, que SetDacA ecrete a 127. Les deux derniers crans d'accent
//sur quatorze perdent donc jusqu'a 5 unites, 161 mV sur 4,09 V, et seulement sur un pas
//plein. Le chiffre exact serait 75 - mais il coute 161 mV sur TOUS les pas pleins pour
//rattraper deux crans : le remede serait pire. Monter instVelHigh aggrave l'ecretage
//d'autant, le baisser mange le niveau du pas plein.
//
//PC2 portait un demi a 20 : 0,64 V contre 2,58 V au
//plein, un ecart de 1 a 4 quand les huit autres voix sont a 1 a 2 et les charleys a
//1 a 1,3. Trop brutal a l'oreille sur cette machine, ramene a 40 comme les autres.
//
//ATTENTION SI ON BAISSE UN SEUIL : Led.ino compare a instVelLow pour afficher un pas en
//demi ou en plein. Descendre le seuil SOUS une velocite deja enregistree ferait basculer
//d'anciens pas en plein a l'ecran. Monter est sans risque.
//Et ces tables ne sont qu'un tampon pose A LA SAISIE : les patterns deja enregistres
//gardent la velocite qu'ils portent, il faut repasser sur les pas pour les remettre.
//BD, SD, LT, MT, HT, RM, HC, CB, CY, OH, CH, PC2
//L'ENTREE 13 (EXT_INST) N'EST PAS UNE TENSION. La piste externe n'a pas de canal de
//multiplexeur - il n'y en a que douze - et sa velocite part telle quelle dans le note-on
//MIDI. Ses deux seuils sont donc des velocites MIDI, pas des volts :
//  demi  = 80  la velocite envoyee jusqu'ici en dur (HIGH_VEL), donc le pas normal
//  plein = 127 l'accent. La x0xb0x en n0nx0x2 accentue au-dela de 100 ; 127 laisse de
//              la marge quel que soit le seuil exact de la machine en face.
//Le demi passe de 40 a 80 pour que les pas DEJA ENREGISTRES, tous ecrits a 80 par
//l'ancien cycle a deux etats, se relisent comme des pas normaux et sonnent comme avant.
//C'est aussi ce qui les affiche en demi plutot qu'en plein : Led.ino compare a
//instVelLow. Monter un seuil est sans risque, cf l'avertissement ci-dessus.
byte instVelHigh[NBR_INST]={
  80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 127, 80, 80};
byte instVelLow[NBR_INST]={
  40, 40, 40, 40, 40, 40, 40, 40, 60, 60, 60, 40, 40, 80, 40, 40};

//Track----------------------------------------------
typedef struct Track Track;
struct Track
{
  unsigned int length;//lenght before loop 0 to 999
  byte patternNbr[MAX_PTRN_TRACK];//Should be 999 
};
Track track[2];

typedef struct TrackSetup TrackSetup;
struct TrackSetup
{
  byte next;
  byte current;//number of the track 0 to 15
  unsigned int pos;//track position 0 to 999
  volatile byte indexCpt;//incrementer each pattern end
  boolean wasEdited;
};
TrackSetup trk;
boolean selectedTrackChanged = FALSE;
boolean trkBuffer = 0;
boolean trackNeedSaved = FALSE;
boolean trackJustSaved = FALSE;
volatile boolean trackPosNeedIncremante = FALSE;
unsigned long timeSinceSaved;

//Ext inst-------------------------------------------
boolean keyboardMode;
byte keybOct = DEFAULT_OCT;
byte noteIndex = 0;//external inst note index

//SPI------------------------------------------------
SPISettings SPIset(4000000, MSBFIRST, SPI_MODE0);

//Encoder--------------------------------------------
//State read on the two encoder pins when the knob rests in a detent
//(both contacts open => both pins pulled HIGH)
#define ENC_DETENT_STATE 3
//Quadrature transitions to accumulate between two detents to validate one notch.
//A complete detent is +/-4, 2 keeps the decoding tolerant to a missed sample
//while still rejecting the contact bounce (which always sums back to 0).
#define ENC_NOTCH_THRESHOLD 2
boolean encSwValue[2];
boolean encSwState;
byte oldStateEnc = ENC_DETENT_STATE;
int8_t encAccu = 0;//quadrature transitions accumulated since the last detent
int8_t encNotch = 0;//-1, 0 or +1 : one validated detent waiting to be consumed
const int8_t knobDir[] = {
  0, -1,  1,  0,
  1,  0,  0, -1,
  -1,  0,  0,  1,
  0, 1, -1, 0 };

//Mux------------------------------------------------
//Les deux sources de MuxSweep() : le pattern qui joue et le mode test de voix. Des
//#define plutot qu'une enum, pour ne pas dependre de l'ordre dans lequel l'IDE genere
//les prototypes des .ino.
//(Une troisieme source, MUX_MIDI, a existe : une note MIDI reposait les douze velocites.
//Elle n'ecrit plus que la sienne, cf SetMuxTrigOne.)
#define MUX_PATTERN 0
#define MUX_TEST    1
#define MUX_TAP     2

byte muxAddr[8]={
  B111, B11, B101, B1, B110, B10, B100, B0};// Due to the  A, B, C pin on PORTA sequence
//La velocite de la derniere frappe de chaque voix en mode TAP, relue par MuxValue().
//Etait un static au fond du bloc TAP de Seq.ino, quand le balayage y vivait aussi.
//Zero pour une voix jamais frappee depuis l'allumage.
byte tapVel[NBR_INST];

//muxInst[] etait ici : la meme table identite qu'instOut[], declaree et jamais lue
//une seule fois dans tout le depot. Douze octets.

//CONDITIONS DE TRIG--------------------------------------------------------------------
//Un octet par pas et par instrument, dans pattern[].probality : NIBBLE HAUT la condition
//cyclique, NIBBLE BAS la probabilite. Le champ existait depuis l'amont - alloue, recopie
//par le copier-coller, reserve en EEPROM - sans que rien ne l'ecrive ni ne le relise.
//1536 octets de RAM qui dormaient depuis le debut du projet. Ils travaillent.
//
//UNE CONDITION CYCLIQUE EST "X SUR Y" : jouer a la Xieme mesure de chaque groupe de Y.
//1:2 donne une note une mesure sur deux, 3:4 la troisieme de quatre, 1:8 une sur huit.
//L'index 0 veut dire toujours, et sa paire n'est jamais lue.
//La probabilite s'applique APRES et independamment : 1:4 a 50 % est une note qui tombe
//une mesure sur quatre, et seulement une fois sur deux parmi celles-la.
#define COND_COUNT 12
const byte condXY[COND_COUNT][2] PROGMEM = {
  {0,0}, {1,2}, {2,2}, {1,3}, {2,3}, {3,3},
  {1,4}, {2,4}, {3,4}, {4,4}, {1,8}, {5,8}};

//LE COMPTEUR DE MESURES BOUCLE SUR 24, ET CE N'EST PAS UN CHIFFRE AU HASARD : c'est le
//plus petit commun multiple de 2, 3, 4 et 8, les seuls Y de la table. Un compteur qui
//reboucle sur 256 ferait sauter la phase des cycles en 3 a chaque tour - 256 % 3 vaut 1 -
//donc un accident toutes les 256 mesures, assez rare pour qu'on le mette sur le compte de
//soi-meme et assez regulier pour rendre fou. Sur 24, toutes les phases sont exactes.
#define COND_WRAP 24
volatile byte measureCount = 0;

//Quelles conditions passent CETTE mesure, un bit par index. Calcule une fois par mesure
//plutot qu'une fois par pas : onze modulos d'octet par mesure au lieu de deux cent
//cinquante-six, et le test par pas devient une lecture de bit.
volatile unsigned int condPassMask = 1;

//Le marqueur ecrit dans la page de reglages d'un pattern enregistre avec des conditions.
//SANS LUI, LA ZONE EEPROM N'EST PAS RELUE : elle etait reservee et jamais ecrite, donc
//une puce vierge y rend 0xFF partout - ce qui donnerait a TOUS les patterns d'avant une
//probabilite de 15 sur 16 sur chaque pas. Le marqueur rend la mise a jour sans effet sur
//ce qui est deja enregistre, et fait gagner deux pages de lecture sur ces patterns-la.
#define COND_MAGIC 0xC1

//Le tempo du maitre, mesure sur l'horloge entrante. Zero tant qu'on n'a rien mesure.
unsigned int slaveBpm = 0;

//LE CREDIT DE SOUS-TICKS DE LA SYNCHRO ESCLAVE. Chaque impulsion recue en depose quatre,
//l'interruption de Timer1 en depense un par tick et ne fait rien quand la caisse est vide.
//C'est ce qui garantit le compte exact de quatre par impulsion sans que la boucle
//principale ait a placer quoi que ce soit - cf timer.ino.
//Le plafond vaut deux impulsions : au-dela, c'est que la cadence estimee est fausse, et
//accumuler ne rattraperait rien.
#define SLAVE_TICK_MAX 8
volatile byte slaveTicks = 0;
//La phase n'est posee qu'une fois, au demarrage du transport. Ensuite le quartz la tient.
boolean slavePhaseSet = FALSE;

boolean condMode = FALSE;//l'ecran des conditions, cf LCD.ino
byte condStep = 0;//le pas qu'on y edite

//LCD------------------------------------------------
boolean needLcdUpdate = TRUE;
byte curIndex;//position of the cursor
byte curPage = 0;
byte cursorPos[MAX_CUR_POS]={
  0, 4, 8, 12};
//La quatrieme lettre etait N, pour "num". C'est le NUMERO DE PISTE, et l'appeler trk le
//dit sans qu'on ait a le deviner - la touche du panneau qui y menait s'appelle NUM, mais
//le champ, lui, designe une piste.
const char *letterUpTrackWrite[MAX_CUR_POS]={
  "P", "P", "L", "T"};
const char *letterUpExtInst[MAX_CUR_POS]={
  "I", "N", "L", "O"};
const char *letterUpConfPage1[MAX_CUR_POS]={
  "S", "B", "M", "M"};
const char *letterUpConfPage2[MAX_CUR_POS]={
  "P", "N", "N", "N"};
const char *letterUpCond[MAX_CUR_POS]={
  "P", "C", "P", "I"};

//MIDI-----------------------------------------------
volatile boolean midiNoteOnActive = FALSE;
//LA HAUTEUR REELLEMENT ENVOYEE, retenue plutot que recalculee. InitMidiNoteOff() la
//reconstruisait depuis extNote[noteIndexCpt - 1], c'est-a-dire depuis un index et une
//pile que TROIS autres chemins ont le droit de deplacer entre le note-on et le note-off :
//le bouton PLAY qui remet noteIndexCpt a zero, le coller de pattern qui reecrit extNote[],
//et le mode clavier qui y ecrit note par note. Le note-off partait alors sur une autre
//hauteur, et celle qui sonnait restait en l'air pour toujours.
//Un octet, et la classe entiere disparait : on relache exactement ce qu'on a envoye.
volatile byte midiNoteOnPitch = 0;
boolean midiStart;
boolean midiStop;
boolean midiContinue;
boolean instWasMidiTrigged[NBR_INST] ={
  FALSE};
//Miroir du registre a decalage des trigs, et les quatre bits qui n'y vont pas : pas de
//voix analogique derriere, et les ecrire produisait un "tick" au mute.
volatile unsigned int doutTrigState = 0;
#define DOUT_TRIG_MASK ((unsigned int)~((1 << GLOBAL_ACC) | (1 << EXT_INST) | (1 << TR1) | (1 << TR2)))
//Les instruments qu'une salve de notes MIDI a armes et qui attendent d'etre declenches
//ensemble, en fin de vidange de la file MIDI.
unsigned int midiTrigPending = 0;

//Din synchro----------------------------------------
boolean dinStartState = LOW;
//dinClkState est partie le 18/09/2026 : ecrite a trois endroits, relue nulle part.
//L'etat de la sortie DIN, c'est la broche.

//trigState----------------------------------------
//volatile : written by the sequencer ISR (Clock.ino), read by SetTrigTime() in the
//main loop. Without it the compiler is free to cache the read across loop iterations.
volatile boolean trig1State = LOW;
boolean trig2State = LOW;
unsigned long trigTime = 0;
boolean trigTimeOk;

//Bootloader-----------------------------------------
unsigned long bootElapseTime = 0;
byte btnPlayStopByte = 0;
byte btnEnterByte = 0;

//Test Mode varibale
boolean userInterfaceTestMode = LOW;
boolean voicesTestMode = LOW;

#endif//end if define_h


