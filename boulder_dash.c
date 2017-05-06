#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#include "data_sprites.h"
#include "data_caves.h"

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(*array))

// Cave map consists of cells, each cell contains 4 tiles.

#define CELL_SIZE 16
#define BORDER_SIZE_IN_CELLS 1
#define BORDER_SIZE (CELL_SIZE*BORDER_SIZE_IN_CELLS)

// Viewport is the whole screen area except the border.
#define VIEWPORT_WIDTH 256
#define VIEWPORT_HEIGHT 192
#define VIEWPORT_WIDTH_IN_CELLS (VIEWPORT_WIDTH/CELL_SIZE)
#define VIEWPORT_HEIGHT_IN_CELLS (VIEWPORT_HEIGHT/CELL_SIZE)

#define VIEWPORT_X_MIN BORDER_SIZE
#define VIEWPORT_Y_MIN BORDER_SIZE
#define VIEWPORT_X_MAX (VIEWPORT_X_MIN + VIEWPORT_WIDTH - 1)
#define VIEWPORT_Y_MAX (VIEWPORT_Y_MIN + VIEWPORT_HEIGHT - 1)

#define VIEWPORT_X_MIN_IN_CELLS BORDER_SIZE_IN_CELLS
#define VIEWPORT_Y_MIN_IN_CELLS BORDER_SIZE_IN_CELLS
#define VIEWPORT_X_MAX_IN_CELLS (VIEWPORT_X_MIN_IN_CELLS + VIEWPORT_WIDTH_IN_CELLS - 1)
#define VIEWPORT_Y_MAX_IN_CELLS (VIEWPORT_Y_MIN_IN_CELLS + VIEWPORT_HEIGHT_IN_CELLS - 1)

#define BACKBUFFER_WIDTH (VIEWPORT_WIDTH + BORDER_SIZE*2)
#define BACKBUFFER_HEIGHT (VIEWPORT_HEIGHT + BORDER_SIZE*2)
#define BACKBUFFER_PIXELS (BACKBUFFER_WIDTH*BACKBUFFER_HEIGHT)
// 4 bits per pixel.
#define BACKBUFFER_BYTES (BACKBUFFER_PIXELS*sizeof(uint8_t)/2)

#define PALETTE_COLORS 5

#define TEXT_AREA_HEIGHT_IN_CELLS 1
#define PLAYFIELD_WIDTH_IN_CELLS VIEWPORT_WIDTH_IN_CELLS
#define PLAYFIELD_HEIGHT_IN_CELLS (VIEWPORT_HEIGHT_IN_CELLS - TEXT_AREA_HEIGHT_IN_CELLS)
#define PLAYFIELD_X_MIN_IN_CELLS VIEWPORT_X_MIN_IN_CELLS
#define PLAYFIELD_Y_MIN_IN_CELLS (VIEWPORT_Y_MIN_IN_CELLS + TEXT_AREA_HEIGHT_IN_CELLS)
#define PLAYFIELD_X_MIN (PLAYFIELD_X_MIN_IN_CELLS*CELL_SIZE)
#define PLAYFIELD_Y_MIN (PLAYFIELD_Y_MIN_IN_CELLS*CELL_SIZE)

#define CAVE_HEIGHT 22
#define CAVE_WIDTH 40
#define NUM_DIFFICULTY_LEVELS 5
#define NUM_RANDOM_OBJECTS 4

#define TURN_DURATION 0.15f
#define ROCKFORD_TURNS_TILL_BIRTH 12
#define MAP_UNCOVER_TURNS 40
#define PAUSE_TURNS_BEFORE_FULL_UNCOVER 2
#define CELLS_PER_LINE_TO_UNCOVER 3

#define OBJ_SPACE 0x00
#define OBJ_DIRT 0x01
#define OBJ_BRICK_WALL 0x02
#define OBJ_MAGIC_WALL 0x03
#define OBJ_PRE_OUTBOX 0x04
#define OBJ_FLASHING_OUTBOX 0x05
#define OBJ_STEEL_WALL 0x07
#define OBJ_FIREFLY_POSITION_1 0x08
#define OBJ_FIREFLY_POSITION_2 0x09
#define OBJ_FIREFLY_POSITION_3 0x0A
#define OBJ_FIREFLY_POSITION_4 0x0B
#define OBJ_FIREFLY_POSITION_1_SCANNED 0x0C
#define OBJ_FIREFLY_POSITION_2_SCANNED 0x0D
#define OBJ_FIREFLY_POSITION_3_SCANNED 0x0E
#define OBJ_FIREFLY_POSITION_4_SCANNED 0x0F
#define OBJ_BOULDER_STATIONARY 0x10
#define OBJ_BOULDER_STATIONARY_SCANNED 0x11
#define OBJ_BOULDER_FALLING 0x12
#define OBJ_BOULDER_FALLING_SCANNED 0x13
#define OBJ_DIAMOND_STATIONARY 0x14
#define OBJ_DIAMOND_STATIONARY_SCANNED 0x15
#define OBJ_DIAMOND_FALLING 0x16
#define OBJ_DIAMOND_FALLING_SCANNED 0x17
#define OBJ_EXPLODE_TO_SPACE_STAGE_0 0x1B
#define OBJ_EXPLODE_TO_SPACE_STAGE_1 0x1C
#define OBJ_EXPLODE_TO_SPACE_STAGE_2 0x1D
#define OBJ_EXPLODE_TO_SPACE_STAGE_3 0x1E
#define OBJ_EXPLODE_TO_SPACE_STAGE_4 0x1F
#define OBJ_EXPLODE_TO_DIAMOND_STAGE_0 0x20
#define OBJ_EXPLODE_TO_DIAMOND_STAGE_1 0x21
#define OBJ_EXPLODE_TO_DIAMOND_STAGE_2 0x22
#define OBJ_EXPLODE_TO_DIAMOND_STAGE_3 0x23
#define OBJ_EXPLODE_TO_DIAMOND_STAGE_4 0x24
#define OBJ_PRE_ROCKFORD_STAGE_1 0x25
#define OBJ_PRE_ROCKFORD_STAGE_2 0x26
#define OBJ_PRE_ROCKFORD_STAGE_3 0x27
#define OBJ_PRE_ROCKFORD_STAGE_4 0x28
#define OBJ_BUTTERFLY_POSITION_1 0x30
#define OBJ_BUTTERFLY_POSITION_2 0x31
#define OBJ_BUTTERFLY_POSITION_3 0x32
#define OBJ_BUTTERFLY_POSITION_4 0x33
#define OBJ_BUTTERFLY_POSITION_1_SCANNED 0x34
#define OBJ_BUTTERFLY_POSITION_2_SCANNED 0x35
#define OBJ_BUTTERFLY_POSITION_3_SCANNED 0x36
#define OBJ_BUTTERFLY_POSITION_4_SCANNED 0x37
#define OBJ_ROCKFORD 0x38
#define OBJ_ROCKFORD_SCANNED 0x39
#define OBJ_AMOEBA 0x3A
#define OBJ_AMOEBA_SCANNED 0x3B

typedef struct {
  uint8_t caveNumber;
  uint8_t magicWallMillingTime; // also max amoeba time at 3% growth
  uint8_t initialDiamondValue;
  uint8_t extraDiamondValue;
  uint8_t randomiserSeed[NUM_DIFFICULTY_LEVELS];
  uint8_t diamondsNeeded[NUM_DIFFICULTY_LEVELS];
  uint8_t caveTime[NUM_DIFFICULTY_LEVELS];
  uint8_t backgroundColor1;
  uint8_t backgroundColor2;
  uint8_t foregroundColor;
  uint8_t unused[2];
  uint8_t randomObject[NUM_RANDOM_OBJECTS];
  uint8_t objectProbability[NUM_RANDOM_OBJECTS];
} CaveInfo;

typedef uint8_t CaveMap[CAVE_HEIGHT][CAVE_WIDTH];

typedef struct {
  CaveInfo info;
  CaveMap map;
} Cave;

//
// Global variables
//

uint8_t *gBackbuffer;
BITMAPINFO *gBitmapInfo;
HDC gDeviceContext;

//
//
//

void debugPrint(char *format, ...) {
  va_list argptr;
  va_start(argptr, format);
  char str[1024];
  vsprintf_s(str, sizeof(str), format, argptr);
  va_end(argptr);
  OutputDebugString(str);
}

void setPixel(int x, int y, uint8_t color) {
  assert(color < PALETTE_COLORS);
  assert((color & 0xF0) == 0);

  int pixelOffset = y*BACKBUFFER_WIDTH + x;
  int byteOffset = pixelOffset / 2;

  assert(byteOffset >= 0 && byteOffset < BACKBUFFER_BYTES);

  uint8_t oldColor = gBackbuffer[byteOffset];
  uint8_t newColor;
  if (pixelOffset % 2 == 0) {
    newColor = (color << 4) | (oldColor & 0x0F);
  }
  else {
    newColor = (oldColor & 0xF0) | color;
  }
  gBackbuffer[byteOffset] = newColor;
}

void drawFilledRect(int left, int top, int right, int bottom, uint8_t color) {
  for (int y = top; y <= bottom; ++y) {
    for (int x = left; x <= right; ++x) {
      setPixel(x, y, color);
    }
  }
}

void drawSprite8x8(uint8_t *sprite, int row, int col, uint8_t fgColor, uint8_t bgColor, int vOffset) {
  for (uint8_t bmpY = 0; bmpY < 8; ++bmpY) {
    int y = row*8 + bmpY;
    uint8_t byte = sprite[(bmpY + vOffset) % 8];

    for (uint8_t bmpX = 0; bmpX < 8; ++bmpX) {
      int x = col*8 + bmpX;
      uint8_t mask = 1 << (7 - bmpX);
      uint8_t bit = byte & mask;
      uint8_t color = bit ? fgColor : bgColor;

      setPixel(x, y, color);
    }
  }
}

void drawSprite(uint8_t *sprite, int outRow, int outCol, int frame, uint8_t fgColor, uint8_t bgColor, int vOffset) {
  int frames = sprite[0];
  int height = sprite[1];
  int width = sprite[2];
  int bytesPerFrame = width*height*8;
  int bytesPerRow = width*8;
  for (int row = 0; row < height; ++row) {
    for (int col = 0; col < width; ++col) {
      uint8_t *data = sprite + 3 + (frame%frames)*bytesPerFrame + row*bytesPerRow + col*8;
      drawSprite8x8(data, outRow+row, outCol+col, fgColor, bgColor, vOffset);
    }
  }
}

void nextRandom(int *randSeed1, int *randSeed2) {
  int tempRand1 = (*randSeed1 & 0x0001) * 0x0080;
  int tempRand2 = (*randSeed2 >> 1) & 0x007F;

  int result = (*randSeed2) + (*randSeed2 & 0x0001) * 0x0080;
  int carry = (result > 0x00FF);
  result = result & 0x00FF;

  result = result + carry + 0x13;
  carry = (result > 0x00FF);
  *randSeed2 = result & 0x00FF;

  result = *randSeed1 + carry + tempRand1;
  carry = (result > 0x00FF);
  result = result & 0x00FF;

  result = result + carry + tempRand2;
  *randSeed1 = result & 0x00FF;
}

void placeObjectLine(CaveMap map, uint8_t object, int row, int col, int length, int direction) {
  int ldx[8] = { 0,  1, 1, 1, 0, -1, -1, -1 };
  int ldy[8] = { -1, -1, 0, 1, 1,  1,  0, -1 };

  for (int i = 0; i < length; i++) {
    map[row + i*ldy[direction]][col + i*ldx[direction]] = object;
  }
}

void placeObjectFilledRect(CaveMap map, uint8_t object, int row, int col, int width, int height, uint8_t fillObject) {
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      if ((j == 0) || (j == height - 1)) {
        map[row+j][col+i] = object;
      }
      else {
        map[row+j][col+i] = fillObject;
      }
    }
    map[row][col+i] = object;
    map[row+height-1][col+i] = object;
  }
}

void placeObjectRect(CaveMap map, uint8_t object, int row, int col, int width, int height) {
  for (int i = 0; i < width; i++) {
    map[row][col+i] = object;
    map[row+height-1][col+i] = object;
  }
  for (int i = 0; i < height; i++) {
    map[row+i][col] = object;
    map[row+i][col+width-1] = object;
  }
}

Cave decodeCave(uint8_t caveIndex) {
  uint8_t *caves[] = {
    gCave1, gCave2, gCave3, gCave4, gCave5, gCave6, gCave7, gCave8, gCave9, gCave10,
    gCave11, gCave12, gCave13, gCave14, gCave15, gCave16, gCave17, gCave18, gCave19, gCave20
  };

  assert(caveIndex < ARRAY_LENGTH(caves));

  Cave cave;
  CopyMemory(&cave.info, caves[caveIndex], sizeof(cave.info));

  // Clear out the map
  for (int y = 0; y < CAVE_HEIGHT; y++) {
    for (int x = 0; x < CAVE_WIDTH; x++) {
      cave.map[y][x] = OBJ_STEEL_WALL;
    }
  }

  // Decode random map objects
  {
    int randSeed1 = 0;
    int randSeed2 = cave.info.randomiserSeed[0];

    for (int y = 1; y < CAVE_HEIGHT; y++) {
      for (int x = 0; x < CAVE_WIDTH; x++) {
        uint8_t object = OBJ_DIRT;
        nextRandom(&randSeed1, &randSeed2);
        for (int i = 0; i < NUM_RANDOM_OBJECTS; i++) {
          if (randSeed1 < cave.info.objectProbability[i]) {
            object = cave.info.randomObject[i];
          }
        }
        cave.map[y][x] = object;
      }
    }
  }

  // Decode explicit map data
  {
    uint8_t *explicitData = caves[caveIndex] + sizeof(CaveInfo);
    int uselessTopBorderHeight = 2;

    for (int i = 0; explicitData[i] != 0xFF; i++) {
      uint8_t object = (explicitData[i] & 0x3F);

      switch (3 & (explicitData[i] >> 6)) {
        case 0: {
          int x = explicitData[++i];
          int y = explicitData[++i] - uselessTopBorderHeight;
          cave.map[y][x] = object;
          break;
        }
        case 1: {
          int x = explicitData[++i];
          int y = explicitData[++i] - uselessTopBorderHeight;
          int length = explicitData[++i];
          int direction = explicitData[++i];
          placeObjectLine(cave.map, object, y, x, length, direction);
          break;
        }
        case 2: {
          int x = explicitData[++i];
          int y = explicitData[++i] - uselessTopBorderHeight;
          int width = explicitData[++i];
          int height = explicitData[++i];
          uint8_t fill = explicitData[++i];
          placeObjectFilledRect(cave.map, object, y, x, width, height, fill);
          break;
        }
        case 3: {
          int x = explicitData[++i];
          int y = explicitData[++i] - uselessTopBorderHeight;
          int width = explicitData[++i];
          int height = explicitData[++i];
          placeObjectRect(cave.map, object, y, x, width, height);
          break;
        }
      }
    }
  }

  // Steel bounds
  placeObjectRect(cave.map, OBJ_STEEL_WALL, 0, 0, CAVE_WIDTH, CAVE_HEIGHT);

  return cave;
}

bool isCellVisible(int cellRow, int cellCol) {
  return
    cellRow >= 0 && cellRow < PLAYFIELD_HEIGHT_IN_CELLS &&
    cellCol >= 0 && cellCol < PLAYFIELD_WIDTH_IN_CELLS;
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

  WNDCLASS wndClass = {0};
  wndClass.style = CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc = wndProc;
  wndClass.hInstance = inst;
  wndClass.hCursor = LoadCursor(0, IDC_ARROW);
  wndClass.lpszClassName = "Boulder Dash";
  RegisterClass(&wndClass);

  int windowScale = 3;
  int windowWidth = BACKBUFFER_WIDTH * windowScale;
  int windowHeight = BACKBUFFER_HEIGHT * windowScale;

  RECT crect = {0};
  crect.right = windowWidth;
  crect.bottom = windowHeight;

  DWORD wndStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
  AdjustWindowRect(&crect, wndStyle, 0);

  HWND wnd = CreateWindowEx(0, wndClass.lpszClassName, "Boulder Dash", wndStyle, 300, 100,
                            crect.right - crect.left, crect.bottom - crect.top,
                            0, 0, inst, 0);
  ShowWindow(wnd, cmdShow);
  UpdateWindow(wnd);

  // Initialize graphics
  {
    HDC deviceContext = GetDC(wnd);

    gDeviceContext = deviceContext;
    gBackbuffer = malloc(BACKBUFFER_PIXELS);

    gBitmapInfo = malloc(sizeof(BITMAPINFOHEADER) + (PALETTE_COLORS * sizeof(RGBQUAD)));
    gBitmapInfo->bmiHeader.biSize = sizeof(gBitmapInfo->bmiHeader);
    gBitmapInfo->bmiHeader.biWidth = BACKBUFFER_WIDTH;
    gBitmapInfo->bmiHeader.biHeight = -BACKBUFFER_HEIGHT;
    gBitmapInfo->bmiHeader.biPlanes = 1;
    gBitmapInfo->bmiHeader.biBitCount = 4;
    gBitmapInfo->bmiHeader.biCompression = BI_RGB;
    gBitmapInfo->bmiHeader.biClrUsed = PALETTE_COLORS;

    RGBQUAD black  = {0x00, 0x00, 0x00, 0x00};
    RGBQUAD red    = {0x00, 0x00, 0xCC, 0x00};
    //RGBQUAD green  = {0x00, 0xCC, 0x00, 0x00};
    RGBQUAD yellow = {0x00, 0xCC, 0xCC, 0x00};
    //RGBQUAD blue   = {0xCC, 0x00, 0x00, 0x00};
    //RGBQUAD purple = {0xCC, 0x00, 0xCC, 0x00};
    //RGBQUAD cyan   = {0xCC, 0xCC, 0x00, 0x00};
    RGBQUAD gray   = {0xCC, 0xCC, 0xCC, 0x00};
    RGBQUAD white  = {0xFF, 0xFF, 0xFF, 0x00};

    gBitmapInfo->bmiColors[0] = black;
    gBitmapInfo->bmiColors[1] = gray;
    gBitmapInfo->bmiColors[2] = white;
    gBitmapInfo->bmiColors[3] = red;
    gBitmapInfo->bmiColors[4] = yellow;
  }

  float dt = 0.0f;
  float targetFps = 60.0f;
  float maxDt = 1.0f / targetFps;
  LARGE_INTEGER perfcFreq = {0};
  LARGE_INTEGER perfc = {0};
  LARGE_INTEGER perfcPrev = {0};

  QueryPerformanceFrequency(&perfcFreq);
  QueryPerformanceCounter(&perfc);

  //
  // Initialize game state
  //

  uint8_t caveNumber = 0;
  Cave cave = decodeCave(caveNumber);
  CaveMap mapCover;
  int difficultyLevel = 0;
  int caveTimeLeft = cave.info.caveTime[difficultyLevel];
  int livesLeft = 3;
  int score = 0;
  int diamondsCollected = 0;
  int turn = 0;
  float turnTimer = 0;
  int rockfordTurnsTillBirth = ROCKFORD_TURNS_TILL_BIRTH;
  int mapUncoverTurnsLeft = MAP_UNCOVER_TURNS;
  int pauseTurnsLeft = 0;

  for (int y = 0; y < CAVE_HEIGHT; ++y) {
    for (int x = 0; x < CAVE_WIDTH; ++x) {
      mapCover[y][x] = OBJ_STEEL_WALL;
    }
  }

  //
  // Game loop
  //

  bool running = true;

  while (running) {
    perfcPrev = perfc;
    QueryPerformanceCounter(&perfc);
    dt = (float)(perfc.QuadPart - perfcPrev.QuadPart) / (float)perfcFreq.QuadPart;
    //debugPrint("dt: %f\n", dt);
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

    //
    // Update
    //

    turnTimer += dt;
    if (turnTimer >= TURN_DURATION) {
      turnTimer -= TURN_DURATION;

      // Do cave turn

      if (pauseTurnsLeft > 0) {
        pauseTurnsLeft--;
      }
      else {
        turn++;

        // Scan cave
        for (int y = 0; y < CAVE_HEIGHT; ++y) {
          for (int x = 0; x < CAVE_WIDTH; ++x) {
            switch (cave.map[y][x]) {
              case OBJ_PRE_ROCKFORD_STAGE_1:
                if (rockfordTurnsTillBirth == 0) {
                  cave.map[y][x] = OBJ_PRE_ROCKFORD_STAGE_2;
                }
                else if (mapUncoverTurnsLeft == 0) {
                  rockfordTurnsTillBirth--;
                }
                break;

              case OBJ_PRE_ROCKFORD_STAGE_2:
                cave.map[y][x] = OBJ_PRE_ROCKFORD_STAGE_3;
                break;

              case OBJ_PRE_ROCKFORD_STAGE_3:
                cave.map[y][x] = OBJ_PRE_ROCKFORD_STAGE_4;
                break;

              case OBJ_PRE_ROCKFORD_STAGE_4:
                cave.map[y][x] = OBJ_ROCKFORD;
                break;
            }
          }
        }

        // Update map cover
        if (mapUncoverTurnsLeft > 0) {
          mapUncoverTurnsLeft--;
          if (mapUncoverTurnsLeft > 1) {
            for (int y = 0; y < CAVE_HEIGHT; ++y) {
              for (int i = 0; i < CELLS_PER_LINE_TO_UNCOVER; ++i) {
                mapCover[y][rand()%CAVE_WIDTH] = OBJ_SPACE;
              }
            }
          }
          else if (mapUncoverTurnsLeft == 1) {
            pauseTurnsLeft = PAUSE_TURNS_BEFORE_FULL_UNCOVER;
          }
          else if (mapUncoverTurnsLeft == 0) {
            for (int y = 0; y < CAVE_HEIGHT; ++y) {
              for (int x = 0; x < CAVE_WIDTH; ++x) {
                mapCover[y][x] = OBJ_SPACE;
              }
            }
          }
        }
      }
    }

    //
    // Render
    //

    // Draw border
    drawFilledRect(0, 0, BACKBUFFER_WIDTH - 1, BACKBUFFER_HEIGHT - 1, 0);

    // Draw cave
    for (int y = 0; y < CAVE_HEIGHT; ++y) {
      for (int x = 0; x < CAVE_WIDTH; ++x) {
        if (!isCellVisible(y, x)) {
          continue;
        }
        int outRow = (PLAYFIELD_Y_MIN_IN_CELLS + y)*2;
        int outCol = (PLAYFIELD_X_MIN_IN_CELLS + x)*2;
        switch (cave.map[y][x]) {
          case OBJ_SPACE:
            drawSprite(spriteSpace, outRow, outCol, 0, 0, 0, 0);
            break;
          case OBJ_STEEL_WALL:
            drawSprite(spriteSteelWall, outRow, outCol, 0, 4, 0, 0);
            break;
          case OBJ_DIRT:
            drawSprite(spriteDirt, outRow, outCol, 0, 3, 0, 0);
            break;
          case OBJ_BRICK_WALL:
            drawSprite(spriteBrickWall, outRow, outCol, 0, 1, 3, 0);
            break;
          case OBJ_BOULDER_STATIONARY:
          case OBJ_BOULDER_FALLING:
            drawSprite(spriteBoulder, outRow, outCol, 0, 4, 0, 0);
            break;
          case OBJ_DIAMOND_STATIONARY:
          case OBJ_DIAMOND_FALLING:
            drawSprite(spriteDiamond, outRow, outCol, 0, 2, 0, 0);
            break;
          case OBJ_PRE_ROCKFORD_STAGE_1:
            if (rockfordTurnsTillBirth > 0) {
              if (rockfordTurnsTillBirth % 2) {
                drawSprite(spriteSteelWall, outRow, outCol, 0, 4, 0, 0);
              }
              else {
                drawSprite(spriteOutbox, outRow, outCol, 0, 4, 0, 0);
              }
            }
            else {
              drawSprite(spriteExplosion, outRow, outCol, 0, 2, 0, 0);
            }
            break;
          case OBJ_PRE_ROCKFORD_STAGE_2:
            drawSprite(spriteExplosion, outRow, outCol, 1, 2, 0, 0);
            break;
          case OBJ_PRE_ROCKFORD_STAGE_3:
            drawSprite(spriteExplosion, outRow, outCol, 2, 2, 0, 0);
            break;
          case OBJ_PRE_ROCKFORD_STAGE_4:
            drawSprite(spriteRockfordMoveRight, outRow, outCol, turn, 1, 0, 0);
            break;
          case OBJ_ROCKFORD:
            drawSprite(spriteRockfordIdleHead, outRow, outCol, 0, 1, 0, 0);
            drawSprite(spriteRockfordIdleBody, outRow+1, outCol, 0, 1, 0, 0);
            break;
        }
      }
    }

    // Draw map cover
    for (int y = 0; y < CAVE_HEIGHT; ++y) {
      for (int x = 0; x < CAVE_WIDTH; ++x) {
        if (mapCover[y][x] == OBJ_SPACE || !isCellVisible(y, x)) {
          continue;
        }
        drawSprite(spriteSteelWall, (PLAYFIELD_Y_MIN_IN_CELLS + y)*2, (PLAYFIELD_X_MIN_IN_CELLS + x)*2, 0, 4, 0, turn);
      }
    }

    // Draw text
    {
      char text[64];
      if (rockfordTurnsTillBirth > 0) {
        sprintf_s(text, sizeof(text), "  PLAYER 1,  %d MEN,  ROOM %c/1", livesLeft, 'A' + caveNumber);
      }
      else {
        sprintf_s(text, sizeof(text), "   %d*%d   %02d   %03d   %06d",
                  cave.info.diamondsNeeded[difficultyLevel],
                  cave.info.initialDiamondValue,
                  diamondsCollected,
                  caveTimeLeft,
                  score);
      }
      for (int i = 0; text[i]; ++i) {
        drawSprite(spriteAscii, 3, 2 + i, text[i]-' ', 1, 0, 0);
      }
    }

    // Display backbuffer
    StretchDIBits(gDeviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT,
                  gBackbuffer, gBitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
  }

  return 0;
}
