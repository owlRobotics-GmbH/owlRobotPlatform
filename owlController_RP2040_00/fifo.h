#ifndef FIFO_H_
#define FIFO_H_


#include <Arduino.h>


// example usage:  
//   FiFo<string, 1000> myFifo;
//   FiFo<struct mystruct, 1000> myFifo;
//   FiFo<char, 1000> myFifo;

// TData: The data type of the fifo.
// len: The length of the fifo.
 
template<typename TData, uint16_t TLen>
class FiFo
{
  public:
    unsigned long frameCounter = 0;
    bool available(){
      	return (fifoStart != fifoEnd); 
    }  
    bool read(TData &frame){
      if (!available ()){
        return false;
      }
  	  frame = fifo[fifoStart];
	    if (fifoStart == TLen-1) fifoStart = 0; 
	      else fifoStart++;
      return true;
    }
    bool write(TData frame){
        int nextFifoEnd = fifoEnd;
    	if (nextFifoEnd == TLen-1) nextFifoEnd = 0; 
	      else nextFifoEnd++;
	
      if (nextFifoEnd != fifoStart){
        // no fifoRx overflow 
        fifo[fifoEnd] = frame;
        fifoEnd = nextFifoEnd;
        frameCounter++;
        return true;
      } else {
        // fifoRx overflow
        fifoEnd = fifoStart;
        return false;
      }  
    }
  protected:
    int fifoStart = 0;
    int fifoEnd = 0;
    TData fifo[TLen];
};


#endif /* FIFO_H */
