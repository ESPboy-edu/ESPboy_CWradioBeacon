/*
ESPboy CW Radio Beacon 
using Si5351 chipset
for www.ESPboy.com project
by RomanS espboy.edu@gmail.com

29.05.2021

COMMANDS:
-h  -?  [help]
-i [info]
-f nn [set frequence]
-e nn [set speed]
-t nn [sound tone] 
-l [led on/off]
-r [relay on/off]
-n [clear text]
-p [print text]
-s [stop send]
-b [begin send]
-c [continue send]
-o [send looping]

*/


#include "lib/ESPboyInit.h"
#include "lib/ESPboyInit.cpp"
#include "lib/ESPboyTerminalGUI.h"
#include "lib/ESPboyTerminalGUI.cpp"

#include <si5351.h>

#define RF_FREQ 145000 // default RF output frequency Hz
#define CW_SPEED 50   // default CW dot length in ms
#define RELAY_PIN D8   // default relay connected pin
#define TONE_FREQ 200  // default play tone freq
#define DEFAULT_MESS F(" testing ESPboy radio beacon module Si5351 ") //default message
//#define DEFAULT_MESS F(" testing") //default message

ESPboyInit myESPboy;
ESPboyTerminalGUI terminalGUIobj(&myESPboy.tft, &myESPboy.mcp);
Si5351 si5351(0x40);

static uint32_t rfFreq = RF_FREQ;
static uint16_t toneFreq = TONE_FREQ;
static uint32_t cwSpeed = CW_SPEED;
static uint16_t cwPosition = 0;
static bool flagLed = true, flagTone = true, flagRelay = false, flagSending = false, flagLoop = true, flagPlay = true; 
static String cwString = "";

//help
const char *PROGMEM hlp[]={
"COMMANDS:",
"-h or -?  help",
"-i  info",
"-f nn  set freq.",
"-e nn  set speed",
"-t nn  sound tone",
"-l  led on/off",
"-n  clear text",
"-p  print text",
"-s  stop send",
"-b  begin send",
"-c  continue send",
"-o  send looping",
"",
};


//Morse code
const uint8_t cwSymbTab[][7] = {
  {1, 2, 3},             // 0  A
  {2, 1, 1, 3},          // 1  B
  {2, 1, 2, 1, 3},       // 2  C
  {2, 1, 1, 3},          // 3  D
  {1, 3},                // 4  E
  {1, 1, 2, 1, 3},       // 5  F
  {2, 2, 1, 3},          // 6  G
  {1, 1, 1, 1, 3},       // 7  H
  {1, 1, 3},             // 8  I
  {1, 2, 2, 2, 3},       // 9  J
  {2, 1, 2, 3},          // 10 K
  {1, 2, 1, 1, 3},       // 11 L
  {2, 2, 3},             // 12 M
  {2, 1, 3},             // 13 N
  {2, 2, 2, 3},          // 14 O
  {1, 2, 2, 1, 3},       // 15 P
  {2, 2, 1, 2, 3},       // 16 Q
  {1, 2, 1, 3},          // 17 R
  {1, 1, 1, 3},          // 18 S
  {2, 3},                // 19 T
  {1, 1, 2, 3},          // 20 U
  {1, 1, 1, 2, 3},       // 21 V
  {1, 2, 2, 3},          // 22 W
  {2, 1, 1, 2, 3},       // 23 X
  {2, 1, 2, 2, 3},       // 24 Y
  {2, 2, 1, 1, 3},       // 25 Z
  {2, 2, 2, 2, 2, 3},    // 26 0
  {1, 2, 2, 2, 2, 3},    // 27 1
  {1, 1, 2, 2, 2, 3},    // 28 2
  {1, 1, 1, 2, 2, 3},    // 29 3
  {1, 1, 1, 1, 2, 3},    // 30 4
  {1, 1, 1, 1, 1, 3},    // 31 5
  {2, 1, 1, 1, 1, 3},    // 32 6
  {2, 2, 1, 1, 1, 3},    // 33 7
  {2, 2, 2, 1, 1, 3},    // 34 8
  {2, 2, 2, 2, 1, 3},    // 35 9
  {1, 2, 2, 1, 2, 1, 3}, // 36 @
  {1, 1, 2, 1, 2},       // 37 end of transmission
  
};


void cwSendSym(uint16_t len){
  si5351.output_enable(SI5351_CLK1, 1);
  if(flagLed)  myESPboy.myLED.setRGB(0, 5, 0); 
  if(flagTone)myESPboy.playTone(toneFreq);
  if(flagRelay) digitalWrite(RELAY_PIN, HIGH);
  
  delay(len);

  if(flagLed)  myESPboy.myLED.setRGB(0, 0, 0); 
  if(flagTone)myESPboy.noPlayTone();
  if(flagRelay) digitalWrite(RELAY_PIN, LOW);
  si5351.output_enable(SI5351_CLK1, 0);  
}



void consolePrint(String strPrint, bool newStr){
  static String toPrint="";

  if(newStr)toPrint="";
  else{
    toPrint+=strPrint;
    terminalGUIobj.printConsole(toPrint, TFT_MAGENTA, 1, 1);
    if(toPrint.length()>=20){
      terminalGUIobj.printConsole("", TFT_MAGENTA, 1, 0);
      toPrint="";
    }
  }
}



void drawHelp(){
  for(uint8_t i=0; i<sizeof(hlp)/sizeof(hlp[0]); i++)
    terminalGUIobj.printConsole(hlp[i], TFT_BLUE, 1, 0);  
}


void printText(){
  String toPrint = cwString;
  toPrint.trim();
  toPrint = "TEXT ["+toPrint+"]";
  terminalGUIobj.printConsole(toPrint, TFT_GREEN, 1, 0);
}


void drawInfo(){
 String toPrint;
  
  toPrint = F("Frq ");
  toPrint += (String)rfFreq;
  toPrint += F(" Spd ");
  toPrint += (String)CW_SPEED;
  terminalGUIobj.printConsole(toPrint, TFT_YELLOW, 1, 0);

  toPrint = F("Led");
  if(flagLed) toPrint+="+ ";
  else toPrint+="- ";
  toPrint += F("Tone");
  if(flagTone) toPrint+="+ ";
  else toPrint+="- ";
  toPrint += F("Relay");
  if(flagRelay) toPrint+="+ ";
  else toPrint+="- ";  
  terminalGUIobj.printConsole(toPrint, TFT_YELLOW, 1, 0);

  toPrint = F("Loop");
  if(flagLoop) toPrint+="+ ";
  else toPrint+="- ";  
  toPrint += F("AutoPlay");
  if(flagPlay) toPrint+="+ ";
  else toPrint+="- ";    
  terminalGUIobj.printConsole(toPrint, TFT_YELLOW, 1, 0);
  
  printText();
}



void triggerLed(){
  flagLed=!flagLed;
  String toPrint = F("LED ");
  if(flagLed) toPrint+="ON";
  else toPrint+="OFF";
  terminalGUIobj.printConsole(toPrint, TFT_WHITE, 1, 0);
}


void triggerTone(){
  flagTone=!flagTone;
  String toPrint = F("Tone ");
  if(flagTone) toPrint+="ON";
  else toPrint+="OFF";
  terminalGUIobj.printConsole(toPrint, TFT_WHITE, 1, 0);
}


void triggerRelay(){
  flagRelay=!flagRelay;
  String toPrint = F("Relay ");
  if(flagRelay) toPrint+="ON";
  else toPrint+="OFF";
  terminalGUIobj.printConsole(toPrint, TFT_WHITE, 1, 0);
}


void triggerLoop(){
  flagLoop=!flagLoop;
  String toPrint = F("Looping ");
  if(flagLoop) toPrint+="ON";
  else toPrint+="OFF";
  terminalGUIobj.printConsole(toPrint, TFT_WHITE, 1, 0);
}


void setFreq(uint32_t freq){
  si5351.set_freq(freq * 100000ULL, SI5351_CLK1);
  String toPrint = F("New freq: ");
  toPrint += (String)freq;
  terminalGUIobj.printConsole(toPrint, TFT_WHITE, 1, 0);
}


void setSpeed(uint32_t spd){
  cwSpeed = spd;
  String toPrint = F("New speed: ");
  toPrint += (String)spd;
  terminalGUIobj.printConsole(toPrint, TFT_WHITE, 1, 0);
}


void setTone(uint32_t tne){
  toneFreq=tne;
  String toPrint = F("New tone: ");
  toPrint += (String)tne;
  terminalGUIobj.printConsole(toPrint, TFT_WHITE, 1, 0);
}


void clearText(){
  cwString="";
  terminalGUIobj.printConsole(F("Text cleared"), TFT_WHITE, 1, 0);
  cwPosition=0;
  consolePrint("", 1);
}



void stopSend(){
  flagPlay = false;
  terminalGUIobj.printConsole(F("Stopped send"), TFT_WHITE, 1, 0);
}



void beginSend(){
  cwPosition=0;
  flagPlay = true;
  terminalGUIobj.printConsole(F("Begin send"), TFT_WHITE, 1, 0);
  terminalGUIobj.printConsole("", TFT_BLACK, 1, 0);
  consolePrint("", 1);
}



void continueSend(){
  flagPlay = true;
  terminalGUIobj.printConsole(F("Continue send"), TFT_WHITE, 1, 0);
  terminalGUIobj.printConsole("", TFT_BLACK, 1, 0);
}



void sendSWmessageFunction(){
 static char currentCharToSend;
 static uint8_t currentPosInCharToSend=0;
 static bool flagSendingChar=0;
 static uint8_t tabIndex;


 if(flagPlay){
  
  if(!flagSendingChar){
    if(cwPosition < cwString.length()){
      currentCharToSend = cwString[cwPosition];
      cwPosition++;
      if(flagLoop && cwPosition == cwString.length()) {
        cwPosition = 0;
      }
    }
    else
      if(cwPosition == cwString.length())
        return;
        
    if (currentCharToSend == ' '){
      delay(cwSpeed*7);
      return;}

    tabIndex = 255;
    if (currentCharToSend=='@') tabIndex = 36;
    if (currentCharToSend=='#') tabIndex = 37;
    if ((currentCharToSend >= 65) && (currentCharToSend <= 90))  tabIndex = currentCharToSend - 65;  // A - Z
    if ((currentCharToSend >= 97) && (currentCharToSend <= 122)) tabIndex = currentCharToSend - 97;  // a - z
    if ((currentCharToSend >= 48) && (currentCharToSend <= 57))  tabIndex = currentCharToSend - 22;  // 0 - 9

    if (currentCharToSend == 255){
      return;
    }

    consolePrint(" ",0);
    consolePrint((String)currentCharToSend,0);

    flagSendingChar=true;
    currentPosInCharToSend=0;
  }

    
  if (flagSendingChar){
    char cwSym = cwSymbTab[tabIndex][currentPosInCharToSend];
    currentPosInCharToSend++;
    switch(cwSym){
      case 1:
        consolePrint(".",0);
        cwSendSym(CW_SPEED);
        break;
      case 2:
        consolePrint("-",0);
        cwSendSym(CW_SPEED * 3);
        break;
      case 3:
        delay(CW_SPEED*2);
        flagSendingChar=0;
        break;
    }

    delay(CW_SPEED);
  }
 }
}




void setup(){
  Serial.begin(115200);
  myESPboy.begin("CW radio beacon");
  terminalGUIobj.toggleDisplayMode(1);
  pinMode(RELAY_PIN, OUTPUT);

  terminalGUIobj.printConsole(F("Init radio module..."), TFT_WHITE, 1, 0);
  if(!si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0)){
    terminalGUIobj.printConsole(F("FAULT"), TFT_RED, 1, 0);
    while(1)delay(1000);
  }
  else 
     terminalGUIobj.printConsole(F("OK"), TFT_GREEN, 1, 0);

  delay(1500);
  drawHelp();
  delay(1500);
  cwString = DEFAULT_MESS;
  drawInfo();
  
  si5351.update_status();
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA);
  si5351.set_correction(80000, SI5351_PLL_INPUT_XO);
  si5351.set_freq(RF_FREQ * 100000ULL, SI5351_CLK1);
  si5351.output_enable(SI5351_CLK1, 0);

  terminalGUIobj.printConsole("", TFT_GREEN, 1, 0);
}



void callCommand(String cmd){
  switch (cmd[1]){
    case 'h': //help
      drawHelp();
      break;
    case '?': //help
      drawHelp();
      break;
    case 'i': //info
      drawInfo();
      break;
    case 'f': //set freq
      cmd=cmd.substring(3);
      cmd.trim();
      if(atoi(cmd.c_str())!=-1)
        setFreq(atoi(cmd.c_str()));
      break;
    case 'e': //set speed
      cmd=cmd.substring(3);
      cmd.trim();
      if(atoi(cmd.c_str())!=-1)
        setSpeed(atoi(cmd.c_str()));
      break;
    case 't': //sound tone
      cmd=cmd.substring(3);
      cmd.trim();
      if(atoi(cmd.c_str())!=-1)
        setTone(atoi(cmd.c_str()));
      break;
    case 'l': //led on/off
      triggerLed();
      break;
    case 'r': //relay on/off
      triggerRelay();
      break;
    case 'n': //clear text
      clearText();
      break;
    case 'p': //print text
      printText();
      break;
    case 's': //stop send
      stopSend();
      break;
    case 'b': //begin send
      beginSend();
      break;
    case 'c': //continue send
      continueSend();
      break;
    case 'o': //send looping
      triggerLoop();
      break;
    default:
      terminalGUIobj.printConsole(F("Unknown command"), TFT_RED, 1, 0);
      break;
  }
}


void loop(){
  static uint8_t keysState;
  static String typing;

  sendSWmessageFunction();
  keysState = myESPboy.getKeys();

  if (keysState && !(keysState&PAD_LFT) && !(keysState&PAD_RGT) && !(keysState&PAD_ESC)) {
    myESPboy.playTone(200, 100);
    typing = terminalGUIobj.getUserInput();
    while(myESPboy.getKeys())delay(100);
    terminalGUIobj.toggleDisplayMode(1);

    if (typing[0]=='-')
      callCommand(typing);    
    else {
      cwString+=typing;
      terminalGUIobj.printConsole("ADD [" + typing + "]", TFT_GREEN, 1, 0);
      terminalGUIobj.printConsole("", TFT_BLACK, 1, 0);
    }
  }

  if (keysState&PAD_ESC){
    uint32_t delayTimer;
    delayTimer=millis();
    while(myESPboy.getKeys())delay(100);
    if (millis()-delayTimer > 1000) 
      stopSend();
  }
  
  if (keysState&PAD_LFT || keysState&PAD_RGT)
     terminalGUIobj.doScroll();   
  else delay(100);
}
