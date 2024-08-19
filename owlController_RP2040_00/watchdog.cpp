#include "watchdog.h"
#include <Arduino.h>

extern "C" {
	#include "hardware/watchdog.h"
  #include "hardware/structs/watchdog.h"
  #include "hardware/structs/psm.h"
  #include "hardware/regs/vreg_and_chip_reset.h"  
}



io_rw_32 *reset_reason = (io_rw_32 *) (VREG_AND_CHIP_RESET_BASE + VREG_AND_CHIP_RESET_CHIP_RESET_OFFSET);


void Watchdog::begin(){
  powerOnResetFlag = (*reset_reason & VREG_AND_CHIP_RESET_CHIP_RESET_HAD_POR_BITS);  // POR: Last reset was from the power-on reset or brown-out detection    
  runFlag = (*reset_reason & VREG_AND_CHIP_RESET_CHIP_RESET_HAD_RUN_BITS); // RUN: Last reset was from the RUN pin
}

void Watchdog::dumpInfo(){
  // check if code was rebooted by watchdog or clean-boot    
  if (causedReboot()) {
    // rebooted by watchdog        
    Serial.println("MCU restarted by watchdog");
  } else {
      // clean boot
  }
  Serial.print("powerOnResetFlag: ");
  Serial.println(powerOnResetFlag);
  Serial.print("runFlag: ");
  Serial.println(runFlag);
}

void Watchdog::enable(){
  //Serial.println("watchdog enabled");
  watchdog_enable(5, 1); // second arg is pause on debug which means the watchdog will pause when stepping through code      
  //startShortTimeout();
  startLongTimeout();
}

void Watchdog::disable(){
  //Serial.println("watchdog disabled");
  watchdogIsDisabled = true;
  hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
  // Reset everything apart from ROSC and XOSC
  hw_set_bits(&psm_hw->wdsel, PSM_WDSEL_BITS & ~(PSM_WDSEL_ROSC_BITS | PSM_WDSEL_XOSC_BITS));
}


void Watchdog::setTimeout(int delay_ms){
  if (watchdogIsDisabled) return;   
  //Serial.print("watchdog timeout: ");
  //Serial.println(delay_ms);
  watchdogTimeout = delay_ms;
  watchdog_load_value = delay_ms * 1000 * 2;
  if (watchdog_load_value > 0xffffffu) watchdog_load_value = 0xffffffu;
  watchdog_hw->load = watchdog_load_value;
}

void Watchdog::resetTimeout(){
  if (doPauseWatchdogUpdate) return;
  if (watchdogIsDisabled) return; 
  watchdog_hw->load = watchdog_load_value;
}

// start watchdog
void Watchdog::startShortTimeout(){
  //Serial.println("starting watchdog short timeout");
  // set watchdog to 5 ms      
  setTimeout(5);
}

void Watchdog::startLongTimeout(){
  //Serial.println("starting watchdog long timeout");  
  setTimeout(5000);
}

void Watchdog::pauseResetTimeout(){
  //Serial.println("pausing watchdog timeout reset");
  doPauseWatchdogUpdate = true; 
}


bool Watchdog::causedReboot(){
  return (watchdog_caused_reboot());
}


void Watchdog::dumpMem(){
	Serial.print("watchdog mem: ");
  Serial.print(watchdog_hw->scratch[5], HEX);
  Serial.print(",");
  Serial.println(watchdog_hw->scratch[6], HEX);
}


void Watchdog::resetMCU(){
  Serial.println("resetting MCU...");
  delay(1000);
  //reset_usb_boot(1 << digitalPinToPinName(LED_BUILTIN), 0);

	hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
	/*if (to_bootloader) {
		watchdog_hw->scratch[5] = BOOTLOADER_ENTRY_MAGIC;
		watchdog_hw->scratch[6] = ~BOOTLOADER_ENTRY_MAGIC;
	} else {
		watchdog_hw->scratch[5] = 0;
		watchdog_hw->scratch[6] = 0;
	}*/
	watchdog_reboot(0, 0, 10);
	while (1) {
		tight_loop_contents();
		asm("");
	}
}


