
// AT-command parser for Arduino streams (serial etc.)

// example command:  AT+V,0x16

#ifndef COMPARSER_H
#define COMPARSER_H

#include <Arduino.h>

class ComParser {
  public:   
    ComParser(Stream *aStream); 
    bool cmdAvailable(String &s);
    bool nextStringToken(String &value);
    bool nextIntToken(int &value);
    bool nextFloatToken(float &value);
    void sendAnswer(String s);
  protected:
    bool requestAvail;
    Stream *stream;
    String cmd;
    int cmdIdx;
    int lastCommaIdx;
    bool preProcessCmd(bool checkCrc);
};


#endif
