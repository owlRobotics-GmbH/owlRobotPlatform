#ifndef WATCHDOG_H
#define WATCHDOG_H


// watchdog for RP2040

class Watchdog {
    public:      
        bool powerOnResetFlag = false;
        bool runFlag = false;        
        void begin();
        void dumpInfo();
        void enable();
        void disable();
        void setTimeout(int delay_ms);
        void resetTimeout();
        void pauseResetTimeout();        
        void startShortTimeout();
        void startLongTimeout();
        bool causedReboot();
        void dumpMem();
        void resetMCU();
    protected:
        bool doPauseWatchdogUpdate = false;
        int watchdogTimeout = 0;        
        bool watchdogIsDisabled = false; 
        unsigned int watchdog_load_value = 0;
   
};





#endif