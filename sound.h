#include <mmdeviceapi.h>
#include <audioclient.h>
#include <math.h>

const GUID CLSID_MMDeviceEnumerator = {0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E};
const GUID IID_IMMDeviceEnumerator = {0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6};
const GUID IID_IAudioClient = {0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2};
const GUID IID_IAudioRenderClient = {0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2};

#define REFTIMES_PER_SEC 10000000
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f

typedef enum {
  SND_ROCKFORD_MOVE_SPACE,
  SND_ROCKFORD_MOVE_DIRT,
  SND_DIAMOND_PICK_UP,
  SND_BOULDER,
  SND_ADDING_TIME_TO_SCORE,
  SND_UPDATE_CELL_COVER,
  SND_ROCKFORD_BIRTH,
} SoundID;

typedef struct {
  bool isPlaying;
  float phase;
  float phaseStep;
  int samplesLeftToPlay;
  float amplitude;
  float noise[32];
} Sound;

typedef struct {
  IAudioClient *audioClient;
  IAudioRenderClient *renderClient;
  UINT32 bufferFramesCount;
  INT32 maxIntegerSampleValue;
  int bytesPerSample;
  int channelsCount;
  int samplesPerSecond;
  float tickDuration;
  Sound sounds[2];
  float initialAddingTimeToScoreSoundFrequency;
  float addingTimeToScoreSoundFrequency;
  float addingTimeToScoreSoundFrequencyStep;
} SoundSystem;
