#include <windows.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846f

#define ARR_LEN(arr) (sizeof(arr)/sizeof(*arr))

typedef enum {
  COLOR_BLACK   = 0xFF000000,
  COLOR_WHITE   = 0xFFFFFFFF,
  COLOR_GREEN   = 0xFF00FF00,
  COLOR_RED     = 0xFFFF0000,
  COLOR_BLUE    = 0xFF0000FF,
  COLOR_YELLOW  = 0xFFFFFF00,
  COLOR_MAGENTA = 0xFFFF00FF,
  COLOR_CYAN    = 0xFF00FFFF,
  COLOR_PINK    = 0xFFF6A5D1,
} Color;

typedef struct {
  Color *memory;
  size_t size;
  int width;
  int height;
  int windowWidth;
  int windowHeight;
  HDC deviceContext;
  BITMAPINFO info;
} BackBuffer;

BackBuffer makeBackBuffer(int width, int height) {
  BackBuffer bb = {0};

  bb.size = width * height * sizeof(*bb.memory);
  bb.memory = malloc(bb.size);

  bb.width = width;
  bb.height = height;

  bb.info.bmiHeader.biSize = sizeof(bb.info.bmiHeader);
  bb.info.bmiHeader.biWidth = width;
  bb.info.bmiHeader.biHeight = height;
  bb.info.bmiHeader.biPlanes = 1;
  bb.info.bmiHeader.biBitCount = 32;
  bb.info.bmiHeader.biCompression = BI_RGB;

  return bb;
}

void setPixel(BackBuffer *bb, int x, int y, Color color) {
  bb->memory[y*bb->width + x] = color;
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
  WNDCLASS wndClass = {0};
  wndClass.style = CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc = wndProc;
  wndClass.hInstance = inst;
  wndClass.hCursor = LoadCursor(0, IDC_ARROW);
  wndClass.lpszClassName = "Boulder Dash";
  RegisterClass(&wndClass);

  int windowWidth = 1920/2;
  int windowHeight = 1080/2;

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
  int bbInvScale = 4;
  BackBuffer bb = makeBackBuffer(windowWidth/bbInvScale, windowHeight/bbInvScale);

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

    for (size_t i = 0; i < bb.width*bb.height; ++i) {
      bb.memory[i] = COLOR_GREEN;
    }

    StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight,
                  0, 0, bb.width, bb.height, bb.memory,
                  &bb.info, DIB_RGB_COLORS, SRCCOPY);
  }
}
