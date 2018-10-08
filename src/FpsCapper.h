#ifndef HAPPYFPSCAP
#define HAPPYFPSCAP


#define MILISECONDSPERFRAME 17

// The milisecodnds at the start of the frame.
unsigned int frameStartMiliseconds;
#if COUNTFRAMES == 1
	unsigned int numberOfFrames;
#endif
signed char capEnabled = 1;

void fpsCapStart(){
	#if PLATFORM == PLAT_VITA
	#else
		frameStartMiliseconds = getTicks();
	#endif
}

void fpsCapWait(){
	#if PLATFORM == PLAT_VITA
		sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
	#else
		if (capEnabled==1){
			unsigned int tempHold;
			tempHold = getTicks();
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