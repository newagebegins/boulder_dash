#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "bitmaps.h"
#include "caves.h"

#define TILE_SIZE (2*SPRITE_SIZE)
#define BORDER_SIZE TILE_SIZE

// Viewport is the whole screen area except the border.
#define VIEWPORT_WIDTH 256
#define VIEWPORT_HEIGHT 192
#define VIEWPORT_X_MIN BORDER_SIZE
#define VIEWPORT_Y_MIN BORDER_SIZE
#define VIEWPORT_X_MAX (VIEWPORT_X_MIN + VIEWPORT_WIDTH - 1)
#define VIEWPORT_Y_MAX (VIEWPORT_Y_MIN + VIEWPORT_HEIGHT - 1)

#define BACKBUFFER_WIDTH (VIEWPORT_WIDTH + BORDER_SIZE*2)
#define BACKBUFFER_HEIGHT (VIEWPORT_HEIGHT + BORDER_SIZE*2)
#define BACKBUFFER_PIXELS (BACKBUFFER_WIDTH*BACKBUFFER_HEIGHT)
#define BACKBUFFER_BYTES (BACKBUFFER_PIXELS*sizeof(uint8_t))

#define PALETTE_COLORS 2

#define WINDOW_SCALE 3
#define WINDOW_WIDTH (BACKBUFFER_WIDTH * WINDOW_SCALE)
#define WINDOW_HEIGHT (BACKBUFFER_HEIGHT * WINDOW_SCALE)

void setPixel(uint8_t *backbuffer, int x, int y, uint8_t color) {
  backbuffer[y*BACKBUFFER_WIDTH + x] = color;
}

void drawFilledRect(uint8_t *backbuffer, int left, int top, int right, int bottom, uint8_t color) {
  for (int y = top; y <= bottom; ++y) {
    for (int x = left; x <= right; ++x) {
      setPixel(backbuffer, x, y, color);
    }
  }
}

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
  UNREFERENCED_PARAMETER(prevInst);
  UNREFERENCED_PARAMETER(cmdLine);

  uint8_t *backbuffer = malloc(BACKBUFFER_BYTES);

  BITMAPINFO *backbufferBmpInf = malloc(sizeof(BITMAPINFOHEADER) + (PALETTE_COLORS*sizeof(RGBQUAD)));
  backbufferBmpInf->bmiHeader.biSize = sizeof(backbufferBmpInf->bmiHeader);
  backbufferBmpInf->bmiHeader.biWidth = BACKBUFFER_WIDTH;
  backbufferBmpInf->bmiHeader.biHeight = -BACKBUFFER_HEIGHT;
  backbufferBmpInf->bmiHeader.biPlanes = 1;
  backbufferBmpInf->bmiHeader.biBitCount = 8;
  backbufferBmpInf->bmiHeader.biCompression = BI_RGB;
  backbufferBmpInf->bmiHeader.biClrUsed = PALETTE_COLORS;

  backbufferBmpInf->bmiColors[0].rgbBlue = 0;
  backbufferBmpInf->bmiColors[0].rgbGreen = 0;
  backbufferBmpInf->bmiColors[0].rgbRed = 0xFF;

  backbufferBmpInf->bmiColors[1].rgbBlue = 0;
  backbufferBmpInf->bmiColors[1].rgbGreen = 0xFF;
  backbufferBmpInf->bmiColors[1].rgbRed = 0;

  WNDCLASS wndClass = {0};
  wndClass.style = CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc = wndProc;
  wndClass.hInstance = inst;
  wndClass.hCursor = LoadCursor(0, IDC_ARROW);
  wndClass.lpszClassName = "Boulder Dash";
  RegisterClass(&wndClass);

  RECT crect = {0};
  crect.right = WINDOW_WIDTH;
  crect.bottom = WINDOW_HEIGHT;

  DWORD wndStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
  AdjustWindowRect(&crect, wndStyle, 0);

  HWND wnd = CreateWindowEx(0, wndClass.lpszClassName, "Boulder Dash", wndStyle, 300, 100,
                            crect.right - crect.left, crect.bottom - crect.top,
                            0, 0, inst, 0);
  ShowWindow(wnd, cmdShow);
  UpdateWindow(wnd);

  float dt = 0.0f;
  float targetFps = 60.0f;
  float maxDt = 1.0f / targetFps;
  LARGE_INTEGER perfcFreq = {0};
  LARGE_INTEGER perfc = {0};
  LARGE_INTEGER perfcPrev = {0};

  QueryPerformanceFrequency(&perfcFreq);
  QueryPerformanceCounter(&perfc);

  bool running = true;
  HDC deviceContext = GetDC(wnd);

  float timer = 0;

  while (running) {
    perfcPrev = perfc;
    QueryPerformanceCounter(&perfc);
    dt = (float)(perfc.QuadPart - perfcPrev.QuadPart) / (float)perfcFreq.QuadPart;
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

    timer += dt;

    if (timer >= 3.0f) {
      backbufferBmpInf->bmiColors[1].rgbBlue = 0xFF;
      backbufferBmpInf->bmiColors[1].rgbGreen = 0;
      backbufferBmpInf->bmiColors[1].rgbRed = 0;
    }

    // Draw border
    drawFilledRect(backbuffer, 0, 0, BACKBUFFER_WIDTH-1, BACKBUFFER_HEIGHT-1, 0);
    // Clear viewport
    drawFilledRect(backbuffer, VIEWPORT_X_MIN, VIEWPORT_Y_MIN, VIEWPORT_X_MAX, VIEWPORT_Y_MAX, 1);

    StretchDIBits(deviceContext,
                  0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                  0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT,
                  backbuffer, backbufferBmpInf,
                  DIB_RGB_COLORS, SRCCOPY);
  }

  return 0;
}
