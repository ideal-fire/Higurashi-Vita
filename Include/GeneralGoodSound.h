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
#if SOUNDPLAYER == SND_3DS
	#include <3ds.h>
	#include <ogg/ogg.h>
	#include <vorbis/codec.h>
	#include <vorbis/vorbisfile.h>
	typedef struct{
		OggVorbis_File _musicOggFile;
		char* _musicMusicBuffer[10];
		ndspWaveBuf _musicWaveBuffer[10];
		int _musicOggCurrentSection; // Use by libvorbis
		char _musicIsTwoBuffers;
		unsigned char _musicChannel;
		unsigned char _musicShoudLoop;
		char _musicIsDone;
	}NathanMusic;
	#define CROSSMUSIC NathanMusic
	#define CROSSSFX NathanMusic
	#define CROSSPLAYHANDLE int
	void nathanUpdateMusicIfNeeded(NathanMusic* _passedMusic);
#endif
#if SOUNDPLAYER == SND_VITA
	#include <psp2/audioout.h>
	
	#include <ogg/ogg.h>
	#include <vorbis/codec.h>
	#include <vorbis/vorbisfile.h>
	#include <psp2/kernel/threadmgr.h> 
	
	#include <samplerate.h>
	typedef struct fruhfreuir{
		void* mainAudioStruct;
		void** audioBuffers;
		float* tempFloatConverted;
		SRC_DATA usualConverterData;
		SRC_STATE* personalConverter;
		unsigned int unscaledSamples;
		unsigned int scaledSamples;
		signed char numBuffers;
		signed char fileFormat;
		signed char shouldLoop;
		SceUID playerThreadId;
		signed int audioPort;
		signed char quitStatus;
		signed char isFadingOut;
		unsigned int fadeoutPerSwap;
		signed int volume;
	}NathanAudio;
	#define CROSSMUSIC NathanAudio
	#define CROSSSFX CROSSMUSIC
	#define CROSSPLAYHANDLE CROSSMUSIC*

	#define FILE_FORMAT_NONE 0
	#define FILE_FORMAT_OGG 1
	#define FILE_FORMAT_MP3 2
	#define FILE_FORMAT_WAV 3

	CROSSMUSIC* loadMusicFILE(FILE* fp, char _passedFormat);
	CROSSSFX* loadSoundFILE(FILE* fp, char _passedFormat);
	CROSSMUSIC* _mlgsnd_loadAudioFILE(legArchiveFile _passedFile, char _passedFileFormat, char _passedShouldLoop, char _passedShouldStream);
#endif

void quitAudio();
void fadeoutMusic(CROSSPLAYHANDLE _passedHandle,int time);
void freeMusic(CROSSMUSIC* toFree);
void freeSound(CROSSSFX* toFree);
int getMusicVolume(CROSSPLAYHANDLE _passedMusicHandle);
char initAudio();
CROSSMUSIC* loadMusic(char* filepath);
CROSSSFX* loadSound(char* filepath);
void pauseMusic(CROSSPLAYHANDLE _passedHandle);
CROSSPLAYHANDLE playMusic(CROSSMUSIC* toPlay, unsigned char _passedChannel);
CROSSPLAYHANDLE playSound(CROSSSFX* toPlay, int timesToPlay, unsigned char _passedChannel);
void resumeMusic(CROSSPLAYHANDLE _passedHandle);
void setMusicVolumeBefore(CROSSMUSIC* _passedMusic,int vol);
void setMusicVolume(CROSSPLAYHANDLE _passedMusic,int vol);
void setSFXVolumeBefore(CROSSSFX* tochange, int toval);
void setSFXVolume(CROSSPLAYHANDLE tochange, int toval);
void stopMusic(CROSSPLAYHANDLE toStop);
void stopSound(CROSSSFX* toStop);
//char isAudioPlaying(CROSSPLAYHANDLE _passedHandle);

#endif /* GENERALGOODGRAPHICS_H */