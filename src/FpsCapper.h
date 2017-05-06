#ifndef HAPPYFPSCAP
#define HAPPYFPSCAP

#define MILISECONDSPERFRAME 16.667

// The milisecodnds at the start of the frame.
u64 frameStartMiliseconds;
//u64 numberOfFrames;


void FpsCapStart(){
	frameStartMiliseconds = GetTicks();
}

void FpsCapWait(){
	// I just hope I only use this at the end of a frame....
	//numberOfFrames=numberOfFrames+1;
	u64 tempHold;
	tempHold = GetTicks();
	//printf("%llu;%llu\n",frameStartMiliseconds,tempHold);
	// LIMIT FPS
	if (tempHold-frameStartMiliseconds<MILISECONDSPERFRAME){
		Wait( MILISECONDSPERFRAME - (tempHold-frameStartMiliseconds));
	}else{
		//printf("Slowdown %llu\n", tempHold-frameStartMiliseconds);
		//printf("Slowdown %d\n",MILISECONDSPERFRAME - (tempHold-frameStartMiliseconds));
		//printf("Slowdown: %d\n",tempHold-frameStartMiliseconds-MILISECONDSPERFRAME);
	}
	
}

#endif