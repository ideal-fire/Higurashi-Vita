#ifndef GENERALGOODSOUNDHEADER
#define GENERALGOODSOUNDHEADER
	/*
	================================================
	== SOUND
	=================================================
	*/
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

	void InitAudio(){
		#if SOUNDPLAYER == SND_SDL
			SDL_Init( SDL_INIT_AUDIO );
			Mix_Init(MIX_INIT_OGG);
			Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 );
		#elif SOUNDPLAYER == SND_SOLOUD
			mySoLoudEngine = Soloud_create();
			Soloud_init(mySoLoudEngine);
		#endif
	}
	int GetMusicVolume(CROSSPLAYHANDLE _passedMusicHandle){
		#if SOUNDPLAYER == SND_SDL
			return Mix_VolumeMusic(-1);
		#elif SOUNDPLAYER == SND_SOLOUD
			return Soloud_getVolume(mySoLoudEngine,_passedMusicHandle)*128;
		#endif
	}
	void SetSFXVolumeBefore(CROSSSFX* tochange, int toval){
		#if SOUNDPLAYER == SND_SDL
			Mix_VolumeChunk(tochange,toval);
		#elif SOUNDPLAYER == SND_SOLOUD
			Wav_setVolume(tochange,(float)((float)toval/(float)128));
		#endif
	}
	void SetSFXVolume(CROSSPLAYHANDLE tochange, int toval){
		#if SOUNDPLAYER == SND_SDL
			SetSFXVolumeBefore(tochange,toval);
		#elif SOUNDPLAYER == SND_SOLOUD
			Soloud_setVolume(mySoLoudEngine,tochange,(float)((float)toval/(float)128));
		#endif
	}
	void SetMusicVolumeBefore(CROSSMUSIC* _passedMusic,int vol){
		#if SOUNDPLAYER == SND_SDL
			Mix_VolumeMusic(vol);
		#elif SOUNDPLAYER == SND_SOLOUD
			WavStream_setVolume(_passedMusic,(float)((float)vol/(float)128));
		#endif
	}
	void SetMusicVolume(CROSSPLAYHANDLE _passedMusic,int vol){
		#if SOUNDPLAYER == SND_SDL
			Mix_VolumeMusic(vol);
		#elif SOUNDPLAYER == SND_SOLOUD
			SetSFXVolume(_passedMusic,vol);
		#endif
	}
	void FadeoutMusic(CROSSPLAYHANDLE _passedHandle,int time){
		#if SOUNDPLAYER == SND_SDL
			Mix_FadeOutMusic(time);
		#elif SOUNDPLAYER == SND_SOLOUD
			Soloud_fadeVolume(mySoLoudEngine,_passedHandle,0,(double)((double)time/(double)1000));
			Soloud_scheduleStop(mySoLoudEngine,_passedHandle,(double)((double)time/(double)1000));
		#endif
	}
	CROSSSFX* LoadSound(char* filepath){
		#if SOUNDPLAYER == SND_SDL
			return Mix_LoadWAV(filepath);
		#elif SOUNDPLAYER == SND_SOLOUD
			CROSSSFX* _myLoadedSoundEffect = Wav_create();
			Wav_load(_myLoadedSoundEffect,filepath);
			return _myLoadedSoundEffect;
		#else
			return NULL;
		#endif
	}
	CROSSMUSIC* LoadMusic(char* filepath){
		#if SOUNDPLAYER == SND_SDL
			return Mix_LoadMUS(filepath);
		#elif SOUNDPLAYER == SND_SOLOUD
			CROSSMUSIC* _myLoadedMusic = WavStream_create();
			WavStream_load(_myLoadedMusic,filepath);
			return _myLoadedMusic;
		#else
			return NULL;
		#endif
	}
	void PauseMusic(CROSSPLAYHANDLE _passedHandle){
		#if SOUNDPLAYER == SND_SDL
			Mix_PauseMusic();
		#elif SOUNDPLAYER == SND_SOLOUD
			Soloud_setPause(mySoLoudEngine,_passedHandle, 1);
		#endif
	}
	void ResumeMusic(CROSSPLAYHANDLE _passedHandle){
		#if SOUNDPLAYER == SND_SDL
			Mix_ResumeMusic();
		#elif SOUNDPLAYER == SND_SOLOUD
			Soloud_setPause(mySoLoudEngine,_passedHandle, 0);
		#endif
	}
	void StopMusic(CROSSMUSIC* toStop){
		#if SOUNDPLAYER == SND_SDL
			Mix_HaltMusic();
		#elif SOUNDPLAYER == SND_SOLOUD
			if (toStop!=NULL){
				WavStream_stop(toStop);
			}
		#endif
	}
	CROSSPLAYHANDLE PlaySound(CROSSSFX* toPlay, int timesToPlay){
		#if SOUNDPLAYER == SND_SDL
			Mix_PlayChannel( -1, toPlay, timesToPlay-1 );
			return toPlay;
		#elif SOUNDPLAYER == SND_SOLOUD
			if (timesToPlay!=1){
				printf("SoLoud can only play a sound once!");
			}
			return Soloud_play(mySoLoudEngine,toPlay);
		#endif
	}
	CROSSPLAYHANDLE PlayMusic(CROSSMUSIC* toPlay){
		#if SOUNDPLAYER == SND_SDL
			Mix_PlayMusic(toPlay,-1);
			return toPlay;
		#elif SOUNDPLAYER == SND_SOLOUD
			WavStream_setLooping(toPlay,1);
			return Soloud_play(mySoLoudEngine,toPlay);
		#endif
	}
	void FreeSound(CROSSSFX* toFree){
		#if SOUNDPLAYER == SND_SDL
			Mix_FreeChunk(toFree);
		#elif SOUNDPLAYER == SND_SOLOUD
			Wav_destroy(toFree);
		#endif
	}
	void FreeMusic(CROSSMUSIC* toFree){
		#if SOUNDPLAYER == SND_SDL
			Mix_FreeMusic(toFree);
		#elif SOUNDPLAYER == SND_SOLOUD
			WavStream_destroy(toFree);
		#endif
	}
#endif