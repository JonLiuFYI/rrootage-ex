#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <SDL2/SDL.h>

#else
#include "SDL.h"

#endif

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "audio.h"

#include "soundmanager.h"

static int useAudio = 0;

#define MUSIC_NUM 3

static char *musicFileName[MUSIC_NUM] = {
  "stg_a2.ogg", "stg_b2.ogg", "stg_c2.ogg",
};


static Audio* music[MUSIC_NUM];

#define CHUNK_NUM 16

static char *chunkFileName[CHUNK_NUM] = {
  "laser_start.wav", "laser.wav", "damage.wav", "bomb.wav", 
  "destroied.wav", "explosion1.wav", "explosion2.wav", "miss.wav", "extend.wav",
  "grz.wav", "grzinv.wav", 
  "shot.wav", "change.wav",
  "reflec1.wav", "reflec2.wav", "ref_ready.wav",
};
static Audio* chunk[CHUNK_NUM];
//static Mix_Chunk *chunk[CHUNK_NUM];
static int chunkChannel[CHUNK_NUM] = {
  0, 1, 2, 3,
  4, 5, 6, 7, 4,
  6, 7,
  6, 7,
  7, 7, 7,
};

void closeSound() {

}


// Initialize the sound.

static void loadSounds() {
  int i;
  char name[64];

  for (i = 0; i < MUSIC_NUM; i++) {
#ifdef PLATFORM_NX
	  strcpy(name, "Assets:/sounds/");
#else
	  strcpy(name, "resources/sounds/");
#endif
	  strcat(name, musicFileName[i]);

	  music[i] = createAudio(name,1, SDL_MIX_MAXVOLUME/3);
	  if (music[i] == NULL)
	  {
		  fprintf(stderr, "Couldn't load: %s\n", name);
	  }
  }
  for ( i=0 ; i<CHUNK_NUM ; i++ ) {
#ifdef PLATFORM_NX
	  strcpy(name, "Assets:/sounds/");
#else
	  strcpy(name, "resources/sounds/");
#endif
    strcat(name, chunkFileName[i]);

    chunk[i] = createAudio(name, 0, SDL_MIX_MAXVOLUME);
    if (chunk[i] == NULL)
    {
        fprintf(stderr, "Couldn't load: %s\n", name);
    }
  }
}

void initSound() {
  int audio_rate;
  Uint16 audio_format;
  int audio_channels;
  int audio_buffers;

  if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
	  fprintf(stderr, "Unable to initialize SDL_AUDIO: %s\n", SDL_GetError());
	  return;
  }

  initAudio();
  //SDL_AudioCallback()
 

  audio_rate = 44100;
  audio_format = AUDIO_S16;
  audio_channels = 1;
  audio_buffers = 4096;
  
  useAudio = 1;
  loadSounds();
}

// Play/Stop the music/chunk.

void playMusic(int idx) {
  if ( !useAudio ) return;
  PlayPersistentMusic(music[idx], SDL_MIX_MAXVOLUME);
}

void fadeMusic() {
  if ( !useAudio ) return;
  stopMusic();
}

void stopMusic() {
  if ( !useAudio ) return;
  PlayPersistentMusic(NULL, 0);
 
}

void playChunk(int idx) {
  if ( !useAudio ) return;

  PlayChannel(chunk[idx], idx, SDL_MIX_MAXVOLUME);
}

void haltChunk(int idx) {
  if ( !useAudio ) return;
  StopChannel(idx);
}
