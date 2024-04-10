#include "comparser.h"

#define DEBUG 1 
#define CONSOLE Serial    // where to output debug stuff

//#define ENABLE_CRC_CHECK  1  // CRC checks can be disabled for testing


ComParser::ComParser(Stream *aStream){
  stream = aStream;
  requestAvail = false;
  cmdIdx = 0;
} 

// pre-process request
bool ComParser::preProcessCmd(bool checkCrc){
  if (cmd.length() < 4) return false;
  byte expectedCrc = 0;
  int idx = cmd.lastIndexOf(',');
  if (idx < 1){
#ifdef ENABLE_CRC_CHECK
    if (checkCrc){
  #ifdef DEBUG
      CONSOLE.println("CRC ERROR");
  #endif      
      return false;
    }
#endif    
  } else {
    for (int i=0; i < idx; i++) expectedCrc += cmd[i];  
    String s = cmd.substring(idx+1, idx+5);
    int crc = strtol(s.c_str(), NULL, 16);  
    if (expectedCrc != crc){
#ifdef ENABLE_CRC_CHECK      
      if (checkCrc){
  #ifdef DEBUG
        CONSOLE.print("CRC ERROR");
        CONSOLE.print(crc,HEX);
        CONSOLE.print(",");
        CONSOLE.print(expectedCrc,HEX);
        CONSOLE.println();
  #endif
        return false;  
      }
#endif      
    } else {
      // remove CRC
      cmd = cmd.substring(0, idx);
      //CONSOLE.println(cmd);
    }    
  }     
  if (cmd[0] != 'A') return false;
  if (cmd[1] != 'T') return false;
  if (cmd[2] != '+') return false;
  
  cmdIdx = 0; 
  lastCommaIdx = -1;
  return true;
}



bool ComParser::cmdAvailable(String &s){
  if (requestAvail) { // clear last request
    cmd = "";
    cmdIdx = 0;
    lastCommaIdx = -1;
    requestAvail = false;
  }
  if (!stream->available()) return false;
  char ch;
  unsigned long timeout = millis() + 10;       
  //battery.resetIdle();  
  while ( (stream->available()) && (millis() < timeout) ){               
    timeout = millis() + 10;       
    ch = stream->read();          
    if ((ch == '\r') || (ch == '\n')) {        
#ifdef DEBUG        
      CONSOLE.println(cmd);
#endif            
      requestAvail = true;
      if (!preProcessCmd(true)) return false;           
      s = cmd;
      return true;
    } else if (cmd.length() < 500){
      cmd += ch;
    }
  }
  return false;
}


// answer via stream with CRC
void ComParser::sendAnswer(String s){
  byte crc = 0;
  for (int i=0; i < s.length(); i++) crc += s[i];
  s += F(",0x");
  if (crc <= 0xF) s += F("0");
  s += String(crc, HEX);  
  s += F("\r\n");             
  //CONSOLE.print(s);  
  stream->print(s);
}


bool ComParser::nextIntToken(int &value){
  String s;
  if (!nextStringToken(s)) return false;
  value = s.toInt();
  return true;  
}

bool ComParser::nextFloatToken(float &value){
  String s;
  if (!nextStringToken(s)) return false;
  value = s.toFloat();
  return true;  
}

bool ComParser::nextStringToken(String &value){
  if (cmdIdx >= cmd.length()) return false;
  while (true){
    char ch = cmd[cmdIdx];
    if ((ch == ',') || (cmdIdx == cmd.length()-1)){
      value = cmd.substring(lastCommaIdx+1, ch==',' ? cmdIdx : cmdIdx+1);      
#ifdef DEBUG
      CONSOLE.print("value:");
      CONSOLE.println(value);
#endif      
      lastCommaIdx = cmdIdx;       
      cmdIdx++;    
      return true;
    }
    cmdIdx++;    
  }  
  return false;
}
