#include <windows.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "cave.h"
#include "graphics.h"

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(*array))
#define PI 3.14159265358979323846f

#define TEXT_AREA_HEIGHT TILE_SIZE
#define HERO_ANIM_FRAME_DURATION 0.05f
#define HERO_ANIM_FRAME_COUNT 6
#define TURN_DURATION 0.15f
#define MAX_MAP_TILES 100*100
#define GEM_FRAME_COUNT 8
#define MAX_SCORE_DIGITS 6
#define SCREEN_WIDTH_IN_TILES (BACKBUFFER_WIDTH / TILE_SIZE)
#define SCREEN_HEIGHT_IN_TILES ((BACKBUFFER_HEIGHT - TEXT_AREA_HEIGHT) / TILE_SIZE)

#define SCREEN_WIDTH_IN_HALF_TILES (SCREEN_WIDTH_IN_TILES*2)
#define SCREEN_HEIGHT_IN_HALF_TILES (SCREEN_HEIGHT_IN_TILES*2)
#define SCREEN_HALF_TILES_COUNT (SCREEN_WIDTH_IN_HALF_TILES*SCREEN_HEIGHT_IN_HALF_TILES)

#define CAMERA_STEP HALF_TILE_SIZE
#define HERO_SIZE TILE_SIZE

#define CAMERA_START_RIGHT_HERO_X (BACKBUFFER_WIDTH - 4*HALF_TILE_SIZE - HERO_SIZE)
#define CAMERA_STOP_RIGHT_HERO_X (BACKBUFFER_WIDTH - 13*HALF_TILE_SIZE - HERO_SIZE)
#define CAMERA_START_LEFT_HERO_X 4*HALF_TILE_SIZE
#define CAMERA_STOP_LEFT_HERO_X 14*HALF_TILE_SIZE

#define CAMERA_START_DOWN_HERO_Y BACKBUFFER_HEIGHT - 3*HALF_TILE_SIZE - HERO_SIZE - TEXT_AREA_HEIGHT
#define CAMERA_STOP_DOWN_HERO_Y BACKBUFFER_HEIGHT - 9*HALF_TILE_SIZE - HERO_SIZE - TEXT_AREA_HEIGHT
#define CAMERA_START_UP_HERO_Y 3*HALF_TILE_SIZE
#define CAMERA_STOP_UP_HERO_Y 9*HALF_TILE_SIZE

typedef enum {
  TILE_TYPE_EMPTY,
  TILE_TYPE_HERO,
  TILE_TYPE_ROCK,
  TILE_TYPE_WALL,
  TILE_TYPE_BRICK,
  TILE_TYPE_EARTH,
  TILE_TYPE_EXPLOSION,
  TILE_TYPE_GEM,
  TILE_TYPE_COUNT,
} TileType;

typedef struct {
  TileType type;
  bool moved;
  bool movedInPreviousFrame;
} Tile;

// Sprite index = row*16 + col
typedef enum {
  SPRITE_TYPE_ENTRANCE = 20,
  SPRITE_TYPE_WALL = 16,
  SPRITE_TYPE_EXPLOSION_1 = 34,
  SPRITE_TYPE_EXPLOSION_2 = 32,
  SPRITE_TYPE_EXPLOSION_3 = 33,
  SPRITE_TYPE_HERO_RUN_1 = 7,
  SPRITE_TYPE_HERO_IDLE_1 = 0,
} SpriteType;

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

  initGraphics();

  Cave cave = getCave(1);

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
  LARGE_INTEGER prefcPrev = {0};

  QueryPerformanceFrequency(&perfcFreq);
  QueryPerformanceCounter(&perfc);

  bool running = true;
  HDC deviceContext = GetDC(wnd);

  int cameraX = 0;
  int cameraY = 0;
  float turnTimer = 0;
  bool rightIsDown = false;
  bool leftIsDown = false;
  bool upIsDown = false;
  bool downIsDown = false;
  int heroRow = 0;
  int heroCol = 0;
  int heroMoveRockTurns = 0;
  int heroMoveFrame = 0;
  float heroAnimTimer = 0;
  bool heroIsAppearing = true;
  bool heroIsMoving = false;
  bool heroIsFacingRight = false;
  bool heroIsFacingRightOld = false;
  float heroIdleTimer = 0;
  int heroIdleFrame = 0;
  float nextIdleAnimTimer = 0;
  int currentIdleAnimation = 0;
  bool idleAnimOnce = false;
  bool heroIsAlive = true;
  bool explosionIsActive = false;
  int explosionFrame = 0;
  int explosionAnim[] = {0,1,0,2};
  int gemFrame = 0;
  int score = 0;

  int foregroundVisibilityTurnMax = 28;
  int foregroundVisibilityTurn = foregroundVisibilityTurnMax;
  int foregroundOffset = 0;
  int foregroundTilesPerTurn = 6;

  int deathForegroundVisibilityTurn = 0;
  int deathForegroundVisibilityTurnMax = 25;
  int deathForegroundOffset = 0;
  int deathForegroundTilesPerTurn = 18;

#define IDLE_ANIMATIONS_COUNT 4
  int idleAnim1[] = {0,1,2,1};
  int idleAnim2[] = {3,4};
  int idleAnim3[] = {0};
  int idleAnim4[] = {4,6,5,3};
  int *currentIdleAnim = 0;
  int currentIdleAnimNumFrames = 0;
  float currentIdleAnimFrameDuration = 0;

  SpriteType appearanceAnim[] = {
    SPRITE_TYPE_ENTRANCE,
    SPRITE_TYPE_WALL,
    SPRITE_TYPE_ENTRANCE,
    SPRITE_TYPE_WALL,
    SPRITE_TYPE_ENTRANCE,
    SPRITE_TYPE_WALL,
    SPRITE_TYPE_ENTRANCE,
    SPRITE_TYPE_WALL,
    SPRITE_TYPE_ENTRANCE,
    SPRITE_TYPE_WALL,
    SPRITE_TYPE_ENTRANCE,
    SPRITE_TYPE_WALL,
    SPRITE_TYPE_EXPLOSION_1,
    SPRITE_TYPE_EXPLOSION_2,
    SPRITE_TYPE_EXPLOSION_3,
    SPRITE_TYPE_HERO_RUN_1,
    SPRITE_TYPE_HERO_IDLE_1
  };
  int appearanceAnimFrame = 0;

  Tile map[MAX_MAP_TILES];
  bool foreground[MAX_MAP_TILES];
  bool deathForeground[SCREEN_HALF_TILES_COUNT];
  bool isInit = true;

  int cameraVelX = 0;
  int cameraVelY = 0;
  int maxCameraX = CAVE_WIDTH*TILE_SIZE - BACKBUFFER_WIDTH;
  int maxCameraY = CAVE_HEIGHT*TILE_SIZE - BACKBUFFER_HEIGHT + TEXT_AREA_HEIGHT;

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

    if (isInit) {
      isInit = false;

      heroIsAppearing = true;
      heroIsAlive = true;
      foregroundVisibilityTurn = foregroundVisibilityTurnMax;
      foregroundOffset = 0;
      deathForegroundVisibilityTurn = 0;
      deathForegroundOffset = 0;

      for (int y = 2, i = 0; y <= 23; y++) {
        for(int x = 0; x <= 39; x++, i++) {
          switch (cave.data[y][x]) {
            case 'W':
              map[i].type = TILE_TYPE_WALL;
              break;
            case '.':
              map[i].type = TILE_TYPE_EARTH;
              break;
            case 'r':
              map[i].type = TILE_TYPE_ROCK;
              break;
            case 'X':
              map[i].type = TILE_TYPE_HERO;
              break;
            case 'w':
              map[i].type = TILE_TYPE_BRICK;
              break;
            case 'd':
              map[i].type = TILE_TYPE_GEM;
              break;
            case ' ':
            case 'P':
            case 'q':
              map[i].type = TILE_TYPE_EMPTY;
              break;
            default:
              assert(!"Unhandled type!");
          }
          if (map[i].type == TILE_TYPE_HERO) {
            heroRow = y - 2;
            heroCol = x;
          }
        }
      }

      for (int i = 0; i < CAVE_WIDTH * CAVE_HEIGHT; ++i) {
        foreground[i] = true;
      }

      for (int i = 0; i < SCREEN_HALF_TILES_COUNT; ++i) {
        deathForeground[i] = false;
      }
    }

    heroIsFacingRightOld = heroIsFacingRight;

    if (rightIsDown || leftIsDown || upIsDown || downIsDown) {
      heroAnimTimer += dt;
      heroIsMoving = true;

      if (rightIsDown) {
        heroIsFacingRight = true;
      } else if (leftIsDown) {
        heroIsFacingRight = false;
      }
    } else {
      heroAnimTimer = 0;
      heroIsMoving = false;
    }

    if ((heroIsFacingRightOld != heroIsFacingRight) || !heroIsMoving) {
      heroMoveRockTurns = 0;
    }

    heroMoveFrame = (int)(heroAnimTimer / HERO_ANIM_FRAME_DURATION) % HERO_ANIM_FRAME_COUNT;

    // Idle animation
    if (!heroIsMoving) {
      nextIdleAnimTimer -= dt;

      heroIdleTimer += dt;
      if (heroIdleTimer > currentIdleAnimFrameDuration*currentIdleAnimNumFrames) {
        heroIdleTimer = 0;
        if (idleAnimOnce) {
          nextIdleAnimTimer = 0;
        }
      }

      if (nextIdleAnimTimer <= 0) {
        heroIdleTimer = 0;
        currentIdleAnimation = rand() % IDLE_ANIMATIONS_COUNT;
        switch (currentIdleAnimation) {
          case 0:
            nextIdleAnimTimer = (float)(rand()%1000 + 500) / 1000.0f;
            break;
          case 1:
            nextIdleAnimTimer = (float)(rand()%2000 + 1000) / 1000.0f;
            break;
          case 2:
            nextIdleAnimTimer = (float)(rand()%2000 + 1000) / 1000.0f;
            break;
          case 3:
            nextIdleAnimTimer = (float)(rand()%2000 + 1000) / 1000.0f;
            break;
          default:
            assert('no!');
        }
      }

      switch (currentIdleAnimation) {
        case 0:
          currentIdleAnim = idleAnim1;
          currentIdleAnimNumFrames = ARRAY_LENGTH(idleAnim1);
          currentIdleAnimFrameDuration = 0.07f;
          idleAnimOnce = true;
          break;

        case 1:
          currentIdleAnim = idleAnim2;
          currentIdleAnimNumFrames = ARRAY_LENGTH(idleAnim2);
          currentIdleAnimFrameDuration = 0.1f;
          idleAnimOnce = false;
          break;

        case 2:
          currentIdleAnim = idleAnim3;
          currentIdleAnimNumFrames = ARRAY_LENGTH(idleAnim3);
          currentIdleAnimFrameDuration = 0.1f;
          idleAnimOnce = false;
          break;

        case 3:
          currentIdleAnim = idleAnim4;
          currentIdleAnimNumFrames = ARRAY_LENGTH(idleAnim4);
          currentIdleAnimFrameDuration = 0.07f;
          idleAnimOnce = true;
          break;

        default:
          assert('no!');
      }

      heroIdleFrame = currentIdleAnim[(int)(heroIdleTimer / currentIdleAnimFrameDuration)];
    } else {
      nextIdleAnimTimer = 0;
      heroIdleTimer = 0;
    }

    turnTimer += dt;
    if (turnTimer >= TURN_DURATION) {
      turnTimer -= TURN_DURATION;

      //
      // Do turn
      //

      gemFrame++;
      if (gemFrame == GEM_FRAME_COUNT) {
        gemFrame = 0;
      }


      if (explosionIsActive) {
        explosionFrame++;

        if (explosionFrame >= ARRAY_LENGTH(explosionAnim)) {
          explosionFrame = 0;
          explosionIsActive = false;
          deathForegroundVisibilityTurn = deathForegroundVisibilityTurnMax;
          for (int row = 0; row < CAVE_HEIGHT; ++row) {
            for (int col = 0; col < CAVE_WIDTH; ++col) {
              int i = row*CAVE_WIDTH + col;
              if (map[i].type == TILE_TYPE_EXPLOSION) {
                map[i].type = TILE_TYPE_EMPTY;
              }
            }
          }
        }
      }

      // Move camera
      {
        int heroScreenX = heroCol * TILE_SIZE - cameraX;
        int heroScreenY = heroRow * TILE_SIZE - cameraY;

        if (heroScreenX >= CAMERA_START_RIGHT_HERO_X) {
          cameraVelX = CAMERA_STEP;
        } else if (heroScreenX <= CAMERA_START_LEFT_HERO_X) {
          cameraVelX = -CAMERA_STEP;
        }
        if (heroScreenY >= CAMERA_START_DOWN_HERO_Y) {
          cameraVelY = CAMERA_STEP;
        } else if (heroScreenY <= CAMERA_START_UP_HERO_Y) {
          cameraVelY = -CAMERA_STEP;
        }

        if (heroScreenX >= CAMERA_STOP_LEFT_HERO_X && heroScreenX <= CAMERA_STOP_RIGHT_HERO_X) {
          cameraVelX = 0;
        }
        if (heroScreenY >= CAMERA_STOP_UP_HERO_Y && heroScreenY <= CAMERA_STOP_DOWN_HERO_Y) {
          cameraVelY = 0;
        }

        cameraX += cameraVelX;
        cameraY += cameraVelY;

        if (cameraX < 0) {
          cameraX = 0;
        } else if (cameraX > maxCameraX) {
          cameraX = maxCameraX;
        }

        if (cameraY < 0) {
          cameraY = 0;
        } else if (cameraY > maxCameraY) {
          cameraY = maxCameraY;
        }
      }

      if (foregroundVisibilityTurn > 0) {
        //
        // Foreground is active
        //

        foregroundVisibilityTurn--;

        foregroundOffset++;
        if (foregroundOffset == TILE_SIZE) {
          foregroundOffset = 0;
        }

        int cameraRow = cameraY / TILE_SIZE;
        int cameraCol = cameraX / TILE_SIZE;

        // Hide some foreground tiles every turn
        for (int tile = 0; tile < foregroundTilesPerTurn; ++tile) {
          for (int try = 0; try < 100; ++try) {
            int row = rand() % SCREEN_HEIGHT_IN_TILES + cameraRow;
            int col = rand() % SCREEN_WIDTH_IN_TILES + cameraCol;
            int cell = row*CAVE_WIDTH + col;
            if (foreground[cell]) {
              foreground[cell] = false;
              break;
            }
          }
        }
      } else {
        //
        // Foreground is not active
        //

        for (int row = 0; row < CAVE_HEIGHT; ++row) {
          for (int col = 0; col < CAVE_WIDTH; ++col) {
            int i = row*CAVE_WIDTH + col;
            map[i].movedInPreviousFrame = map[i].moved;
            map[i].moved = false;
          }
        }

        // Move entities (processing order: top to bottom, left to right).
        // Move only one tile per turn.
        for (int row = 0; row < CAVE_HEIGHT; ++row) {
          for (int col = 0; col < CAVE_WIDTH; ++col) {
            int current = row*CAVE_WIDTH + col;
            if (map[current].moved) {
              continue;
            }
            if (map[current].type == TILE_TYPE_ROCK || map[current].type == TILE_TYPE_GEM) {
              int below = (row+1)*CAVE_WIDTH + col;
              int left = row*CAVE_WIDTH + (col-1);
              int right = row*CAVE_WIDTH + (col+1);
              int belowLeft = (row+1)*CAVE_WIDTH + (col-1);
              int belowRight = (row+1)*CAVE_WIDTH + (col+1);
              if (map[below].type == TILE_TYPE_EMPTY) {
                map[below].type = map[current].type;
                map[below].moved = true;
                map[current].type = TILE_TYPE_EMPTY;
              } else if (map[below].type == TILE_TYPE_HERO) {
                if (map[current].movedInPreviousFrame) {
                  map[current].type = TILE_TYPE_EMPTY;
                  map[below].type = TILE_TYPE_EMPTY;
                  heroIsAlive = false;
                  explosionIsActive = true;

                  // center
                  map[(row+1)*CAVE_WIDTH + (col+0)].type = TILE_TYPE_EXPLOSION;
                  // right
                  map[(row+1)*CAVE_WIDTH + (col+1)].type = TILE_TYPE_EXPLOSION;
                  // left
                  map[(row+1)*CAVE_WIDTH + (col-1)].type = TILE_TYPE_EXPLOSION;
                  // above center
                  map[(row+0)*CAVE_WIDTH + (col+0)].type = TILE_TYPE_EXPLOSION;
                  // above right
                  map[(row+0)*CAVE_WIDTH + (col+1)].type = TILE_TYPE_EXPLOSION;
                  // above left
                  map[(row+0)*CAVE_WIDTH + (col-1)].type = TILE_TYPE_EXPLOSION;
                  // below center
                  map[(row+2)*CAVE_WIDTH + (col+0)].type = TILE_TYPE_EXPLOSION;
                  // below right
                  map[(row+2)*CAVE_WIDTH + (col+1)].type = TILE_TYPE_EXPLOSION;
                  // below left
                  map[(row+2)*CAVE_WIDTH + (col-1)].type = TILE_TYPE_EXPLOSION;
                }
              } else if (map[below].type == TILE_TYPE_ROCK || map[below].type == TILE_TYPE_GEM) {
                if (map[left].type == TILE_TYPE_EMPTY && map[belowLeft].type == TILE_TYPE_EMPTY) {
                  map[left].type = map[current].type;
                  map[left].moved = true;
                  map[current].type = TILE_TYPE_EMPTY;
                } else if (map[right].type == TILE_TYPE_EMPTY && map[belowRight].type == TILE_TYPE_EMPTY) {
                  map[right].type = map[current].type;
                  map[right].moved = true;
                  map[current].type = TILE_TYPE_EMPTY;
                }
              }
            }
          }
        }

        //
        // Update hero
        //

        if (heroIsAppearing) {
          appearanceAnimFrame++;
          if (appearanceAnimFrame == ARRAY_LENGTH(appearanceAnim)) {
            appearanceAnimFrame = 0;
            heroIsAppearing = false;
          }
        } else if (heroIsAlive) {
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

          int deltaCol = newCol - heroCol;
          int newCell = newRow*CAVE_WIDTH + newCol;

          switch (map[newCell].type) {
            case TILE_TYPE_EMPTY:
            case TILE_TYPE_EARTH:
            case TILE_TYPE_GEM:
              if (map[newCell].type == TILE_TYPE_GEM) {
                score += 10;
              }
              map[heroRow*CAVE_WIDTH + heroCol].type = TILE_TYPE_EMPTY;
              heroRow = newRow;
              heroCol = newCol;
              map[heroRow*CAVE_WIDTH + heroCol].type = TILE_TYPE_HERO;
              break;

            case TILE_TYPE_ROCK:
              int targetRockCell = newRow*CAVE_WIDTH + newCol + deltaCol;
              if (deltaCol != 0 && map[targetRockCell].type == TILE_TYPE_EMPTY) {
                heroMoveRockTurns++;
                if (heroMoveRockTurns == 3) {
                  heroMoveRockTurns = 0;
                  map[heroRow*CAVE_WIDTH + heroCol].type = TILE_TYPE_EMPTY;
                  heroRow = newRow;
                  heroCol = newCol;
                  map[heroRow*CAVE_WIDTH + heroCol].type = TILE_TYPE_HERO;
                  map[targetRockCell].type = TILE_TYPE_ROCK;
                }
              }
              break;
          }
        }
      }

      if (deathForegroundVisibilityTurn > 0) {
        deathForegroundVisibilityTurn--;
        if (deathForegroundVisibilityTurn == 0) {
          isInit = true;
        }

        deathForegroundOffset++;
        if (deathForegroundOffset == HALF_TILE_SIZE) {
          deathForegroundOffset = 0;
        }

        for (int tile = 0; tile < deathForegroundTilesPerTurn; ++tile) {
          for (int try = 0; try < 100; ++try) {
            int row = rand() % SCREEN_HEIGHT_IN_HALF_TILES;
            int col = rand() % SCREEN_WIDTH_IN_HALF_TILES;
            int cell = row*SCREEN_WIDTH_IN_HALF_TILES + col;
            if (!deathForeground[cell]) {
              deathForeground[cell] = true;
              break;
            }
          }
        }
      }
    }

    // Clear back buffer
    memset(backbuffer, 0, BACKBUFFER_BYTES);

    //
    // Draw tiles
    //
    for (int row = 0; row < CAVE_HEIGHT; ++row) {
      for (int col = 0; col < CAVE_WIDTH; ++col) {
        TileType tile = map[row*CAVE_WIDTH + col].type;

        if (tile == TILE_TYPE_EMPTY) {
          continue;
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
            if (heroIsAppearing) {
              int atlCol = appearanceAnim[appearanceAnimFrame] % SPRITE_ATLAS_WIDTH_TILES;
              int atlRow = appearanceAnim[appearanceAnimFrame] / SPRITE_ATLAS_WIDTH_TILES;
              atlX = atlCol * TILE_SIZE;
              atlY = atlRow * TILE_SIZE;
            } else if (heroIsMoving) {
              atlX = 112 + heroMoveFrame * 16;
              atlY = 0;
              flipHorizontally = !heroIsFacingRight;
            } else {
              atlX = heroIdleFrame * 16;
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

          case TILE_TYPE_EXPLOSION:
            atlX = explosionAnim[explosionFrame]*16;
            atlY = 32;
            break;

          case TILE_TYPE_GEM:
            atlX = gemFrame*16;
            atlY = 48;
            break;
        }

        int bbX = col * TILE_SIZE - cameraX;
        int bbY = row * TILE_SIZE - cameraY + TEXT_AREA_HEIGHT;

        for (int y = 0; y < TILE_SIZE; ++y) {
          for (int x = 0; x < TILE_SIZE; ++x) {
            int srcX;
            if (flipHorizontally) {
              srcX = atlX + TILE_SIZE - x - 1;
            } else {
              srcX = atlX + x;
            }
            int srcY = atlY + y;
            int dstX = bbX + x;
            int dstY = bbY + y;
            if (dstX >= 0 && dstX < BACKBUFFER_WIDTH && dstY >= TEXT_AREA_HEIGHT && dstY < BACKBUFFER_HEIGHT) {
              backbuffer[dstY*BACKBUFFER_WIDTH + dstX] = spriteAtlas[srcY*SPRITE_ATLAS_WIDTH + srcX];
            }
          }
        }
      }
    }

    //
    // Draw foreground
    //

    if (foregroundVisibilityTurn > 0) {
      for (int row = 0; row < CAVE_HEIGHT; ++row) {
        for (int col = 0; col < CAVE_WIDTH; ++col) {
          bool isVisible = foreground[row*CAVE_WIDTH + col];

          if (!isVisible) {
            continue;
          }

          int srcMinY = 16; // Wall sprite Y
          int srcMaxY = srcMinY + TILE_SIZE - 1;

          int atlX = 0;
          int atlY = srcMinY + foregroundOffset;

          int bbX = col * TILE_SIZE - cameraX;
          int bbY = row * TILE_SIZE - cameraY + TEXT_AREA_HEIGHT;

          for (int y = 0; y < TILE_SIZE; ++y) {
            for (int x = 0; x < TILE_SIZE; ++x) {
              int srcX = atlX + x;
              int srcY = atlY + y;
              if (srcY > srcMaxY) {
                srcY = srcMinY + (srcY - srcMaxY) - 1;
              }
              int dstX = bbX + x;
              int dstY = bbY + y;
              if (dstX >= 0 && dstX < BACKBUFFER_WIDTH && dstY >= TEXT_AREA_HEIGHT && dstY < BACKBUFFER_HEIGHT) {
                backbuffer[dstY*BACKBUFFER_WIDTH + dstX] = spriteAtlas[srcY*SPRITE_ATLAS_WIDTH + srcX];
              }
            }
          }
        }
      }
    }

    if (deathForegroundVisibilityTurn > 0) {
      for (int row = 0; row < SCREEN_HEIGHT_IN_HALF_TILES; ++row) {
        for (int col = 0; col < SCREEN_WIDTH_IN_HALF_TILES; ++col) {
          bool isVisible = deathForeground[row*SCREEN_WIDTH_IN_HALF_TILES + col];

          if (!isVisible) {
            continue;
          }

          int srcMinY = 16; // Wall sprite Y
          int srcMaxY = srcMinY + HALF_TILE_SIZE - 1;

          int atlX = 0;
          int atlY = srcMinY + deathForegroundOffset;

          int bbX = col * HALF_TILE_SIZE;
          int bbY = row * HALF_TILE_SIZE + TEXT_AREA_HEIGHT;

          for (int y = 0; y < HALF_TILE_SIZE; ++y) {
            for (int x = 0; x < HALF_TILE_SIZE; ++x) {
              int srcX = atlX + x;
              int srcY = atlY + y;
              if (srcY > srcMaxY) {
                srcY = srcMinY + (srcY - srcMaxY) - 1;
              }
              int dstX = bbX + x;
              int dstY = bbY + y;
              if (dstX >= 0 && dstX < BACKBUFFER_WIDTH && dstY >= TEXT_AREA_HEIGHT && dstY < BACKBUFFER_HEIGHT) {
                backbuffer[dstY*BACKBUFFER_WIDTH + dstX] = spriteAtlas[srcY*SPRITE_ATLAS_WIDTH + srcX];
              }
            }
          }
        }
      }
    }

    {
      static char text[MAX_SCORE_DIGITS + 1];
      sprintf_s(text, sizeof(text), "%0*d", MAX_SCORE_DIGITS, score);
      drawText(text, 1, 3);
    }

    StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight,
                  0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT, backbuffer,
                  &backbufferBmpInf, DIB_RGB_COLORS, SRCCOPY);
  }
}
