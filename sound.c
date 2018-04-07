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

  audioClient->lpVtbl->Start(audioClient);
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

    // TODO(slava): Add other sounds
    // TODO(slava): Let specify a form of tone (triangle, square)?
    // TODO(slava): Let specify attack, decay, etc?
    switch (soundId) {
      case SND_ROCKFORD_MOVE_SPACE:
        toneFrequency = 220.0f;
        soundDurationSec = sys->tickDuration;
        break;
      case SND_ROCKFORD_MOVE_DIRT:
        toneFrequency = 190.0f;
        soundDurationSec = sys->tickDuration;
        break;
      case SND_DIAMOND_PICK_UP:
        toneFrequency = 440.0f;
        soundDurationSec = sys->tickDuration;
        break;
      default:
        assert(!"Unknown sound ID");
    }

    freeSound->isPlaying = true;
    freeSound->phase = 0;
    freeSound->phaseStep = TWO_PI*toneFrequency / sys->samplesPerSecond;
    freeSound->samplesLeftToPlay = (int)(soundDurationSec * sys->samplesPerSecond);
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
        fval += sinf(sound->phase);
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
    float amplitude = 0.9f;
    INT32 val = (INT32)(fval * amplitude * sys->maxIntegerSampleValue);

    for (int channel = 0; channel < sys->channelsCount; ++channel)
      for (int byte = 0; byte < sys->bytesPerSample; ++byte)
        buffer[b++] = (val >> (byte * 8)) & 0xFF;
  }

  hr = sys->renderClient->lpVtbl->ReleaseBuffer(sys->renderClient, availableFramesCount, 0);
  assert(SUCCEEDED(hr));
}
