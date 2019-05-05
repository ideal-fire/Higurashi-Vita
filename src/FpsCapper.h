#ifndef HAPPYFPSCAP
#define HAPPYFPSCAP

#if GBPLAT == GB_VITA
	#include <psp2/kernel/processmgr.h>
#endif

#define MILISECONDSPERFRAME 17

// The milisecodnds at the start of the frame.
unsigned int frameStartMiliseconds;
signed char capEnabled = 1;

void fpsCapStart(){
	#if GBPLAT==GB_LINUX || GBPLAT==GB_WINDOWS
		frameStartMiliseconds = getMilli();
	#endif
}

void fpsCapWait(){
	#if GBPLAT == GB_VITA
		sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
	#else
		if (capEnabled==1){
			unsigned int tempHold;
			tempHold = getMilli();
			//printf("%llu;%llu\n",frameStartMiliseconds,tempHold);
			// LIMIT FPS
			if (tempHold-frameStartMiliseconds<MILISECONDSPERFRAME){
				wait( (MILISECONDSPERFRAME - (tempHold-frameStartMiliseconds)));
			}else{
				//printf("Slowdown %llu\n", tempHold-frameStartMiliseconds);
				//printf("Slowdown %d\n",MILISECONDSPERFRAME - (tempHold-frameStartMiliseconds));
				//printf("Slowdown: %d\n",tempHold-frameStartMiliseconds-MILISECONDSPERFRAME);
			}
		}
	#endif
}

#endif