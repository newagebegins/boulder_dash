#include <windows.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(*array))
#define PI 3.14159265358979323846f

#define TILE_WIDTH 16
#define TILE_HEIGHT TILE_WIDTH
#define BACKBUFFER_WIDTH 256
#define BACKBUFFER_HEIGHT 192
#define BACKBUFFER_PIXEL_COUNT BACKBUFFER_WIDTH*BACKBUFFER_HEIGHT
#define SPRITE_ATLAS_WIDTH 256
#define MAP_HEIGHT 2
#define MAP_WIDTH 3

typedef enum {
  Tile_earth,
  Tile_Gem,
  Tile_wall,
  Tile_brick,
  Tile_count,
} Tile;

uint32_t *backbuffer;
uint32_t *spriteAtlas;
int cameraX;
int cameraY;

void drawSprite(int bbX, int bbY, int atlX, int atlY) {
  for (int y = 0; y < TILE_HEIGHT; ++y) {
    for (int x = 0; x < TILE_WIDTH; ++x) {
      int srcX = atlX + x;
      int srcY = atlY + y;
      int dstX = bbX + x;
      int dstY = bbY + y;
      backbuffer[dstY*BACKBUFFER_WIDTH + dstX] = spriteAtlas[srcY*SPRITE_ATLAS_WIDTH + srcX];
    }
  }
}

void drawTile(Tile tile, int col, int row) {
  int atlX = 0;
  int atlY = 0;

  switch (tile) {
    case Tile_earth:
      atlX = 16;
      atlY = 32;
      break;

    case Tile_brick:
      atlX = 16;
      atlY = 128;
      break;
  }

  int x = col * TILE_WIDTH - cameraX;
  int y = row * TILE_HEIGHT - cameraY;

  drawSprite(x, y, atlX, atlY);
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

char *level1 =
  "#################\n"
  "#...... ..$.o ..#\n"
  "#.oRo...... ....#\n"
  "#.......... .. .#\n"
  "#o.  ...........#\n"
  "#o.oo...........#\n"
  "#...o..o........#\n"
  "#===============#\n"
  "#. ...o..$. ..o.#\n"
  "#..$.....o..... #\n"
  "#################";

int CALLBACK WinMain(HINSTANCE inst, HINSTANCE prevInst, LPSTR cmdLine, int cmdShow) {
  backbuffer = malloc(BACKBUFFER_WIDTH * BACKBUFFER_HEIGHT * sizeof(*backbuffer));

  BITMAPINFO backbufferBmpInf = {0};
  backbufferBmpInf.bmiHeader.biSize = sizeof(backbufferBmpInf.bmiHeader);
  backbufferBmpInf.bmiHeader.biWidth = BACKBUFFER_WIDTH;
  backbufferBmpInf.bmiHeader.biHeight = -BACKBUFFER_HEIGHT;
  backbufferBmpInf.bmiHeader.biPlanes = 1;
  backbufferBmpInf.bmiHeader.biBitCount = 32;
  backbufferBmpInf.bmiHeader.biCompression = BI_RGB;

  WNDCLASS wndClass = {0};
  wndClass.style = CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc = wndProc;
  wndClass.hInstance = inst;
  wndClass.hCursor = LoadCursor(0, IDC_ARROW);
  wndClass.lpszClassName = "Boulder Dash";
  RegisterClass(&wndClass);

  int windowWidth = BACKBUFFER_WIDTH*3;
  int windowHeight = BACKBUFFER_HEIGHT*3;

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

  spriteAtlas = malloc(imageWidth * imageHeight * sizeof(*spriteAtlas));

  for (int dstRow = 0; dstRow < imageHeight; ++dstRow) {
    int srcRow = imageHeight - dstRow - 1;
    uint32_t *dst = spriteAtlas + dstRow*imageWidth;
    uint8_t *src = pixels + srcRow*imageWidth*3;
    for (int i = 0, j = 0; i < imageWidth*3; i += 3, ++j) {
      int red   = *(src + i + 0) << 0;
      int green = *(src + i + 1) << 8;
      int blue  = *(src + i + 2) << 16;
      int color = red | green | blue;
      *(dst + j) = color;
    }
  }

  Tile map[MAP_HEIGHT][MAP_WIDTH] = {
    {Tile_earth, Tile_earth, Tile_earth},
    {Tile_earth, Tile_brick, Tile_earth},
  };

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

    for (int i = 0; i < BACKBUFFER_PIXEL_COUNT; ++i) {
      backbuffer[i] = 0xFF00FF00;
    }

    for (int row = 0; row < MAP_HEIGHT; ++row) {
      for (int col = 0; col < MAP_WIDTH; ++col) {
        drawTile(map[row][col], col, row);
      }
    }

    StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight,
                  0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT, backbuffer,
                  &backbufferBmpInf, DIB_RGB_COLORS, SRCCOPY);
  }
}
