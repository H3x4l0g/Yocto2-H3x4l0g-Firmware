//======================================================================================
//
//   E E P R O M . I N O                        la memoire, et sa carte
//
//   Deux 24M02 sur le meme bus I2C, 512 ko en tout. C'est ici que vivent les 128
//   patterns, les pistes et le setup - et c'est pour ca qu'une mise a jour du firmware
//   ne touche jamais a ce que tu as enregistre.
//
//--------------------------------------------------------------------------------------
//   AVANT D'Y TOUCHER
//
//   1. LA CARTE CI-DESSOUS PORTE TOUT CE QUI EST DEJA ENREGISTRE. Changer une taille ou
//      un offset decale les 128 patterns d'un coup. Les zones probabilite, micro-timing
//      et retrig sont reservees et comptees dans PTRN_SIZE alors que rien ne les ecrit :
//      c'est volontaire, les retirer casserait toutes les sauvegardes existantes.
//
//   1bis. AUCUN TRANSFERT NE CHEVAUCHE LA FRONTIERE ENTRE LES DEUX PUCES, et c'est ce
//      qui rend le point 1 vital. Une page de 128 octets qui franchirait 65536 ne
//      passerait PAS sur la puce suivante : elle reboucleraient a l'adresse 0 de la meme
//      puce, ecrasant le pattern 0. Si ca n'arrive jamais, c'est que toutes les zones
//      tombent sur des multiples de 128 - PTRN_SIZE vaut 1280, les regions sont a 0, 128,
//      384, 1152 - et que 65536 en est un aussi. Verifie au plus juste : la page de
//      velocite du pattern 51 occupe 65408 a 65535, soit pile les 128 derniers octets de
//      la premiere puce. Changer une taille pour une valeur qui n'est pas un multiple de
//      128, c'est ouvrir ce trou.
//
//   2. L'ADRESSE DE LA PUCE SUIT L'ADRESSE DE L'OCTET. Les bits 16 et au-dela designent
//      le boitier : HRDW_ADDRESS + (adress >> 16). Ce calcul etait recopie a neuf
//      endroits et deux l'avaient oublie - une lecture rendait alors les donnees de
//      l'autre puce au-dela du pattern 51. D'ou WireBeginTX() et WireRequestFrom(),
//      qui sont les deux seules portes d'entree.
//
//   3. SavePattern() BLOQUE PLUSIEURS DIZAINES DE MILLISECONDES, neuf pages a ecrire
//      avec leur temporisation. Les MIDI.read() qui la parsement sont la pour que la
//      synchro esclave ne decroche pas pendant ce temps - ce ne sont pas des restes.
//
//   4. TOUTE ECRITURE FINIT PAR WireEndTX(), JAMAIS PAR Wire.endTransmission() NU. C'est
//      elle qui releve le code d'erreur et attend la puce ; une ecriture qui la
//      contournerait pourrait de nouveau echouer sans que l'ecran le dise.
//
//--------------------------------------------------------------------------------------
//   LA CARTE
//
// The Drum sequencer has two 24M02 I2C EEPROM
// Each EEPROM is 4 x 65535 bytes so in total 2 x 4 x 65535 = 524280 bytes (512kB)
// A pattern is composed to:
//  -16 instruments  which is:
//              -Trig value   = 2 bytes  0 to 16 steps so it is 16bits value
//              -Length value = 1 byte   0 to 16 
//              -Scale value  = 1 byte   0 to 3  (1/16, 1/32, 1/8t or 1/16t)
//              -Shuffle value= 1 byte   
//              -Dir. value   = 1 byte
//              -Velocity     = 16 bytes  one for each step 0 to 127
//              -Probality    = 16 bytes  one for each step 0 to 100%
//              -uTiming      = 16 bytes  0 to 24  (in real -12 to +12 but can't store negative value)
//              -Retrig       = 16 bytes  TO BE DEFINE
//                     TOTAL  = 70 bytes per intruments
//
//  -Pattern External Instr= 128 bytes  (for 128 notes)
//  -Pattern global length = 1 byte
//  -Pattern global scale  = 1 byte
//  -Pattern global shuffle= 1 byte
//  -Pattern global dir.   = 1 byte
//  -Pattern group length  = 1 byte
//  -Pattern group pos.    = 1 byte
//  -Pattern active Acc    = 2 bytes 
//  -UNUSED pattern setup  = 24 bytes
// 
//  PATTERN_SIZE = (70 x 16) +  = 1280 bytes (9 pages of 128 bytes)
//  OFFSET_PATTERN = 0
//
// Setup size:
//  -OFFSET_SETUP = TRACK_OFFSET + (TRACK_SIZE * MAX_TRACK) = 73728;
//  -SETUP_SIZE = 64 bytes  only 4 bytes used NOW...
//
//======================================================================================

#define PTRN_SIZE          (PTRN_TRIG_SIZE + PTRN_SETUP_SIZE + PTRN_VEL_SIZE + PTRN_PROB_SIZE + PTRN_uTIMING_SIZE + PTRN_RETRIG_SIZE + PTRN_EXT_SIZE) //1280 bytes
#define PTRN_OFFSET        (unsigned long)(0)
#define PTRN_TRIG_SIZE     (unsigned long)(96)
#define PTRN_TRIG_OFFSET   (PTRN_OFFSET + PTRN_TRIG_SIZE)
#define PTRN_SETUP_SIZE    (unsigned long)(32)
#define PTRN_SETUP_OFFSET  (PTRN_TRIG_OFFSET + PTRN_SETUP_SIZE)
#define PTRN_VEL_SIZE      (unsigned long)(256)
#define PTRN_VEL_OFFSET    (PTRN_SETUP_OFFSET + PTRN_VEL_SIZE)

//Probabilite, micro-timing et retrig : trois champs de Pattern (define.h) et trois zones
//reservees ici, mais RIEN ne les ecrit ni ne les relit - les boucles de sauvegarde et de
//chargement etaient en commentaire depuis l'amont, et ont ete retirees le 13/09/2026.
//Les tailles restent dans PTRN_SIZE : les changer decalerait tous les patterns deja
//enregistres. La place est donc reservee pour le jour ou ces fonctions existeront.
//A noter : PTRN_RETRIG_OFFSET sert deja a stocker extNote, cf LoadPattern().

#define PTRN_PROB_SIZE     (unsigned long)(256)
#define PTRN_PROB_OFFSET   (PTRN_VEL_OFFSET + PTRN_PROB_SIZE)
#define PTRN_uTIMING_SIZE  (unsigned long)(256)
#define PTRN_uTIMING_OFFSET (PTRN_PROB_OFFSET + PTRN_uTIMING_SIZE)
#define PTRN_RETRIG_SIZE   (unsigned long)(256)
#define PTRN_RETRIG_OFFSET (PTRN_uTIMING_OFFSET + PTRN_RETRIG_SIZE)
#define PTRN_EXT_SIZE      (unsigned long)(128)
#define PTRN_EXT_OFFSET    (PTRN_RETRIG_OFFSET + PTRN_EXT_SIZE)
#define TRACK_SIZE         (unsigned long)(1024)//unused at this time but could be use to store Pattern Chain
#define TRACK_OFFSET       (PTRN_SIZE * MAX_PTRN)
#define OFFSET_SETUP       (TRACK_OFFSET + (TRACK_SIZE * MAX_TRACK))
#define SETUP_SIZE         (unsigned long)(64)
#define OFFSET_GROUP       (unsigned long)(37) //save only .groupPos and .groupLength
#define GROUP_SIZE         (unsigned long)(2)
#define MAX_PAGE_SIZE      (unsigned long)(128)
//LE TAMPON I2C DOIT TENIR UNE PAGE ENTIERE, ET CE N'EST PAS LE CAS PAR DEFAUT.
//yocto2.ino inclut <WireN.h> et non <Wire.h> : c'est une bibliotheque du sketchbook dont
//le seul interet est BUFFER_LENGTH a 132. Le Wire livre avec MightyCore plafonne a 32, et
//son write() JETTE EN SILENCE tout ce qui depasse - il pose un write error que personne
//ne lit et rend 0. Une page de velocite fait 2 octets d'adresse plus 128 de donnees, soit
//130 : avec le Wire standard, chaque sauvegarde de pattern serait tronquee a 30 octets
//utiles, et ca compilerait sans un avertissement.
//132 = 128 + 2 + 2 de marge. Le chiffre a ete choisi pour cette machine, et rien ne le
//disait. Cette verification transforme une perte de donnees muette en erreur de
//compilation.
static_assert(BUFFER_LENGTH >= MAX_PAGE_SIZE + 2,
              "Tampon I2C trop petit : il faut WireN (BUFFER_LENGTH 132), pas Wire (32)");
//RELAYER LE MIDI PENDANT LES TRANSFERTS LONGS-------------------------------------------
//Les "MIDI.read()" semes dans les boucles de ce fichier ne sont pas decoratifs. En synchro
//esclave, c'est HandleClock() qui depose les credits de sous-ticks (timer.ino), et il n'est
//appele que par MIDI.read() depuis la boucle principale : tant qu'un transfert EEPROM tient
//la boucle, la caisse se vide, le timer s'arrete et LA MACHINE PREND DU RETARD SUR LE
//MAITRE. Les impulsions s'accumulent dans le tampon de l'UART et redeposent tout d'un coup
//au retour - mais un timer qui tourne exactement a la cadence d'arrivee ne resorbe jamais
//ce retard-la. Il s'installe, et le changement de pattern suivant s'y ajoute.
//
//C'est ce qui derivait au changement de pattern, et SEULEMENT en esclave : en maitre, le
//timer bat tout seul et se moque de ce que fait la boucle.
//
//SavePattern() en avait cinq depuis l'amont, LoadPattern() aucun - alors que c'est le
//chargement qui tourne a CHAQUE changement de pattern, quand la sauvegarde demande une
//pression sur ENTER. L'asymetrie datait du debut. Ajoutes le 18/09/2026 dans les deux
//chargements et dans la sauvegarde de piste, qui fait huit pages.

#define HRDW_ADDRESS       0x50
#define HRDW_ADDRESS_UP    (0x50 | B100)

//L'EEPROM DIT ELLE-MEME QUAND ELLE A FINI------------------------------------------------
//Apres un endTransmission() qui a ecrit une page, la puce entre dans son cycle d'ecriture
//interne et NAK sa propre adresse tant qu'il dure. On l'interroge donc au lieu de deviner :
//beginTransmission puis endTransmission a vide, et un zero veut dire "c'est ecrit".
//
//Ca remplace seize delay(DELAY_WR) de 10 ms - le DOUBLE des 5 ms que la fiche donne pour
//maximum, et plus du triple des ~3 ms que la puce prend vraiment. Une sauvegarde de pattern
//fait quatre pages, une de piste huit : 40 a 80 ms de delay() qui bloquaient TOUT, la
//relache de TRIG-1 comme MIDI.read(), donc l'horloge en synchro esclave. L'attente devient
//juste au lieu d'etre devinee, et elle reste correcte sur une puce lente.
//
//MIDI.read() dans la boucle : c'est le meme geste que les appels deja semes au fil des
//sauvegardes de ce fichier, et pour la meme raison. Le bus I2C est libre a cet instant - la
//transaction est finie - et CountPPQN() ne touche qu'au SPI et a l'UART, jamais a l'I2C.
//
//BORNEE. Une puce morte ou absente NAK pour toujours : sans borne, le panneau se figerait
//sans rien dire. 400 essais a ~30 us l'un plafonnent a une douzaine de millisecondes, deja
//plus que le maximum de la fiche - au-dela on rend la main, et l'ecriture suivante echouera
//visiblement plutot que de figer la machine.
//
//L'idee vient du firmware MSS pour la NAVA, EepWaitReady().
#define WIRE_READY_TRIES   400

//La puce visee par la derniere transaction : WireBeginTX() la calcule deja depuis l'adresse
//sur 24 bits, et il n'y a aucune raison de refaire ce calcul ici - c'est le meme piege que
//les neuf recopies du choix de puce, dont deux s'etaient trompees.
static byte wireDevice = HRDW_ADDRESS;

//Rend FALSE si la puce n'est jamais revenue : c'est une ecriture ratee, cf WireEndTX().
//Les NAK de la scrutation elle-meme ne sont PAS des erreurs - c'est la puce qui dit
//qu'elle ecrit encore - et ne sont donc pas releves.
boolean WireWaitReady()
{
  for (unsigned int t = 0; t < WIRE_READY_TRIES; t++){
    Wire.beginTransmission(wireDevice);
    if (Wire.endTransmission() == 0) return TRUE;//la puce repond : le cycle est fini
    MIDI.read();//l'horloge esclave ne s'arrete pas pour une sauvegarde
  }
  return FALSE;
}

//LE SEUL POINT DE SORTIE D'UNE ECRITURE---------------------------------------------------
//Wire.endTransmission() rend un code - 0 si tout est passe, 2 si la puce n'a pas repondu a
//son adresse, 3 si elle a refuse un octet, 4 pour une erreur de bus, 5 pour un bus fige -
//et AUCUNE des dix-neuf ecritures de ce fichier ne le lisait. Une puce occupee, mal
//contactee ou absente perdait donc sa page sans un mot, et on ne l'apprenait qu'au
//rechargement : sur le seul chemin de la machine qui soit irreversible, aucun garde-fou.
//Les dix-neuf passent maintenant par ici, pendant de WireBeginTX(). On garde le PREMIER
//code rencontre dans eepromError, plus EEP_ERR_NOT_READY si la puce ne sort jamais de son
//cycle d'ecriture ; la fin de LcdUpdate() le montre a la place de "[SAVED!]", et le
//remet a zero. Le premier et pas le dernier : une puce qui lache fait echouer toutes les
//pages suivantes, et c'est la premiere qui dit pourquoi.
//RIEN N'EST RETENTE, et les pages suivantes sont ecrites quand meme. Une ecriture qui
//echoue une fois sur un faux contact doit SE VOIR : la rattraper en silence cacherait
//justement ce que ce garde-fou est la pour montrer. Et s'arreter a la premiere page ratee
//ne rendrait pas la sauvegarde plus entiere - celles d'avant sont deja ecrites.
//Les LECTURES ne passent pas ici : une lecture ratee charge un pattern faux en RAM, mais
//ne touche pas a ce qui est enregistre.
void WireEndTX()
{
  byte err = Wire.endTransmission();
  if (err && !eepromError) eepromError = err;
  if (!WireWaitReady() && !eepromError) eepromError = EEP_ERR_NOT_READY;
}

////////////////////////Function//////////////////////

//save pattern
void SavePattern(byte patternNbr)
{
  unsigned long adress = (unsigned long)(PTRN_OFFSET + patternNbr * PTRN_SIZE);
  WireBeginTX(adress); 
  //TRIG-----------------------------------------------
  for (byte i = 0; i < NBR_INST; i++){ 
    byte lowbyte =  (pattern[ptrnBuffer].inst[i] & 0xFF);
    byte highbyte = (pattern[ptrnBuffer].inst[i] >> 8) & 0xFF;
    Wire.write((byte)(lowbyte)); 
    Wire.write((byte)(highbyte));
    Wire.write((byte)(pattern[ptrnBuffer].length[i]));
    Wire.write((byte)(pattern[ptrnBuffer].scale[i]));
    Wire.write((byte)(pattern[ptrnBuffer].shuffle[i]));
    Wire.write((byte)(pattern[ptrnBuffer].dir[i]));
     MIDI.read();// to improve slave sync response
  }//96 bytes

  //GLOBAL SETUP-----------------------------------------------
  Wire.write((byte)(pattern[ptrnBuffer].globalLength));
  Wire.write((byte)(pattern[ptrnBuffer].globalScale));
  Wire.write((byte)(pattern[ptrnBuffer].globalShuffle));
  Wire.write((byte)(pattern[ptrnBuffer].globalDir));
  Wire.write((byte)(pattern[ptrnBuffer].groupPos));
  Wire.write((byte)(pattern[ptrnBuffer].groupLength));
  Wire.write((byte)(pattern[ptrnBuffer].globalAcc));
  //La longueur de la pile de notes EXT. Elle n'etait PAS enregistree : extNote[] revenait
  //au complet, sans son point de bouclage, qui gardait alors la valeur laissee par le
  //pattern precedemment charge dans ce buffer et ne survivait a aucune coupure. La piste
  //EXT n'a donc jamais ete enregistrable. Le copier-coller la transportait, lui
  //(SeqFunc.ino), ce qui trahit l'oubli plutot que le choix.
  //Elle se range dans la queue inutilisee de la page : aucun offset ne bouge, aucun
  //pattern deja enregistre n'est decale.
  Wire.write((byte)(pattern[ptrnBuffer].extLength));
  //LE MARQUEUR DES CONDITIONS DE TRIG. La zone qui les porte etait reservee dans la carte
  //depuis l'amont et JAMAIS ECRITE : une puce vierge y rend 0xFF partout, ce qui donnerait
  //a tous les patterns d'avant une probabilite de 15 sur 16 sur chacun de leurs pas. Sans
  //ce marqueur dans la page de reglages - qui, elle, a toujours ete ecrite - la zone n'est
  //pas relue et les conditions partent a zero. Un pattern d'avant se charge donc
  //exactement comme avant, et on economise deux pages de lecture au passage.
  Wire.write((byte)(COND_MAGIC));
  Wire.write((byte)(RTG_MAGIC));//le roulement, meme role et meme raison
  Wire.write((byte)(UTIM_MAGIC));//le decalage, idem
  //107 bytes => 21 byte left ;)

  WireEndTX();//end page transmission

  MIDI.read();// to improve slave sync response

  //VELOCITY-----------------------------------------------
  for(int nbrPage = 0; nbrPage < 2; nbrPage++){
    adress = (unsigned long)(PTRN_OFFSET + (patternNbr * PTRN_SIZE) + (MAX_PAGE_SIZE * nbrPage) + PTRN_SETUP_OFFSET);
    WireBeginTX(adress);
    for (byte i = 0; i < 8; i++){//loop as many instrument for a page
      for (byte j = 0; j < NBR_STEP; j++){
        Wire.write((byte)(pattern[ptrnBuffer].velocity[i + (nbrPage * 8)][j] & 0xFF)); 
      }
      MIDI.read();// to improve slave sync response
    }
    WireEndTX();//end of 128 bytes transfer

    
  }//2 * 128 bytes = 256 bytes

  //======================================NOT USED YET=============================================
  //========================================================================================================

  //CONDITIONS DE TRIG-------------------------------------
  //Deux pages de 128 dans la zone reservee a la probabilite, a PTRN_VEL_OFFSET. Elle
  //attendait depuis l'amont : aucun offset ne bouge, aucun pattern deja enregistre n'est
  //decale. Huit instruments par page, seize pas chacun.
  for(int nbrPage = 0; nbrPage < 2; nbrPage++){
    adress = (unsigned long)(PTRN_OFFSET + (patternNbr * PTRN_SIZE) + (MAX_PAGE_SIZE * nbrPage) + PTRN_VEL_OFFSET);
    WireBeginTX(adress);
    for (byte i = 0; i < 8; i++){
      for (byte j = 0; j < NBR_STEP; j++){
        Wire.write((byte)(pattern[ptrnBuffer].probality[i + (nbrPage * 8)][j]));
      }
      MIDI.read();// to improve slave sync response
    }
    WireEndTX();//end of 128 bytes transfer
  }//2 * 128 bytes = 256 bytes

  //ROULEMENT (RETRIG)-------------------------------------
  //Deux pages dans la zone reservee au micro-timing, a PTRN_PROB_OFFSET. ATTENTION au
  //decalage des noms : dans ce fichier chaque _OFFSET designe le DEBUT DE LA ZONE
  //SUIVANTE, parce qu'il vaut l'offset precedent PLUS sa propre taille. Les velocites
  //s'ecrivent donc a PTRN_SETUP_OFFSET, les conditions a PTRN_VEL_OFFSET, le roulement
  //ici a PTRN_PROB_OFFSET, et extNote plus bas a PTRN_RETRIG_OFFSET. La convention vient
  //de l'amont et on ne la change pas : se tromper d'un cran en renommant decalerait la
  //lecture de tous les patterns deja enregistres.
  for(int nbrPage = 0; nbrPage < 2; nbrPage++){
    adress = (unsigned long)(PTRN_OFFSET + (patternNbr * PTRN_SIZE) + (MAX_PAGE_SIZE * nbrPage) + PTRN_PROB_OFFSET);
    WireBeginTX(adress);
    for (byte i = 0; i < 8; i++){
      for (byte j = 0; j < NBR_STEP; j++){
        Wire.write((byte)(pattern[ptrnBuffer].retrig[i + (nbrPage * 8)][j]));
      }
      MIDI.read();// to improve slave sync response
    }
    WireEndTX();//end of 128 bytes transfer
  }//2 * 128 bytes = 256 bytes

  //MICRO-TIMING-------------------------------------------
  //Deux pages a PTRN_uTIMING_OFFSET - la zone nommee RETRIG, cf le decalage des noms
  //explique au-dessus : chaque _OFFSET designe le debut de la zone SUIVANTE.
  for(int nbrPage = 0; nbrPage < 2; nbrPage++){
    adress = (unsigned long)(PTRN_OFFSET + (patternNbr * PTRN_SIZE) + (MAX_PAGE_SIZE * nbrPage) + PTRN_uTIMING_OFFSET);
    WireBeginTX(adress);
    for (byte i = 0; i < 8; i++){
      for (byte j = 0; j < NBR_STEP; j++){
        Wire.write((byte)(pattern[ptrnBuffer].uTiming[i + (nbrPage * 8)][j]));
      }
      MIDI.read();// to improve slave sync response
    }
    WireEndTX();//end of 128 bytes transfer
  }//2 * 128 bytes = 256 bytes

  //EXT INST-----------------------------------------------
  adress = (unsigned long)(PTRN_OFFSET + (patternNbr * PTRN_SIZE) + PTRN_RETRIG_OFFSET);
  WireBeginTX(adress);
  for (byte j = 0; j < MAX_PAGE_SIZE; j++){
    Wire.write((byte)(pattern[ptrnBuffer].extNote[j] & 0xFF));
  }//128 bytes  
  MIDI.read();// to improve slave sync response
  WireEndTX();//end page transmission
  // 128 bytes

  
  
}//END OF SAVE PATTERN FUNCTION

/////////////////////////////////////////////////-LOAD-////////////////////////////////
//Load Pattern
void LoadPattern(byte patternNbr)
{
  unsigned long adress = (unsigned long)(PTRN_OFFSET + patternNbr * PTRN_SIZE);
  WireBeginTX(adress); 
  Wire.endTransmission();
  WireRequestFrom(adress, MAX_PAGE_SIZE);
  //TRIG-----------------------------------------------
  for(int i =0; i<NBR_INST;i++){
    //Le C++ ne garantit pas l'ordre d'evaluation de deux Wire.read() dans la meme
    //expression. avr-gcc 7.3 lit bien l'octet faible en premier a -O0, -O1, -O2, -O3 et
    //-Os - verifie - donc aucun degat aujourd'hui. Mais rien ne l'y oblige, et si l'ordre
    //s'inversait TOUS les patterns se chargeraient avec les octets de trig permutes :
    //un pattern sur deux jouerait autre chose que ce qu'il contient. Deux lignes
    //separees coutent zero octet et suppriment le risque.
    byte instLow  = Wire.read();
    byte instHigh = Wire.read();
    pattern[!ptrnBuffer].inst[i] = ((unsigned int)instHigh << 8) | instLow;
    pattern[!ptrnBuffer].length[i]  = Wire.read();
    pattern[!ptrnBuffer].scale[i]  = Wire.read();
    pattern[!ptrnBuffer].shuffle[i]  = Wire.read();
    pattern[!ptrnBuffer].dir[i]  = Wire.read();
    MIDI.read();// to improve slave sync response
  }
  //96 bytes requested
  //SETUP-----------------------------------------------
  pattern[!ptrnBuffer].globalLength = Wire.read();
  pattern[!ptrnBuffer].globalScale = Wire.read();
  pattern[!ptrnBuffer].globalShuffle = Wire.read();
  pattern[!ptrnBuffer].globalDir = Wire.read();
  pattern[!ptrnBuffer].groupPos = Wire.read();
  pattern[!ptrnBuffer].groupLength = Wire.read();
  pattern[!ptrnBuffer].globalAcc = Wire.read();
  //La borne n'est pas du zele : les patterns enregistres AVANT que cet octet n'existe
  //portent ici ce que l'initialisation d'usine y avait laisse - zero - et une puce jamais
  //initialisee y porte 0xFF. Au-dela du maximum, on retombe sur une note, ce que ces
  //patterns font de toute facon aujourd'hui. Meme garde qu'a la saisie, cf key.ino.
  pattern[!ptrnBuffer].extLength = Wire.read();
  if (pattern[!ptrnBuffer].extLength > MAX_EXT_INST_NOTE) pattern[!ptrnBuffer].extLength = 0;
  //Le marqueur des conditions de trig, cf SavePattern(). La requete demande deja une page
  //entiere, donc cet octet-la etait dans le tampon depuis toujours.
  boolean condSaved = (Wire.read() == COND_MAGIC);
  boolean rtgSaved = (Wire.read() == RTG_MAGIC);
  boolean utimSaved = (Wire.read() == UTIM_MAGIC);
  //105 bytes requested

    //VELOCITY-----------------------------------------------
  for(int nbrPage = 0; nbrPage < 2; nbrPage++){
    adress = (unsigned long)(PTRN_OFFSET + (patternNbr * PTRN_SIZE) + (MAX_PAGE_SIZE * nbrPage) + PTRN_SETUP_OFFSET);
    WireBeginTX(adress);
    Wire.endTransmission();
    WireRequestFrom(adress, MAX_PAGE_SIZE);
    for (byte i = 0; i < 8; i++){//loop as many instrument for a page
      for (byte j = 0; j < NBR_STEP; j++){
        pattern[!ptrnBuffer].velocity[i + (nbrPage * 8)][j] = (Wire.read() & 0xFF);
      }
      MIDI.read();// to improve slave sync response
    }
  }

  //======================================NOT USED YET=============================================
  //======================================================================================================

  //CONDITIONS DE TRIG-------------------------------------
  //Relues seulement si le pattern a ete enregistre avec. Sinon on remet le champ a zero :
  //il porte encore celles du pattern precedemment charge dans ce tampon, et les laisser
  //serait le "trou au bout de la pile" une fois de plus - une donnee qui traine et qu'on
  //prend pour la sienne.
  if (condSaved){
    for(int nbrPage = 0; nbrPage < 2; nbrPage++){
      adress = (unsigned long)(PTRN_OFFSET + (patternNbr * PTRN_SIZE) + (MAX_PAGE_SIZE * nbrPage) + PTRN_VEL_OFFSET);
      WireBeginTX(adress);
      Wire.endTransmission();
      WireRequestFrom(adress, MAX_PAGE_SIZE);
      for (byte i = 0; i < 8; i++){
        for (byte j = 0; j < NBR_STEP; j++){
          pattern[!ptrnBuffer].probality[i + (nbrPage * 8)][j] = (Wire.read() & 0xFF);
        }
        MIDI.read();// to improve slave sync response
      }
    }
  }
  else {
    for (byte i = 0; i < NBR_INST; i++){
      for (byte j = 0; j < NBR_STEP; j++){
        pattern[!ptrnBuffer].probality[i][j] = 0;
      }
    }
  }

  //ROULEMENT (RETRIG)-------------------------------------
  //Meme regle et meme raison que les conditions juste au-dessus : sans marqueur, la zone
  //n'est pas relue et le champ part a zero. Elle n'a jamais ete ecrite avant aujourd'hui,
  //donc une puce vierge y rend 0xFF partout - soit un index hors table sur les seize pas
  //de toutes les voix. RetrigMask() le rejetterait, mais l'ecran afficherait des taux
  //venus de nulle part, et on chercherait pourquoi.
  //ATTENTION a PTRN_PROB_OFFSET : cf le commentaire de SavePattern(), dans ce fichier
  //chaque _OFFSET designe le debut de la zone SUIVANTE.
  if (rtgSaved){
    for(int nbrPage = 0; nbrPage < 2; nbrPage++){
      adress = (unsigned long)(PTRN_OFFSET + (patternNbr * PTRN_SIZE) + (MAX_PAGE_SIZE * nbrPage) + PTRN_PROB_OFFSET);
      WireBeginTX(adress);
      Wire.endTransmission();
      WireRequestFrom(adress, MAX_PAGE_SIZE);
      for (byte i = 0; i < 8; i++){
        for (byte j = 0; j < NBR_STEP; j++){
          pattern[!ptrnBuffer].retrig[i + (nbrPage * 8)][j] = (Wire.read() & 0xFF);
        }
        MIDI.read();// to improve slave sync response
      }
    }
  }
  else {
    for (byte i = 0; i < NBR_INST; i++){
      for (byte j = 0; j < NBR_STEP; j++){
        pattern[!ptrnBuffer].retrig[i][j] = 0;
      }
    }
  }

  //MICRO-TIMING-------------------------------------------
  //Meme regle et meme raison que les deux champs au-dessus. Sans marqueur, zone non
  //relue et champ remis a zero : un pattern enregistre avant aujourd'hui se charge
  //exactement comme avant, et une puce vierge ne rend pas 0xFF sur les seize pas de
  //toutes les voix - ce qui vaudrait un retard sur chaque pas du pattern.
  if (utimSaved){
    for(int nbrPage = 0; nbrPage < 2; nbrPage++){
      adress = (unsigned long)(PTRN_OFFSET + (patternNbr * PTRN_SIZE) + (MAX_PAGE_SIZE * nbrPage) + PTRN_uTIMING_OFFSET);
      WireBeginTX(adress);
      Wire.endTransmission();
      WireRequestFrom(adress, MAX_PAGE_SIZE);
      for (byte i = 0; i < 8; i++){
        for (byte j = 0; j < NBR_STEP; j++){
          pattern[!ptrnBuffer].uTiming[i + (nbrPage * 8)][j] = (Wire.read() & 0xFF);
        }
        MIDI.read();// to improve slave sync response
      }
    }
  }
  else {
    for (byte i = 0; i < NBR_INST; i++){
      for (byte j = 0; j < NBR_STEP; j++){
        pattern[!ptrnBuffer].uTiming[i][j] = 0;
      }
    }
  }

  //EXT INST-----------------------------------------------
  adress = (unsigned long)(PTRN_OFFSET + (patternNbr * PTRN_SIZE) + PTRN_RETRIG_OFFSET);
  WireBeginTX(adress);
  Wire.endTransmission();
  WireRequestFrom(adress, MAX_PAGE_SIZE);

  for (byte j = 0; j < MAX_PAGE_SIZE; j++){
    pattern[!ptrnBuffer].extNote[j] = Wire.read();
  }
  MIDI.read();// to improve slave sync response
}

//Track save
void SaveTrack(byte trackNbr)
{
  //Last two bytes of the track corresponding to the length 1 to 999
  //Have to be 1020 and 1021 in the array index !!
  byte lowbyte =  (byte)(track[trkBuffer].length & 0xFF);
  byte highbyte = (byte)((track[trkBuffer].length >> 8) & 0xFF);
  track[trkBuffer].patternNbr[1020] = lowbyte;
  track[trkBuffer].patternNbr[1021] = highbyte;

  unsigned long adress;
  //Need 8 pages to save 1024 bytes track size
  for(int nbrPage = 0; nbrPage < 8 ; nbrPage++){
    adress = (unsigned long)(TRACK_OFFSET + (trackNbr * TRACK_SIZE) + (MAX_PAGE_SIZE * nbrPage));
    WireBeginTX(adress);
    MIDI.read();// to improve slave sync response
    for (byte i = 0; i < MAX_PAGE_SIZE; i++){//loop as many instrument for a page
      Wire.write((byte)(track[trkBuffer].patternNbr[i + (MAX_PAGE_SIZE * nbrPage)] & 0xFF)); 
    }
    WireEndTX();//end of 128 bytes transfer
  }

}

//Load track
void LoadTrack(byte trackNbr)
{
  unsigned long adress;
  //Need 8 pages to load 1024 bytes track size
  for(int nbrPage = 0; nbrPage < 8 ; nbrPage++){
    adress = (unsigned long)(TRACK_OFFSET + (trackNbr * TRACK_SIZE) + (MAX_PAGE_SIZE * nbrPage));
    WireBeginTX(adress);
    Wire.endTransmission();
    WireRequestFrom(adress, MAX_PAGE_SIZE);
    for (byte i = 0; i < MAX_PAGE_SIZE; i++){//loop as many instrument for a page
      track[trkBuffer].patternNbr[i + (MAX_PAGE_SIZE * nbrPage)] = (Wire.read() & 0xFF);
    }
    MIDI.read();// to improve slave sync response
  }
  byte lowbyte = track[trkBuffer].patternNbr[1020];
  byte highbyte = track[trkBuffer].patternNbr[1021];
  track[trkBuffer].length =  (unsigned int)(lowbyte | (highbyte << 8));
  //LoadSeqSetup() borne chacun de ses cinq champs ; celui-ci ne bornait rien. Sur une
  //zone jamais ecrite, les deux octets valent 0xFF et la longueur sort a 65535, alors que
  //les positions utiles s'arretent a 999 et que patternNbr[] ne compte que 1024 entrees.
  //Seq.ino laisse avancer trk.pos tant qu'il est <= length : la lecture partait donc
  //au-dela du tableau, a prendre de la RAM quelconque pour des numeros de pattern.
  //Zero plutot qu'un ecretage a 999 : une longueur impossible veut dire piste vide, pas
  //piste de 999 pas de n'importe quoi.
  if (track[trkBuffer].length > MAX_TRACK_POS) track[trkBuffer].length = 0;

}

//Save Setup
void SaveSeqSetup()
{
  unsigned long adress = (unsigned long)(OFFSET_SETUP);
  WireBeginTX(adress);
  Wire.write((byte)(seq.sync)); 
  Wire.write((byte)(seq.defaultBpm));
  Wire.write((byte)(seq.TXchannel));
  Wire.write((byte)(seq.RXchannel));
  Wire.write((byte)(seq.ptrnChangeSync));
  WireEndTX();//end page transmission
}

//Load Setup
void LoadSeqSetup()
{
  unsigned long adress = (unsigned long)(OFFSET_SETUP);
  WireBeginTX(adress); 
  Wire.endTransmission();
  WireRequestFrom(adress, SETUP_SIZE);
  seq.sync = (Wire.read() & 0xFF);
  seq.sync = constrain(seq.sync, 0, 1);
  seq.defaultBpm = (Wire.read() & 0xFF);
  seq.defaultBpm = constrain(seq.defaultBpm, MIN_BPM, MAX_BPM);
  seq.TXchannel = (Wire.read() & 0xFF);
  seq.TXchannel = constrain(seq.TXchannel, 1 ,16);
  seq.RXchannel = (Wire.read() & 0xFF);
  seq.RXchannel = constrain(seq.RXchannel, 1 ,16);
  seq.ptrnChangeSync = (Wire.read() & 0xFF);
  seq.ptrnChangeSync = constrain(seq.ptrnChangeSync, 0, 1);
}

//Save pattern group
void SavePatternGroup(byte firstPattern, byte length)
{
  for (int a = 0; a <= length ; a++){
    //Never step outside the pattern area : past MAX_PTRN the address lands in the
    //tracks. firstPattern and length can come from EEPROM content.
    if (firstPattern + a >= MAX_PTRN) break;
    unsigned long adress = (unsigned long)(PTRN_OFFSET + ((firstPattern + a) * PTRN_SIZE) + OFFSET_GROUP);
    WireBeginTX(adress);
    Wire.write((byte)(a)); //pattern[ptrnBuffer].groupPos
    Wire.write((byte)(length));//pattern[ptrnBuffer].groupLength
    WireEndTX();//end page transmission
  }
}

//Clear pattern group
void ClearPatternGroup(byte firstPattern, byte length)
{
  for (int a = 0; a <= length ; a++){
    //Same guard as SavePatternGroup : this one is called with values read back from
    //the EEPROM, which the hardcoded chip address bug could have left inconsistent.
    if (firstPattern + a >= MAX_PTRN) break;
    unsigned long adress = (unsigned long)(PTRN_OFFSET + ((firstPattern + a) * PTRN_SIZE) + OFFSET_GROUP);
    WireBeginTX(adress);
    Wire.write((byte)(0)); //pattern[ptrnBuffer].groupPos
    Wire.write((byte)(0));//pattern[ptrnBuffer].groupLength
    WireEndTX();//end page transmission
  }
}

//Load pattern group type => POSITION = 0 or LENGTH = 1
byte LoadPatternGroup(byte patternNum, byte type)
{
  unsigned long adress = (unsigned long)(PTRN_OFFSET + (patternNum * PTRN_SIZE) + OFFSET_GROUP + type);
  WireBeginTX(adress);
  Wire.endTransmission();
  //The chip address carries bits 16 and up, so it has to follow the address. Reading
  //from a hardcoded HRDW_ADDRESS returned another chip's data past pattern 51, and the
  //caller in Seq.ino feeds that straight into ClearPatternGroup().
  WireRequestFrom(adress, (byte)1);
  byte data = (Wire.read() & 0xFF);
  return data;
}


//init pattern in the EEprom
void InitEEprom()
{
  unsigned long adress;

  //Pattern init
  for (byte nbrPattern = 0; nbrPattern < MAX_PTRN ; nbrPattern++)
  {
    unsigned long adress = (unsigned long)(PTRN_OFFSET + nbrPattern * PTRN_SIZE);
    WireBeginTX(adress); 
    //TRIG-----------------------------------------------
    for (byte i = 0; i < NBR_INST; i++){  
      Wire.write((byte)(0));//(pattern[ptrnBuffer].inst[i] & 0xFF);
      Wire.write((byte)(0));//(pattern[ptrnBuffer].inst[i] >> 8) & 0xFF;
      Wire.write((byte)(DEFAULT_LEN - 1));//(pattern[ptrnBuffer].length[i]));
      Wire.write((byte)(DEFAULT_SCALE));//(pattern[ptrnBuffer].scale[i]));
      Wire.write((byte)(DEFAULT_SHUF));//(pattern[ptrnBuffer].shuffle[i]));
      Wire.write((byte)(0));//(pattern[ptrnBuffer].dir[i]));
    }//96 bytes

    //GLOBAL SETUP-----------------------------------------------
    Wire.write((byte)(DEFAULT_LEN - 1));//(pattern[ptrnBuffer].globalLength));
    Wire.write((byte)(DEFAULT_SCALE));//(pattern[ptrnBuffer].globalScale));
    Wire.write((byte)(DEFAULT_SHUF));//(pattern[ptrnBuffer].globalShuffle));
    Wire.write((byte)(0));//(pattern[ptrnBuffer].globalDir));
    Wire.write((byte)(0));//(pattern[ptrnBuffer].groupPos));
    Wire.write((byte)(0));//(pattern[ptrnBuffer].groupLength));
    //globalAcc et extLength etaient absents d'ici alors que SavePattern() et
    //LoadPattern() les comptent : les zeros de remplissage en tenaient lieu, ce qui
    //donnait le bon resultat par accident. Les trois fonctions decrivent desormais la
    //meme page, champ par champ.
    Wire.write((byte)(0));//(pattern[ptrnBuffer].globalAcc));
    Wire.write((byte)(0));//(pattern[ptrnBuffer].extLength));
    //104 bytes
    for(int a =0; a < 24; a++){
      Wire.write( (byte)(0));//unused parameters
    }//128 bytes FIRST PAGE

    WireEndTX();//end page transmission

    //VELOCITY-----------------------------------------------
    for(int nbrPage = 0; nbrPage < 2; nbrPage++){
      adress = (unsigned long)(PTRN_OFFSET + (nbrPattern * PTRN_SIZE) + (MAX_PAGE_SIZE * nbrPage) + PTRN_SETUP_OFFSET);
      WireBeginTX(adress);
      for (byte i = 0; i < 8; i++){//loop as many instrument for a page
        for (byte j = 0; j < NBR_STEP; j++){
          Wire.write((byte)(40));//(pattern[ptrnBuffer].velocity[i + 4*nbrPage][j] & 0xFF)); 
        }
      }
      WireEndTX();//end of 64 bytes transfer
    }//2 * 128 bytes = 256 bytes

    //PROBALITY-----------------------------------------------
    for(int nbrPage = 0; nbrPage < 2; nbrPage++){
      adress = (unsigned long)(PTRN_OFFSET + (nbrPattern * PTRN_SIZE) + (MAX_PAGE_SIZE * nbrPage) + PTRN_VEL_OFFSET);
      WireBeginTX(adress);
      for (byte i = 0; i < 8; i++){//loop as many instrument for a page
        for (byte j = 0; j < NBR_STEP; j++){
          Wire.write((byte)(0));//(pattern[ptrnBuffer].probality[i + 4*nbrPage][j] & 0xFF)); 
        }
      }
      WireEndTX();//end of 64 bytes transfer
    }//2 * 128 bytes = 256 bytes

    //uTIMING-----------------------------------------------
    for(int nbrPage = 0; nbrPage < 2; nbrPage++){
      adress = (unsigned long)(PTRN_OFFSET + (nbrPattern * PTRN_SIZE) + (MAX_PAGE_SIZE * nbrPage) + PTRN_PROB_OFFSET);
      WireBeginTX(adress);
      for (byte i = 0; i < 8; i++){//loop as many instrument for a page
        for (byte j = 0; j < NBR_STEP; j++){
          Wire.write((byte)(0));//(pattern[ptrnBuffer].uTiming[i + 4*nbrPage][j] & 0xFF)); 
        }
      }
      WireEndTX();//end of 64 bytes transfer
    }//2 * 128 bytes = 256 bytes

    //RETRIG-----------------------------------------------
    for(int nbrPage = 0; nbrPage < 2; nbrPage++){
      adress = (unsigned long)(PTRN_OFFSET + (nbrPattern * PTRN_SIZE) + (MAX_PAGE_SIZE * nbrPage) + PTRN_uTIMING_OFFSET);
      WireBeginTX(adress);
      for (byte i = 0; i < 8; i++){//loop as many instrument for a page
        for (byte j = 0; j < NBR_STEP; j++){
          Wire.write((byte)(0));//(pattern[ptrnBuffer].uTiming[i + 4*nbrPage][j] & 0xFF)); 
        }
      }
      WireEndTX();//end of 64 bytes transfer
    }

    //EXT INST-----------------------------------------------
    adress = (unsigned long)(PTRN_OFFSET + (nbrPattern * PTRN_SIZE) + PTRN_RETRIG_OFFSET);
    WireBeginTX(adress);
    for (byte j = 0; j < MAX_PAGE_SIZE; j++){
      Wire.write((byte)(0));
    }//128 bytes -> fisrt page
    WireEndTX();//end page transmission

    //Show init statut on steps Leds
    static unsigned int tempInitLeds;
    tempInitLeds |= bitSet(tempInitLeds, (nbrPattern/8));
    SetDoutLed(tempInitLeds, 0, 0);
  }//END PATTERN INIT

  //TRACK INST-----------------------------------------------

  for (byte nbrTrack = 0; nbrTrack < MAX_TRACK ; nbrTrack++)
  {   
    for (byte nbrPage = 0; nbrPage < TRACK_SIZE/MAX_PAGE_SIZE; nbrPage++){
      adress = (unsigned long)(TRACK_OFFSET + (nbrTrack * TRACK_SIZE) + (nbrPage * MAX_PAGE_SIZE));
      WireBeginTX(adress);
      for (byte j = 0; j < MAX_PAGE_SIZE; j++){
        Wire.write((byte)(0));
      }//128 bytes -> fisrt page
      WireEndTX();//end page transmission
    }
    //Show init statut on steps Leds
    static unsigned int tempInitLeds;
    tempInitLeds |= bitSet(tempInitLeds, nbrTrack);
    SetDoutLed(tempInitLeds, 0, 0);
  }//END TRACK INIT

  //Sequencer Setup initialize----------------------------------
  adress = (unsigned long)(OFFSET_SETUP);
  WireBeginTX(adress);
  Wire.write((byte)(MASTER));//seq.sync)); 
  Wire.write((byte)(DEFAULT_BPM));//seq.defaultBpm));
  Wire.write((byte)(1));//seq.TXchannel));
  Wire.write((byte)(1));//seq.RXchannel));
  WireEndTX();//end page transmission

  //Set the 3 last byte of the first EEprom to 0, 1, 2 value to NEVER init EEPROM again
  WireBeginTX(262137);
  Wire.write((byte)(0));//seq.defaultBpm));
  Wire.write((byte)(1));//seq.TXchannel));
  Wire.write((byte)(2));//seq.RXchannel));
  WireEndTX();//end page transmission


}

void DebugEEprom(){
  Serial.println(OFFSET_SETUP);
}

//==================================================================================================
//Wire begin Function to set address on the I2C bus
//==================================================================================================
//Ouvrir une lecture a une adresse EEPROM----------------------------------------------
//Le pendant de WireBeginTX() : l'adresse d'un octet dans les EEPROM tient sur 18 bits, et
//les bits 16 et au-dela designent la PUCE - HRDW_ADDRESS + (adress >> 16) - tandis que
//les 16 bits bas sont l'adresse interne envoyee par WireBeginTX().
//Ce calcul etait recopie a NEUF endroits, et deux l'avaient oublie : une lecture partait
//sur un HRDW_ADDRESS fige et rendait les donnees d'une autre puce au-dela du pattern 51.
//L'un des deux alimentait ClearPatternGroup(), donc une lecture fausse effacait des
//chaines ailleurs. Meme raisonnement que les douze copies du recalage piste-pattern :
//une regle ecrite une fois ne peut plus etre oubliee a un site.
void WireRequestFrom(unsigned long address, byte count)
{
  byte hardwareAddress = HRDW_ADDRESS + (address >> 16);
  Wire.requestFrom(hardwareAddress, count);
}

void WireBeginTX(unsigned long address)
{
  byte hardwareAddress;
  hardwareAddress = HRDW_ADDRESS + (address >> 16); 
  wireDevice = hardwareAddress;//c'est elle que WireWaitReady() interrogera
  Wire.beginTransmission(hardwareAddress);
  Wire.write((byte)(address >> 8));
  Wire.write((byte)(address & 0xFF));
}
























































