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
  sys->initialAddingTimeToScoreSoundFrequency = 200.0f;
  sys->addingTimeToScoreSoundFrequency = sys->initialAddingTimeToScoreSoundFrequency;
  sys->addingTimeToScoreSoundFrequencyStep = 5.0f;

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
    float toneFrequency = 0.0f;
    float baseFrequency = 0.0f;
    float freqVariance = 0.0f;
    float amplitude = 0.0f;
    float soundDurationSec = 0.0f;
    float baseDuration = 0.0f;
    float durationVariance = 0.0f;

    // TODO(slava): More sounds
    // TODO(slava): Let specify attack, decay, etc?
    switch (soundId) {
      case SND_ROCKFORD_MOVE_SPACE:
        baseFrequency = 100.0f;
        freqVariance = 20.0f;
        baseDuration = 0.1f;
        amplitude = 0.1f;
        break;
      case SND_ROCKFORD_MOVE_DIRT:
        baseFrequency = 800.0f;
        freqVariance = 100.0f;
        baseDuration = 0.1f;
        amplitude = 0.1f;
        break;
      case SND_DIAMOND_PICK_UP:
        baseFrequency = 2500.0f;
        freqVariance = 200.0f;
        baseDuration = 0.4f;
        amplitude = 0.2f;
        break;
      case SND_BOULDER:
        baseFrequency = 500.0f;
        freqVariance = 0.0f;
        baseDuration = 0.5f;
        amplitude = 0.1f;
        break;
      case SND_ADDING_TIME_TO_SCORE:
        baseFrequency = sys->addingTimeToScoreSoundFrequency;
        baseDuration = 1.0f;
        amplitude = 0.2f;
        break;
      case SND_UPDATE_CELL_COVER:
        baseFrequency = 4000.0f;
        freqVariance = 1000.0f;
        baseDuration = 0.1f;
        amplitude = 0.1f;
        break;
      case SND_UPDATE_TILE_COVER:
        baseFrequency = 3000.0f;
        freqVariance = 2500.0f;
        baseDuration = 0.12f;
        durationVariance = 0.09f;
        amplitude = 0.05f;
        break;
      case SND_ROCKFORD_BIRTH:
        baseFrequency = 3000.0f;
        baseDuration = 0.2f;
        amplitude = 0.2f;
        break;
      case SND_AMOEBA:
        baseFrequency = 4000.0f;
        freqVariance = 3500.0f;
        baseDuration = 0.4f;
        durationVariance = 0.2f;
        amplitude = 0.01f;
        break;
      default:
        assert(!"Unknown sound ID");
    }

    toneFrequency = baseFrequency + freqVariance*(rand()/(float)RAND_MAX) - freqVariance;
    soundDurationSec = sys->tickDuration*(baseDuration + durationVariance*(rand()/(float)RAND_MAX) - durationVariance);

    freeSound->isPlaying = true;
    freeSound->phase = 0;
    freeSound->phaseStep = TWO_PI*toneFrequency / sys->samplesPerSecond;
    freeSound->samplesLeftToPlay = (int)(soundDurationSec * sys->samplesPerSecond);
    freeSound->amplitude = amplitude;
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
      if (sound->isPlaying) {
        fval += (sound->phase < PI ? -1.0f : 1.0f) * sound->amplitude;
        sound->phase += sound->phaseStep;
        if (sound->phase >= TWO_PI) {
          sound->phase -= TWO_PI;
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
    float amplitude = 0.7f;
    INT32 val = (INT32)(fval * amplitude * sys->maxIntegerSampleValue);

    for (int channel = 0; channel < sys->channelsCount; ++channel)
      for (int byte = 0; byte < sys->bytesPerSample; ++byte)
        buffer[b++] = (val >> (byte * 8)) & 0xFF;
  }

  hr = sys->renderClient->lpVtbl->ReleaseBuffer(sys->renderClient, availableFramesCount, 0);
  assert(SUCCEEDED(hr));
}
