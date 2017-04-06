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
#define BACKBUFFER_BYTES BACKBUFFER_PIXEL_COUNT*sizeof(int)
#define SPRITE_ATLAS_WIDTH 256
#define HERO_ANIM_FRAME_DURATION 0.05f
#define HERO_ANIM_FRAME_COUNT 6
#define TURN_DURATION 0.15f
#define MAX_MAP_TILES 100*100

typedef enum {
  TILE_TYPE_EMPTY,
  TILE_TYPE_HERO,
  TILE_TYPE_ROCK,
  TILE_TYPE_WALL,
  TILE_TYPE_BRICK,
  TILE_TYPE_EARTH,
  TILE_TYPE_COUNT,
} TileType;

typedef struct {
  TileType type;
  bool moved;
} Tile;

uint32_t *backbuffer;
uint32_t *spriteAtlas;
int cameraX = 0;
int cameraY = 0;
int heroMoveFrame = 0;
float heroAnimTimer = 0;
bool heroIsFacingRight = false;
bool heroIsRunning = false;

void drawSprite(int bbX, int bbY, int atlX, int atlY, bool flipHorizontally) {
  for (int y = 0; y < TILE_HEIGHT; ++y) {
    for (int x = 0; x < TILE_WIDTH; ++x) {
      int srcX;
      if (flipHorizontally) {
        srcX = atlX + TILE_WIDTH - x - 1;
      } else {
        srcX = atlX + x;
      }
      int srcY = atlY + y;
      int dstX = bbX + x;
      int dstY = bbY + y;
      int dstOffset = dstY*BACKBUFFER_WIDTH + dstX;
      assert(dstOffset >= 0 && dstOffset < BACKBUFFER_PIXEL_COUNT);
      backbuffer[dstOffset] = spriteAtlas[srcY*SPRITE_ATLAS_WIDTH + srcX];
    }
  }
}

void drawTile(TileType tile, int col, int row) {
  if (tile == TILE_TYPE_EMPTY) {
    return;
  }

  bool flipHorizontally = false;
  int atlX = 0;
  int atlY = 0;

  switch (tile) {
    case TILE_TYPE_EARTH:
      atlX = 32;
      atlY = 16;
      break;

    case TILE_TYPE_BRICK:
      atlX = 48;
      atlY = 16;
      break;

    case TILE_TYPE_HERO:
      if (heroAnimTimer > 0) {
        atlX = 32 + heroMoveFrame * 16;
        atlY = 0;
        flipHorizontally = heroIsFacingRight;
      } else {
        atlX = 0;
        atlY = 0;
      }
      break;

    case TILE_TYPE_ROCK:
      atlX = 16;
      atlY = 16;
      break;

    case TILE_TYPE_WALL:
      atlX = 0;
      atlY = 16;
      break;
  }

  int x = col * TILE_WIDTH - cameraX;
  int y = row * TILE_HEIGHT - cameraY;

  drawSprite(x, y, atlX, atlY, flipHorizontally);
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
  "################"
  "#...... .. .o .#"
  "o.oRo...... ...#"
  "#.......... .. #"
  "#o.  ..........#"
  "#o.oo..........#"
  "#...o..o.......#"
  "#==============#"
  "#. ...o.. . ..o#"
  "#.. .....o.....#"
  "#.. .....o.....#"
  "################";

int CALLBACK WinMain(HINSTANCE inst, HINSTANCE prevInst, LPSTR cmdLine, int cmdShow) {
  backbuffer = malloc(BACKBUFFER_BYTES);

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

  float turnTimer = 0;
  bool rightIsDown = false;
  bool leftIsDown = false;
  bool upIsDown = false;
  bool downIsDown = false;
  int heroRow = 0;
  int heroCol = 0;

  int mapWidth = 16;
  int mapHeight = 12;
  int mapTiles = mapWidth * mapHeight;
  Tile map[MAX_MAP_TILES];
  for (int i = 0; i < mapTiles; ++i) {
    switch (level1[i]) {
      case '#':
        map[i].type = TILE_TYPE_WALL;
        break;
      case '.':
        map[i].type = TILE_TYPE_EARTH;
        break;
      case 'o':
        map[i].type = TILE_TYPE_ROCK;
        break;
      case 'R':
        map[i].type = TILE_TYPE_HERO;
        break;
      case '=':
        map[i].type = TILE_TYPE_BRICK;
        break;
      default:
        assert("Unhandled type!");
    }
    if (map[i].type == TILE_TYPE_HERO) {
      heroRow = i / mapWidth;
      heroCol = i % mapWidth;
    }
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

            case VK_LEFT:
              leftIsDown = isDown;
              break;

            case VK_RIGHT:
              rightIsDown = isDown;
              break;

            case VK_UP:
              upIsDown = isDown;
              break;
              
            case VK_DOWN:
              downIsDown = isDown;
              break;
          }
          break;

        default:
          TranslateMessage(&msg);
          DispatchMessage(&msg);
          break;
      }
    }

    if (rightIsDown || leftIsDown || upIsDown || downIsDown) {
      heroAnimTimer += dt;

      if (rightIsDown) {
        heroIsFacingRight = true;
      } else if (leftIsDown) {
        heroIsFacingRight = false;
      }
    } else {
      heroAnimTimer = 0;
    }

    int k = (int)(heroAnimTimer / HERO_ANIM_FRAME_DURATION);
    heroMoveFrame = k % HERO_ANIM_FRAME_COUNT;

    turnTimer += dt;
    if (turnTimer >= TURN_DURATION) {
      turnTimer -= TURN_DURATION;

      //
      // Do turn
      //

      for (int row = 0; row < mapHeight; ++row) {
        for (int col = 0; col < mapWidth; ++col) {
          map[row*mapWidth + col].moved = false;
        }
      }

      // Move rocks (processing order: top to bottom, left to right).
      // Move only one tile per turn.
      for (int row = 0; row < mapHeight; ++row) {
        for (int col = 0; col < mapWidth; ++col) {
          int current = row*mapWidth + col;
          int below = (row+1)*mapWidth + col;
          if (map[current].type == TILE_TYPE_ROCK && !map[current].moved && map[below].type == TILE_TYPE_EMPTY) {
            map[current].type = TILE_TYPE_EMPTY;
            map[below].type = TILE_TYPE_ROCK;
            map[below].moved = true;
          }
        }
      }

      // Move hero
      {
        int newRow = heroRow;
        int newCol = heroCol;

        if (rightIsDown) {
          ++newCol;
        } else if (leftIsDown) {
          --newCol;
        } else if (upIsDown) {
          --newRow;
        } else if (downIsDown) {
          ++newRow;
        }

        int newCell = newRow*mapWidth + newCol;
        if (map[newCell].type != TILE_TYPE_WALL && map[newCell].type != TILE_TYPE_BRICK && map[newCell].type != TILE_TYPE_ROCK) {
          map[heroRow*mapWidth + heroCol].type = TILE_TYPE_EMPTY;
          heroRow = newRow;
          heroCol = newCol;
          assert(heroRow >= 0 && heroRow < mapHeight && heroCol >= 0 && heroCol < mapWidth);
          map[heroRow*mapWidth + heroCol].type = TILE_TYPE_HERO;
        }
      }
    }

    memset(backbuffer, 0, BACKBUFFER_BYTES);

    for (int row = 0; row < mapHeight; ++row) {
      for (int col = 0; col < mapWidth; ++col) {
        drawTile(map[row*mapWidth + col].type, col, row);
      }
    }

    StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight,
                  0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT, backbuffer,
                  &backbufferBmpInf, DIB_RGB_COLORS, SRCCOPY);
  }
}
