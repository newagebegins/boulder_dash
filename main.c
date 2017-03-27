#include <windows.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#define PI 3.14159265358979323846f

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(*array))

LRESULT CALLBACK wndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(wnd, msg, wparam, lparam);
  }
  return 0;
}

int CALLBACK WinMain(HINSTANCE inst, HINSTANCE prevInst, LPSTR cmdLine, int cmdShow) {
  int backBufferWidth = 256;
  int backBufferHeight = 192;
  int *backBuffer = malloc(backBufferWidth * backBufferHeight * sizeof(*backBuffer));

  BITMAPINFO backBufferBitmapInfo = {0};
  backBufferBitmapInfo.bmiHeader.biSize = sizeof(backBufferBitmapInfo.bmiHeader);
  backBufferBitmapInfo.bmiHeader.biWidth = backBufferWidth;
  backBufferBitmapInfo.bmiHeader.biHeight = backBufferHeight;
  backBufferBitmapInfo.bmiHeader.biPlanes = 1;
  backBufferBitmapInfo.bmiHeader.biBitCount = 32;
  backBufferBitmapInfo.bmiHeader.biCompression = BI_RGB;

  WNDCLASS wndClass = {0};
  wndClass.style = CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc = wndProc;
  wndClass.hInstance = inst;
  wndClass.hCursor = LoadCursor(0, IDC_ARROW);
  wndClass.lpszClassName = "Boulder Dash";
  RegisterClass(&wndClass);

  int windowWidth = backBufferWidth*3;
  int windowHeight = backBufferHeight*3;

  RECT crect = {0};
  crect.right = windowWidth;
  crect.bottom = windowHeight;

  DWORD wndStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
  AdjustWindowRect(&crect, wndStyle, 0);

  HWND wnd = CreateWindowEx(0, wndClass.lpszClassName, "Boulder Dash", wndStyle, 300, 0,
                            crect.right - crect.left, crect.bottom - crect.top,
                            0, 0, inst, 0);
  ShowWindow(wnd, cmdShow);
  UpdateWindow(wnd);

  float dt = 0.0f;
  float targetFps = 60.0f;
  float maxDt = 1.0f / targetFps;
  LARGE_INTEGER perfcFreq = {0};
  LARGE_INTEGER perfc = {0};
  LARGE_INTEGER prefcPrev = {0};

  QueryPerformanceFrequency(&perfcFreq);
  QueryPerformanceCounter(&perfc);

  bool running = true;
  HDC deviceContext = GetDC(wnd);

  HANDLE fileHandle = CreateFile("sprites.bmp", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  assert(fileHandle != INVALID_HANDLE_VALUE);
  LARGE_INTEGER fileSize;
  GetFileSizeEx(fileHandle, &fileSize);
  uint8_t *fileContents = malloc(fileSize.LowPart);
  DWORD bytesRead;
  ReadFile(fileHandle, fileContents, fileSize.LowPart, &bytesRead, NULL);
  assert(bytesRead == fileSize.LowPart);
  CloseHandle(fileHandle);

  int pixelsOffset = *(int*)(fileContents + 10);
  int imageWidth = *(int*)(fileContents + 18);
  int imageHeight = *(int*)(fileContents + 22);
  uint8_t *pixels = fileContents + pixelsOffset;

  int *texture = malloc(imageWidth * imageHeight * sizeof(*texture));
  for (int i = 0, j = 0; i < imageWidth*imageHeight*3; i += 3, ++j) {
    int red = pixels[i] << 0;
    int green = pixels[i+1] << 8;
    int blue = pixels[i+2] << 16;
    int color = red | green | blue;
    texture[j] = color;
  }

  while (running) {
    prefcPrev = perfc;
    QueryPerformanceCounter(&perfc);
    dt = (float)(perfc.QuadPart - prefcPrev.QuadPart) / (float)perfcFreq.QuadPart;
    if (dt > maxDt) {
      dt = maxDt;
    }

    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
      switch (msg.message) {
        case WM_QUIT:
          running = false;
          break;

        case WM_KEYDOWN:
        case WM_KEYUP:
          bool isDown = ((msg.lParam & (1 << 31)) == 0);
          switch (msg.wParam) {
            case VK_ESCAPE:
              running = false;
              break;
          }
          break;

        default:
          TranslateMessage(&msg);
          DispatchMessage(&msg);
          break;
      }
    }

    for (int i = 0; i < backBufferWidth * backBufferHeight; ++i) {
      backBuffer[i] = 0xFF00FF00;
    }

    for (int srcY = 0, destY = 0; srcY < imageHeight; ++srcY, ++destY) {
      for (int srcX = 0, destX = 0; srcX < imageWidth; ++srcX, ++destX) {
        backBuffer[destY*backBufferWidth + destX] = texture[srcY*imageWidth + srcX];
      }
    }

    StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight,
                  0, 0, backBufferWidth, backBufferHeight, backBuffer,
                  &backBufferBitmapInfo, DIB_RGB_COLORS, SRCCOPY);
  }
}
