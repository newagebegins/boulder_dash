inline UINT32 power(UINT32 base, UINT32 exponent) {
  UINT32 result = 1;
  while (exponent-- > 0) result *= base;
  return result;
}

static void initializeSoundSystem(SoundSystem *sys, float bufferDurationSec, float tickDuration) {
  //
  // Initialize WASAPI
  //

  HRESULT hr;

  IMMDeviceEnumerator *enumerator;
  CoInitialize(NULL);
  hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&enumerator);
  assert(SUCCEEDED(hr));

  IMMDevice *device;
  hr = enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device);
  assert(SUCCEEDED(hr));

  IAudioClient *audioClient;
  hr = device->lpVtbl->Activate(device, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&audioClient);
  assert(SUCCEEDED(hr));

  WAVEFORMATEX *mixFormat;
  hr = audioClient->lpVtbl->GetMixFormat(audioClient, &mixFormat);
  assert(SUCCEEDED(hr));

  WAVEFORMATEX waveFormat;
  memcpy(&waveFormat, mixFormat, sizeof(WAVEFORMATEX));
  waveFormat.wFormatTag = WAVE_FORMAT_PCM;
  waveFormat.cbSize = 0;

  REFERENCE_TIME duration = (REFERENCE_TIME)(bufferDurationSec*REFTIMES_PER_SEC);
  hr = audioClient->lpVtbl->Initialize(audioClient, AUDCLNT_SHAREMODE_SHARED, 0, duration, 0, &waveFormat, NULL);
  assert(SUCCEEDED(hr));

  UINT32 bufferFramesCount;
  hr = audioClient->lpVtbl->GetBufferSize(audioClient, &bufferFramesCount);
  assert(SUCCEEDED(hr));

  IAudioRenderClient *renderClient;
  hr = audioClient->lpVtbl->GetService(audioClient, &IID_IAudioRenderClient, (void**)&renderClient);
  assert(SUCCEEDED(hr));

  sys->audioClient = audioClient;
  sys->renderClient = renderClient;
  sys->bufferFramesCount = bufferFramesCount;
  sys->maxIntegerSampleValue = power(2, waveFormat.wBitsPerSample - 1) - 1;
  sys->bytesPerSample = waveFormat.wBitsPerSample / 8;
  sys->channelsCount = waveFormat.nChannels;
  sys->samplesPerSecond = waveFormat.nSamplesPerSec;
  sys->tickDuration = tickDuration;
  sys->initialAddingTimeToScoreSoundFrequency = 500.0f;
  sys->addingTimeToScoreSoundFrequency = sys->initialAddingTimeToScoreSoundFrequency;
  sys->addingTimeToScoreSoundFrequencyStep = 8.0f;

  audioClient->lpVtbl->Start(audioClient);
}

static void fillNoiseBuffer(Sound *sound) {
  for (int i = 0; i < ARRAY_LENGTH(sound->noise); ++i) {
    sound->noise[i] = 2.0f*((float)rand()/(float)RAND_MAX) - 1.0f;
  }
}

static void playSound(SoundSystem *sys, SoundID soundId) {
  Sound *freeSound = 0;
  for (int soundIndex = 0; soundIndex < ARRAY_LENGTH(sys->sounds); ++soundIndex) {
    Sound *sound = &sys->sounds[soundIndex];
    if (!sound->isPlaying) {
      freeSound = sound;
      break;
    }
  }

  if (freeSound) {
    float toneFrequency;
    float soundDurationSec;
    ToneShape toneShape;
    float amplitude;

    // TODO(slava): More sounds
    // TODO(slava): Let specify attack, decay, etc?
    switch (soundId) {
      case SND_ROCKFORD_MOVE_SPACE:
        toneFrequency = 100.0f;
        soundDurationSec = sys->tickDuration;
        toneShape = TONE_SHAPE_NOISE;
        amplitude = 0.1f;
        break;
      case SND_ROCKFORD_MOVE_DIRT:
        toneFrequency = 2000.0f;
        soundDurationSec = 0.3f*sys->tickDuration;
        toneShape = TONE_SHAPE_NOISE;
        amplitude = 0.1f;
        break;
      case SND_DIAMOND_PICK_UP: {
        float variance = 200.0f;
        toneFrequency = 2000.0f + variance*(rand()/(float)RAND_MAX) - variance;
        soundDurationSec = 0.5f*sys->tickDuration;
        toneShape = TONE_SHAPE_TRIANGLE;
        amplitude = 0.4f;
        break;
      }
      case SND_BOULDER:
        toneFrequency = 1500.0f;
        soundDurationSec = 0.7f*sys->tickDuration;
        toneShape = TONE_SHAPE_NOISE;
        amplitude = 0.1f;
        break;
      case SND_ADDING_TIME_TO_SCORE:
        toneFrequency = sys->addingTimeToScoreSoundFrequency;
        soundDurationSec = sys->tickDuration;
        toneShape = TONE_SHAPE_TRIANGLE;
        amplitude = 0.4f;
        break;
      case SND_UPDATE_CELL_COVER:
        float variance = 1000.0f;
        toneFrequency = 4000.0f + variance*(rand()/(float)RAND_MAX) - variance;
        soundDurationSec = 0.1f*sys->tickDuration;
        toneShape = TONE_SHAPE_SQUARE;
        amplitude = 0.1f;
        break;
      default:
        assert(!"Unknown sound ID");
    }

    freeSound->isPlaying = true;
    freeSound->phase = 0;
    freeSound->phaseStep = TWO_PI*toneFrequency / sys->samplesPerSecond;
    freeSound->samplesLeftToPlay = (int)(soundDurationSec * sys->samplesPerSecond);
    freeSound->toneShape = toneShape;
    freeSound->amplitude = amplitude;
    if (toneShape == TONE_SHAPE_NOISE) {
      fillNoiseBuffer(freeSound);
    }
  } else {
    // All sound slots are occupied.
  }
}

static void outputSound(SoundSystem *sys) {
  HRESULT hr;

  UINT32 paddingFramesCount;
  hr = sys->audioClient->lpVtbl->GetCurrentPadding(sys->audioClient, &paddingFramesCount);
  assert(SUCCEEDED(hr));

  UINT32 availableFramesCount = sys->bufferFramesCount - paddingFramesCount;

  BYTE *buffer;
  hr = sys->renderClient->lpVtbl->GetBuffer(sys->renderClient, availableFramesCount, &buffer);
  assert(SUCCEEDED(hr));

  for (UINT32 frame = 0, b = 0; frame < availableFramesCount; ++frame) {
    float fval = 0;
    for (int soundIndex = 0; soundIndex < ARRAY_LENGTH(sys->sounds); ++soundIndex) {
      Sound *sound = &sys->sounds[soundIndex];
      float v = 0;
      if (sound->isPlaying) {
        switch (sound->toneShape) {
          case TONE_SHAPE_SINE:
            v = sinf(sound->phase);
            break;
          case TONE_SHAPE_SQUARE:
            if (sound->phase < PI) {
              v = -1.0f;
            } else {
              v = 1.0f;
            }
            break;
          case TONE_SHAPE_TRIANGLE:
            if (sound->phase < 0.5f*PI) {
              v = 2.0f*sound->phase/PI;
            } else if (sound->phase < 1.5f*PI) {
              v = 2.0f*(1.0f - sound->phase/PI);
            } else {
              v = -4.0f + (2.0f/PI)*sound->phase;
            }
            break;
          case TONE_SHAPE_NOISE: {
            int index = (int)(sound->phase/TWO_PI)*(ARRAY_LENGTH(sound->noise) - 1);
            assert(index >= 0 && index < ARRAY_LENGTH(sound->noise));
            v = sound->noise[index];
            break;
          }
        }
        fval += v * sound->amplitude;
        sound->phase += sound->phaseStep;
        if (sound->phase >= TWO_PI) {
          sound->phase -= TWO_PI;
          if (sound->toneShape == TONE_SHAPE_NOISE) {
            fillNoiseBuffer(sound);
          }
        }
        sound->samplesLeftToPlay--;
        if (sound->samplesLeftToPlay == 0) {
          sound->isPlaying = false;
        }
      }
    }

    if (fval > 1.0f) {
      fval = 1.0f;
    } else if (fval < -1.0f) {
      fval = -1.0f;
    }

    // If fval is 1.0f, an overflow of INT32 is going to happen when we multiply
    // by maxSampleVal. To avoid this, we multiply by amplitude which is less
    // than 1.
    float amplitude = 0.9f;
    INT32 val = (INT32)(fval * amplitude * sys->maxIntegerSampleValue);

    for (int channel = 0; channel < sys->channelsCount; ++channel)
      for (int byte = 0; byte < sys->bytesPerSample; ++byte)
        buffer[b++] = (val >> (byte * 8)) & 0xFF;
  }

  hr = sys->renderClient->lpVtbl->ReleaseBuffer(sys->renderClient, availableFramesCount, 0);
  assert(SUCCEEDED(hr));
}
