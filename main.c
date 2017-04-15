#include <windows.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(*array))
#define PI 3.14159265358979323846f

#define TILE_SIZE 16
#define HALF_TILE_SIZE TILE_SIZE/2
#define TILE_WIDTH TILE_SIZE
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
#define EXPLOSION_FRAME_DURATION 0.1f
#define GEM_FRAME_DURATION 0.2f
#define GEM_FRAME_COUNT 8
#define SCREEN_WIDTH_IN_TILES BACKBUFFER_WIDTH / TILE_SIZE
#define SCREEN_HEIGHT_IN_TILES BACKBUFFER_HEIGHT / TILE_SIZE

#define CAMERA_STEP HALF_TILE_SIZE
#define HERO_SIZE TILE_SIZE

#define CAMERA_START_RIGHT_HERO_X BACKBUFFER_WIDTH - 4*HALF_TILE_SIZE - HERO_SIZE
#define CAMERA_STOP_RIGHT_HERO_X BACKBUFFER_WIDTH - 13*HALF_TILE_SIZE - HERO_SIZE
#define CAMERA_START_LEFT_HERO_X 4*HALF_TILE_SIZE
#define CAMERA_STOP_LEFT_HERO_X 14*HALF_TILE_SIZE

#define CAMERA_START_DOWN_HERO_Y BACKBUFFER_HEIGHT - 3*HALF_TILE_SIZE - HERO_SIZE
#define CAMERA_STOP_DOWN_HERO_Y BACKBUFFER_HEIGHT - 9*HALF_TILE_SIZE - HERO_SIZE
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
  "###############################################"
  "#$$   oRo o  .................................#"
  "#$$.......o...................................#"
  "#...  $$  $  .................................#"
  "#...  oo     .................................#"
  "#.....ooo    .................................#"
  "#......oo    .................................#"
  "#......oo    .................................#"
  "#......oo   ..................................#"
  "#.......oo  ..................................#"
  "#........o....................................#"
  "#.......oo  ..................................#"
  "#........o....................................#"
  "#.......oo  ..................................#"
  "#........o....................................#"
  "#.......oo  ..................................#"
  "#........o....................................#"
  "#.......oo  ..................................#"
  "#........o....................................#"
  "#.......oo  ..................................#"
  "#........o....................................#"
  "###############################################";

int CALLBACK WinMain(HINSTANCE inst, HINSTANCE prevInst, LPSTR cmdLine, int cmdShow) {
  uint32_t *backbuffer = malloc(BACKBUFFER_BYTES);

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

  uint32_t *spriteAtlas = malloc(imageWidth * imageHeight * sizeof(*spriteAtlas));

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
  bool heroIsMoving = false;
  bool heroIsFacingRight = false;
  bool heroIsFacingRightOld = false;
  bool heroIsRunning = false;
  float heroIdleTimer = 0;
  int heroIdleFrame = 0;
  float nextIdleAnimTimer = 0;
  int currentIdleAnimation = 0;
  bool idleAnimOnce = false;
  bool heroIsAlive = true;
  float explosionFrameTimer = 0;
  bool explosionIsActive = false;
  int explosionFrame = 0;
  int explosionAnim[] = {0,1,0,2};
  int gemFrame = 0;
  float gemTimer = 0;

  float backgroundOffsetTimer = 0;
  float backgroundOffsetDuration = 0.1f;
  int backgroundOffset = 0;

#define IDLE_ANIMATIONS_COUNT 4
  int idleAnim1[] = {0,1,2,1};
  int idleAnim2[] = {3,4};
  int idleAnim3[] = {0};
  int idleAnim4[] = {4,6,5,3};
  int *currentIdleAnim = 0;
  int currentIdleAnimNumFrames = 0;
  float currentIdleAnimFrameDuration = 0;

  int mapWidth = 47;
  int mapHeight = 22;
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
      case '$':
        map[i].type = TILE_TYPE_GEM;
        break;
      default:
        assert("Unhandled type!");
    }
    if (map[i].type == TILE_TYPE_HERO) {
      heroRow = i / mapWidth;
      heroCol = i % mapWidth;
    }
  }

  int cameraVelX = 0;
  int cameraVelY = 0;
  int maxCameraX = mapWidth*TILE_WIDTH - BACKBUFFER_WIDTH;
  int maxCameraY = mapHeight*TILE_HEIGHT - BACKBUFFER_HEIGHT;

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

    gemTimer += dt;
    gemFrame = (int)(gemTimer / GEM_FRAME_DURATION) % GEM_FRAME_COUNT;

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

    if (explosionIsActive) {
      explosionFrameTimer += dt;
      if (explosionFrameTimer >= EXPLOSION_FRAME_DURATION) {
        explosionFrameTimer -= EXPLOSION_FRAME_DURATION;
        explosionFrame++;

        if (explosionFrame >= ARRAY_LENGTH(explosionAnim)) {
          explosionFrameTimer = 0;
          explosionFrame = 0;
          explosionIsActive = false;
          for (int row = 0; row < mapHeight; ++row) {
            for (int col = 0; col < mapWidth; ++col) {
              int i = row*mapWidth + col;
              if (map[i].type == TILE_TYPE_EXPLOSION) {
                map[i].type = TILE_TYPE_EMPTY;
              }
            }
          }
        }
      }
    }

    turnTimer += dt;
    if (turnTimer >= TURN_DURATION) {
      turnTimer -= TURN_DURATION;

      //
      // Do turn
      //

      for (int row = 0; row < mapHeight; ++row) {
        for (int col = 0; col < mapWidth; ++col) {
          int i = row*mapWidth + col;
          map[i].movedInPreviousFrame = map[i].moved;
          map[i].moved = false;
        }
      }

      // Move entities (processing order: top to bottom, left to right).
      // Move only one tile per turn.
      for (int row = 0; row < mapHeight; ++row) {
        for (int col = 0; col < mapWidth; ++col) {
          int current = row*mapWidth + col;
          if (map[current].moved) {
            continue;
          }
          if (map[current].type == TILE_TYPE_ROCK || map[current].type == TILE_TYPE_GEM) {
            int below = (row+1)*mapWidth + col;
            int left = row*mapWidth + (col-1);
            int right = row*mapWidth + (col+1);
            int belowLeft = (row+1)*mapWidth + (col-1);
            int belowRight = (row+1)*mapWidth + (col+1);
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
                map[(row+1)*mapWidth + (col+0)].type = TILE_TYPE_EXPLOSION;
                // right
                map[(row+1)*mapWidth + (col+1)].type = TILE_TYPE_EXPLOSION;
                // left
                map[(row+1)*mapWidth + (col-1)].type = TILE_TYPE_EXPLOSION;
                // above center
                map[(row+0)*mapWidth + (col+0)].type = TILE_TYPE_EXPLOSION;
                // above right
                map[(row+0)*mapWidth + (col+1)].type = TILE_TYPE_EXPLOSION;
                // above left
                map[(row+0)*mapWidth + (col-1)].type = TILE_TYPE_EXPLOSION;
                // below center
                map[(row+2)*mapWidth + (col+0)].type = TILE_TYPE_EXPLOSION;
                // below right
                map[(row+2)*mapWidth + (col+1)].type = TILE_TYPE_EXPLOSION;
                // below left
                map[(row+2)*mapWidth + (col-1)].type = TILE_TYPE_EXPLOSION;
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

      // Move hero
      if (heroIsAlive) {
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
        int newCell = newRow*mapWidth + newCol;

        switch (map[newCell].type) {
          case TILE_TYPE_EMPTY:
          case TILE_TYPE_EARTH:
          case TILE_TYPE_GEM:
            map[heroRow*mapWidth + heroCol].type = TILE_TYPE_EMPTY;
            heroRow = newRow;
            heroCol = newCol;
            map[heroRow*mapWidth + heroCol].type = TILE_TYPE_HERO;
            break;

          case TILE_TYPE_ROCK:
            int targetRockCell = newRow*mapWidth + newCol + deltaCol;
            if (deltaCol != 0 && map[targetRockCell].type == TILE_TYPE_EMPTY) {
              heroMoveRockTurns++;
              if (heroMoveRockTurns == 3) {
                heroMoveRockTurns = 0;
                map[heroRow*mapWidth + heroCol].type = TILE_TYPE_EMPTY;
                heroRow = newRow;
                heroCol = newCol;
                map[heroRow*mapWidth + heroCol].type = TILE_TYPE_HERO;
                map[targetRockCell].type = TILE_TYPE_ROCK;
              }
            }
            break;
        }
      }

      // Move camera
      {
        int heroScreenX = heroCol * TILE_WIDTH - cameraX;
        int heroScreenY = heroRow * TILE_HEIGHT - cameraY;

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
    }

    // Clear back buffer
    memset(backbuffer, 0, BACKBUFFER_BYTES);

    //
    // Draw tiles
    //
    for (int row = 0; row < mapHeight; ++row) {
      for (int col = 0; col < mapWidth; ++col) {
        TileType tile = map[row*mapWidth + col].type;

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
            if (heroIsMoving) {
              atlX = 112 + heroMoveFrame * 16;
              atlY = 0;
              flipHorizontally = heroIsFacingRight;
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

        int bbX = col * TILE_WIDTH - cameraX;
        int bbY = row * TILE_HEIGHT - cameraY;

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
            if (dstX >= 0 && dstX < BACKBUFFER_WIDTH && dstY >= 0 && dstY < BACKBUFFER_HEIGHT) {
              backbuffer[dstY*BACKBUFFER_WIDTH + dstX] = spriteAtlas[srcY*SPRITE_ATLAS_WIDTH + srcX];
            }
          }
        }
      }
    }

    //
    // Background animation
    //

    backgroundOffsetTimer += dt;
    if (backgroundOffsetTimer > backgroundOffsetDuration) {
      backgroundOffsetTimer -= backgroundOffsetDuration;
      backgroundOffset++;
      if (backgroundOffset == TILE_SIZE) {
        backgroundOffset = 0;
      }
    }

    for (int row = 0; row < SCREEN_HEIGHT_IN_TILES; ++row) {
      for (int col = 0; col < SCREEN_WIDTH_IN_TILES; ++col) {
        int srcMinY = 16; // Wall sprite Y
        int srcMaxY = srcMinY + TILE_SIZE - 1;

        int atlX = 0;
        int atlY = srcMinY + backgroundOffset;

        int bbX = col * TILE_WIDTH - cameraX;
        int bbY = row * TILE_HEIGHT - cameraY;

        for (int y = 0; y < TILE_HEIGHT; ++y) {
          for (int x = 0; x < TILE_WIDTH; ++x) {
            int srcX = atlX + x;
            int srcY = atlY + y;
            if (srcY > srcMaxY) {
              srcY = srcMinY + (srcY - srcMaxY) - 1;
            }
            int dstX = bbX + x;
            int dstY = bbY + y;
            if (dstX >= 0 && dstX < BACKBUFFER_WIDTH && dstY >= 0 && dstY < BACKBUFFER_HEIGHT) {
              backbuffer[dstY*BACKBUFFER_WIDTH + dstX] = spriteAtlas[srcY*SPRITE_ATLAS_WIDTH + srcX];
            }
          }
        }
      }
    }

    StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight,
                  0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT, backbuffer,
                  &backbufferBmpInf, DIB_RGB_COLORS, SRCCOPY);
  }
}
