#ifndef GENERALGOODSOUND_H
#define GENERALGOODSOUND_H

#if SOUNDPLAYER == SND_SDL
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_mixer.h>

	#define CROSSSFX Mix_Chunk
	#define CROSSMUSIC Mix_Music
	#define CROSSPLAYHANDLE void*
#endif
#if SOUNDPLAYER == SND_NONE
	#define CROSSSFX int
	#define CROSSMUSIC int
	#define CROSSPLAYHANDLE char
#endif
#if SOUNDPLAYER == SND_SOLOUD
	#include <soloud_c.h>
	#define CROSSMUSIC WavStream
	#define CROSSSFX Wav
	#define CROSSPLAYHANDLE unsigned int
	Soloud* mySoLoudEngine;
#endif
	
void fadeoutMusic(CROSSPLAYHANDLE _passedHandle,int time);
void freeMusic(CROSSMUSIC* toFree);
void freeSound(CROSSSFX* toFree);
int getMusicVolume(CROSSPLAYHANDLE _passedMusicHandle);
void initAudio();
CROSSMUSIC* loadMusic(char* filepath);
CROSSSFX* loadSound(char* filepath);
void pauseMusic(CROSSPLAYHANDLE _passedHandle);
CROSSPLAYHANDLE playMusic(CROSSMUSIC* toPlay);
CROSSPLAYHANDLE playSound(CROSSSFX* toPlay, int timesToPlay);
void resumeMusic(CROSSPLAYHANDLE _passedHandle);
void setMusicVolumeBefore(CROSSMUSIC* _passedMusic,int vol);
void setMusicVolume(CROSSPLAYHANDLE _passedMusic,int vol);
void setSFXVolumeBefore(CROSSSFX* tochange, int toval);
void setSFXVolume(CROSSPLAYHANDLE tochange, int toval);
void stopMusic(CROSSMUSIC* toStop);
void stopSound(CROSSSFX* toStop);
 
#endif /* GENERALGOODGRAPHICS_H */