#ifndef HAPPYFPSCAP
#define HAPPYFPSCAP

#define COUNTFRAMES 0

#define MILISECONDSPERFRAME 17

// The milisecodnds at the start of the frame.
unsigned int frameStartMiliseconds;
#if COUNTFRAMES == 1
	unsigned int numberOfFrames;
#endif
signed char capEnabled = 1;

void FpsCapStart(){
	frameStartMiliseconds = getTicks();
}

void FpsCapWait(){
	if (capEnabled==1){
		#if COUNTFRAMES == 1
			// I just hope I only use this at the end of a frame....
			numberOfFrames=numberOfFrames+1;
		#endif
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
	}else{
		#if PLATFORM == PLAT_VITA
			sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
		#endif
	}
}

#endif