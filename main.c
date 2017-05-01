#include <windows.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include "cave.h"
#include "graphics.h"

void debugPrint(char *format, ...) {
  va_list argptr;
  va_start(argptr, format);
  char str[1024];
  vsprintf_s(str, sizeof(str), format, argptr);
  va_end(argptr);
  OutputDebugString(str);
}

#define HERO_SUPERPOWER false
#define NO_ANIMATIONS false

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(*array))
#define PI 3.14159265358979323846f

#define HERO_ANIM_FRAME_DURATION 0.05f
#define HERO_ANIM_FRAME_COUNT 6
#define TURN_DURATION 0.15f
#define MAX_MAP_TILES 100*100
#define DIAMOND_FRAME_COUNT 8
#define FIREFLY_FRAME_COUNT 4

#define CAMERA_STEP HALF_TILE_SIZE
#define HERO_SIZE TILE_SIZE

#define CAMERA_START_RIGHT_HERO_X (PLAYFIELD_X_MAX - 6*HALF_TILE_SIZE + 1)
#define CAMERA_STOP_RIGHT_HERO_X (PLAYFIELD_X_MAX - 13*HALF_TILE_SIZE + 1)
#define CAMERA_START_LEFT_HERO_X (PLAYFIELD_X_MIN + 6*HALF_TILE_SIZE)
#define CAMERA_STOP_LEFT_HERO_X (PLAYFIELD_X_MIN + 14*HALF_TILE_SIZE)

#define CAMERA_START_DOWN_HERO_Y (PLAYFIELD_Y_MAX - 4*HALF_TILE_SIZE + 1)
#define CAMERA_STOP_DOWN_HERO_Y (PLAYFIELD_Y_MAX - 9*HALF_TILE_SIZE + 1)
#define CAMERA_START_UP_HERO_Y (PLAYFIELD_Y_MIN + 4*HALF_TILE_SIZE)
#define CAMERA_STOP_UP_HERO_Y (PLAYFIELD_Y_MIN + 9*HALF_TILE_SIZE)

#define MIN_CAMERA_X 0
#define MIN_CAMERA_Y 0
#define MAX_CAMERA_X (CAVE_WIDTH*TILE_SIZE - PLAYFIELD_WIDTH)
#define MAX_CAMERA_Y ((CAVE_HEIGHT-2)*TILE_SIZE - PLAYFIELD_HEIGHT)

#define BORDER_COLOR_NORMAL COLOR_BLACK
#define BORDER_COLOR_FLASH COLOR_WHITE

typedef enum {
  DIR_LEFT,
  DIR_UP,
  DIR_RIGHT,
  DIR_DOWN,
} Direction;

typedef enum {
  TURN_LEFT,
  TURN_RIGHT,
  STRAIGHT_AHEAD,
} Turning;

typedef struct {
  ObjectType type;
  bool moved;
  bool movedInPreviousFrame;
  Direction direction;
} Object;

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

typedef struct {
  int x;
  int y;
} Position;

Object map[CAVE_HEIGHT][CAVE_WIDTH];

ObjectType getObjectTypeAtPosition(Position position) {
  return map[position.y][position.x].type;
}

Position getRelativePosition(Position position, Direction direction) {
  Position result = {0};
  switch (direction) {
    case DIR_LEFT:
      result.x = position.x - 1;
      result.y = position.y;
      break;
    case DIR_UP:
      result.x = position.x;
      result.y = position.y - 1;
      break;
    case DIR_RIGHT:
      result.x = position.x + 1;
      result.y = position.y;
      break;
    case DIR_DOWN:
      result.x = position.x;
      result.y = position.y + 1;
      break;
    default:
      assert(!"Unknown direction");
  }
  return result;
}

bool checkFireflyExplode(Position position) {
  ObjectType type = getObjectTypeAtPosition(position);
  return type == OBJ_ROCKFORD || type == OBJ_AMOEBA;
}

bool fireflyWillExplode(Position fireflyPosition) {
  return
    checkFireflyExplode(getRelativePosition(fireflyPosition, DIR_UP)) ||
    checkFireflyExplode(getRelativePosition(fireflyPosition, DIR_LEFT)) ||
    checkFireflyExplode(getRelativePosition(fireflyPosition, DIR_RIGHT)) ||
    checkFireflyExplode(getRelativePosition(fireflyPosition, DIR_DOWN));
}

typedef enum {
  EXPLODE_TO_SPACE,
  EXPLODE_TO_DIAMONDS,
} ExplodeTo;

void explode(Position position, ExplodeTo to) {
  UNREFERENCED_PARAMETER(position);
  UNREFERENCED_PARAMETER(to);
}

Position getNewFireflyPosition(Position fireflyPosition, Direction fireflyDirection, Turning turning) {
  UNREFERENCED_PARAMETER(fireflyPosition);
  UNREFERENCED_PARAMETER(fireflyDirection);
  UNREFERENCED_PARAMETER(turning);
  return fireflyPosition;
}

Direction getNewDirection(Direction fireflyDirection, Turning turning) {
  UNREFERENCED_PARAMETER(fireflyDirection);
  UNREFERENCED_PARAMETER(turning);
  return fireflyDirection;
}

void placeFirefly(Position position, Direction direction) {
  UNREFERENCED_PARAMETER(position);
  UNREFERENCED_PARAMETER(direction);
}

void placeSpace(Position position) {
  UNREFERENCED_PARAMETER(position);
}

void scanFirefly(Position fireflyPosition, Direction fireflyDirection) {
  Position newPosition;
  Direction newDirection;

  if (fireflyWillExplode(fireflyPosition)) {
    explode(fireflyPosition, EXPLODE_TO_SPACE);
  } else {
    newPosition = getNewFireflyPosition(fireflyPosition, fireflyDirection, TURN_LEFT);
    if (getObjectTypeAtPosition(newPosition) == OBJ_SPACE) {
      newDirection = getNewDirection(fireflyDirection, TURN_LEFT);
      placeFirefly(newPosition, newDirection);
      placeSpace(fireflyPosition);
    } else {
      newPosition = getNewFireflyPosition(fireflyPosition, fireflyDirection, STRAIGHT_AHEAD);
      if (getObjectTypeAtPosition(newPosition) == OBJ_SPACE) {
        placeFirefly(newPosition, fireflyDirection);
        placeSpace(fireflyPosition);
      } else {
        newDirection = getNewDirection(fireflyDirection, TURN_RIGHT);
        placeFirefly(fireflyPosition, newDirection);
      }
    }
  }
}

void placePreRockford(Position position, int preRockfordStage) {
  UNREFERENCED_PARAMETER(position);
  UNREFERENCED_PARAMETER(preRockfordStage);
}

void placeRockford(Position position) {
  UNREFERENCED_PARAMETER(position);
}

void scanPreRockford(Position currentScanPosition, int preRockfordStage, int timeTillBirth) {
  assert(timeTillBirth >= 0);
  assert(preRockfordStage >= 1 && preRockfordStage <= 4);

  if (timeTillBirth == 0) {
    if (preRockfordStage <= 3) {
      placePreRockford(currentScanPosition, preRockfordStage+1);
    } else {
      placeRockford(currentScanPosition);
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

  initGraphics();

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

  //////////////////////////////////

  int difficultyLevel = 0;
  int caveNumber = 1;

  uint8_t caveData[CAVE_HEIGHT][CAVE_WIDTH];
  Cave *cave = getCave(caveNumber);
  decodeCaveData(cave, caveData);

#if 0
  cave->diamondsNeeded[difficultyLevel] = 1;
#endif

  int caveTimeLeft = 0;
  int caveTimeTurn = 0;
  int caveTimeTurnMax = 7;
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
  int diamondFrame = 0;
  int fireflyFrame = 0;

  int score = 0;
  int diamondsCollected = 0;
  int borderColor = COLOR_BLACK;

  int foregroundVisibilityTurnMax = NO_ANIMATIONS ? 1 : 28;
  int deathForegroundVisibilityTurnMax = NO_ANIMATIONS ? 1 : 25;

  int foregroundVisibilityTurn = foregroundVisibilityTurnMax;
  int foregroundOffset = 0;
  int foregroundTilesPerTurn = 6;

  int deathForegroundVisibilityTurn = 0;
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

  SpriteType exitAnim[] = {
    SPRITE_TYPE_WALL,
    SPRITE_TYPE_ENTRANCE,
  };
  int exitAnimFrame = 0;

  bool foreground[MAX_MAP_TILES];
  bool deathForeground[PLAYFIELD_HALF_TILES_COUNT];
  bool isInit = true;

  int cameraVelX = 0;
  int cameraVelY = 0;

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

      caveTimeLeft = cave->caveTime[difficultyLevel];
      caveTimeTurn = 0;
      heroIsAppearing = !NO_ANIMATIONS;
      heroIsAlive = true;
      foregroundVisibilityTurn = foregroundVisibilityTurnMax;
      foregroundOffset = 0;
      deathForegroundVisibilityTurn = 0;
      deathForegroundOffset = 0;
      diamondsCollected = 0;
      exitAnimFrame = 0;

      for (int y = 2; y <= 23; y++) {
        for(int x = 0; x <= 39; x++) {
          int mapY = y-2;
          int mapX = x;
          map[mapY][mapX].type = caveData[y][x];
          if (map[mapY][mapX].type == OBJ_PRE_ROCKFORD_STAGE_1) {
            heroRow = mapY;
            heroCol = mapX;
          }
        }
      }

      for (int i = 0; i < CAVE_WIDTH * CAVE_HEIGHT; ++i) {
        foreground[i] = true;
      }

      for (int i = 0; i < PLAYFIELD_HALF_TILES_COUNT; ++i) {
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

    int heroScreenLeft = PLAYFIELD_X_MIN + heroCol * TILE_SIZE - cameraX;
    int heroScreenTop = PLAYFIELD_Y_MIN + heroRow * TILE_SIZE - cameraY;
    int heroScreenRight = heroScreenLeft + TILE_SIZE;
    int heroScreenBottom = heroScreenTop + TILE_SIZE;

    turnTimer += dt;
    if (turnTimer >= TURN_DURATION) {
      turnTimer -= TURN_DURATION;

      //
      // Do turn
      //

      explosionIsActive = false;
      borderColor = BORDER_COLOR_NORMAL;

      diamondFrame++;
      if (diamondFrame == DIAMOND_FRAME_COUNT) {
        diamondFrame = 0;
      }

      fireflyFrame++;
      if (fireflyFrame == FIREFLY_FRAME_COUNT) {
        fireflyFrame = 0;
      }

      // Move camera
      {
        if (heroScreenRight > CAMERA_START_RIGHT_HERO_X) {
          cameraVelX = CAMERA_STEP;
        } else if (heroScreenLeft < CAMERA_START_LEFT_HERO_X) {
          cameraVelX = -CAMERA_STEP;
        }
        if (heroScreenBottom > CAMERA_START_DOWN_HERO_Y) {
          cameraVelY = CAMERA_STEP;
        } else if (heroScreenTop < CAMERA_START_UP_HERO_Y) {
          cameraVelY = -CAMERA_STEP;
        }

        if (heroScreenLeft >= CAMERA_STOP_LEFT_HERO_X && heroScreenRight <= CAMERA_STOP_RIGHT_HERO_X) {
          cameraVelX = 0;
        }
        if (heroScreenTop >= CAMERA_STOP_UP_HERO_Y && heroScreenBottom <= CAMERA_STOP_DOWN_HERO_Y) {
          cameraVelY = 0;
        }

        cameraX += cameraVelX;
        cameraY += cameraVelY;

        if (cameraX < MIN_CAMERA_X) {
          cameraX = MIN_CAMERA_X;
        } else if (cameraX > MAX_CAMERA_X) {
          cameraX = MAX_CAMERA_X;
        }

        if (cameraY < MIN_CAMERA_Y) {
          cameraY = MIN_CAMERA_Y;
        } else if (cameraY > MAX_CAMERA_Y) {
          cameraY = MAX_CAMERA_Y;
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
            int row = rand() % PLAYFIELD_HEIGHT_IN_TILES + cameraRow;
            int col = rand() % PLAYFIELD_WIDTH_IN_TILES + cameraCol;
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

        caveTimeTurn++;
        if (caveTimeTurn == caveTimeTurnMax) {
          caveTimeTurn = 0;
          --caveTimeLeft;
          if (caveTimeLeft == 0) {
            isInit = true;
          }
        }

        for (int row = 0; row < CAVE_HEIGHT; ++row) {
          for (int col = 0; col < CAVE_WIDTH; ++col) {
            map[row][col].movedInPreviousFrame = map[row][col].moved;
            map[row][col].moved = false;
          }
        }

        // Move entities (processing order: top to bottom, left to right).
        // Move only one tile per turn.
        for (int row = 0; row < CAVE_HEIGHT; ++row) {
          for (int col = 0; col < CAVE_WIDTH; ++col) {
            if (map[row][col].moved) {
              continue;
            }
            if (map[row][col].type == OBJ_EXPLODE_TO_SPACE_STAGE_1) {
              explosionIsActive = true;
            } else if (map[row][col].type == OBJ_BOULDER_STATIONARY || map[row][col].type == OBJ_DIAMOND_STATIONARY) {
              if (map[row+1][col].type == OBJ_SPACE) {
                map[row+1][col].type = map[row][col].type;
                map[row+1][col].moved = true;
                map[row][col].type = OBJ_SPACE;
              } else if (map[row+1][col].type == OBJ_PRE_ROCKFORD_STAGE_1) {
                if (map[row][col].movedInPreviousFrame && !HERO_SUPERPOWER) {
                  map[row][col].type = OBJ_SPACE;
                  map[row+1][col].type = OBJ_SPACE;
                  heroIsAlive = false;
                  for (int expRow = row; expRow <= row+2; ++expRow) {
                    for (int expCol = col-1; expCol <= col+1; ++expCol) {
                      if (map[expRow][expCol].type != OBJ_STEEL_WALL) {
                        map[expRow][expCol].type = OBJ_EXPLODE_TO_SPACE_STAGE_1;
                      }
                    }
                  }
                }
              } else if (map[row+1][col].type == OBJ_BOULDER_STATIONARY || map[row+1][col].type == OBJ_DIAMOND_STATIONARY || map[row+1][col].type == OBJ_BRICK_WALL) {
                if (map[row][col-1].type == OBJ_SPACE && map[row+1][col-1].type == OBJ_SPACE) {
                  map[row][col-1].type = map[row][col].type;
                  map[row][col-1].moved = true;
                  map[row][col].type = OBJ_SPACE;
                } else if (map[row][col+1].type == OBJ_SPACE && map[row+1][col+1].type == OBJ_SPACE) {
                  map[row][col+1].type = map[row][col].type;
                  map[row][col+1].moved = true;
                  map[row][col].type = OBJ_SPACE;
                }
              }
            } else if (map[row][col].type == OBJ_PRE_OUTBOX) {
              if (diamondsCollected >= cave->diamondsNeeded[difficultyLevel]) {
                exitAnimFrame++;
                if (exitAnimFrame == ARRAY_LENGTH(exitAnim)) {
                  exitAnimFrame = 0;
                }
              } else {
                exitAnimFrame = 0;
              }
            } else if (map[row][col].type == OBJ_FIREFLY_POSITION_1) {
            } else if (map[row][col].type == OBJ_PRE_ROCKFORD_STAGE_1) {
              if (heroIsAppearing) {
                appearanceAnimFrame++;
                if (appearanceAnimFrame == ARRAY_LENGTH(appearanceAnim)) {
                  appearanceAnimFrame = 0;
                  heroIsAppearing = false;
                }
              } else if (heroIsAlive) {
                int newRow = row;
                int newCol = col;

                if (rightIsDown) {
                  ++newCol;
                } else if (leftIsDown) {
                  --newCol;
                } else if (upIsDown) {
                  --newRow;
                } else if (downIsDown) {
                  ++newRow;
                }

                switch (map[newRow][newCol].type) {
                  case OBJ_SPACE:
                  case OBJ_DIRT:
                  case OBJ_DIAMOND_STATIONARY:
#if HERO_SUPERPOWER
                  case OBJ_BRICK_WALL:
                  case OBJ_BOULDER_STATIONARY:
#endif
                    if (map[newRow][newCol].type == OBJ_DIAMOND_STATIONARY) {
                      if (diamondsCollected >= cave->diamondsNeeded[difficultyLevel]) {
                        score += cave->extraDiamondValue;
                      } else {
                        score += cave->initialDiamondValue;
                      }
                      diamondsCollected++;
                      if (diamondsCollected == cave->diamondsNeeded[difficultyLevel]) {
                        borderColor = BORDER_COLOR_FLASH;
                      }
                    }
                    map[row][col].type = OBJ_SPACE;
                    heroRow = newRow;
                    heroCol = newCol;
                    map[newRow][newCol].type = OBJ_PRE_ROCKFORD_STAGE_1;
                    map[newRow][newCol].moved = true;
                    break;

#if !HERO_SUPERPOWER
                  case OBJ_BOULDER_STATIONARY:
                    int deltaCol = newCol - col;
                    if (deltaCol != 0 && map[newRow][newCol+deltaCol].type == OBJ_SPACE) {
                      heroMoveRockTurns++;
                      if (heroMoveRockTurns == 3) {
                        heroMoveRockTurns = 0;
                        map[row][col].type = OBJ_SPACE;
                        heroRow = newRow;
                        heroCol = newCol;
                        map[newRow][newCol].type = OBJ_PRE_ROCKFORD_STAGE_1;
                        map[newRow][newCol].moved = true;
                        map[newRow][newCol+deltaCol].type = OBJ_BOULDER_STATIONARY;
                      }
                    }
                    break;
#endif
                  case OBJ_PRE_OUTBOX:
                    if (diamondsCollected >= cave->diamondsNeeded[difficultyLevel]) {
                      map[row][col].type = OBJ_SPACE;
                      heroRow = newRow;
                      heroCol = newCol;
                      map[newRow][newCol].type = OBJ_PRE_ROCKFORD_STAGE_1;
                      map[newRow][newCol].moved = true;

                      cave = getCave(++caveNumber);
                      decodeCaveData(cave, caveData);
                      isInit = true;
                    }
                    break;
                }
              }
            }
          }
        }
      }

      if (explosionIsActive) {
        explosionFrame++;

        if (explosionFrame >= ARRAY_LENGTH(explosionAnim)) {
          explosionFrame = 0;
          deathForegroundVisibilityTurn = deathForegroundVisibilityTurnMax;
          for (int row = 0; row < CAVE_HEIGHT; ++row) {
            for (int col = 0; col < CAVE_WIDTH; ++col) {
              if (map[row][col].type == OBJ_EXPLODE_TO_SPACE_STAGE_1) {
                map[row][col].type = OBJ_SPACE;
              }
            }
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
            int row = rand() % PLAYFIELD_HEIGHT_IN_HALF_TILES;
            int col = rand() % PLAYFIELD_WIDTH_IN_HALF_TILES;
            int cell = row*PLAYFIELD_WIDTH_IN_HALF_TILES + col;
            if (!deathForeground[cell]) {
              deathForeground[cell] = true;
              break;
            }
          }
        }
      }
    }

    // Draw border
    drawFilledRect(0, 0, BACKBUFFER_WIDTH-1, BACKBUFFER_HEIGHT-1, borderColor);
    // Clear viewport
    drawFilledRect(VIEWPORT_X_MIN, VIEWPORT_Y_MIN, VIEWPORT_X_MAX, VIEWPORT_Y_MAX, COLOR_BLACK);

    //
    // Draw tiles
    //
    for (int row = 0; row < CAVE_HEIGHT; ++row) {
      for (int col = 0; col < CAVE_WIDTH; ++col) {
        ObjectType tile = map[row][col].type;

        if (tile == OBJ_SPACE) {
          continue;
        }

        bool flipHorizontally = false;
        int atlX = 0;
        int atlY = 0;

        switch (tile) {
          case OBJ_DIRT:
            atlX = 32;
            atlY = 16;
            break;

          case OBJ_BRICK_WALL:
            atlX = 48;
            atlY = 16;
            break;

          case OBJ_PRE_ROCKFORD_STAGE_1:
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

          case OBJ_BOULDER_STATIONARY:
            atlX = 16;
            atlY = 16;
            break;

          case OBJ_STEEL_WALL:
            atlX = 0;
            atlY = 16;
            break;

          case OBJ_PRE_OUTBOX: {
            int atlCol = exitAnim[exitAnimFrame] % SPRITE_ATLAS_WIDTH_TILES;
            int atlRow = exitAnim[exitAnimFrame] / SPRITE_ATLAS_WIDTH_TILES;
            atlX = atlCol * TILE_SIZE;
            atlY = atlRow * TILE_SIZE;
            break;
          }

          case OBJ_EXPLODE_TO_SPACE_STAGE_1:
            atlX = explosionAnim[explosionFrame]*16;
            atlY = 32;
            break;

          case OBJ_DIAMOND_STATIONARY:
            atlX = diamondFrame*16;
            atlY = 48;
            break;

          case OBJ_FIREFLY_POSITION_1:
            atlX = fireflyFrame*16;
            atlY = 64;
            break;
        }

        int bbX = PLAYFIELD_X_MIN + col * TILE_SIZE - cameraX;
        int bbY = PLAYFIELD_Y_MIN + row * TILE_SIZE - cameraY;

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
            if (dstX >= PLAYFIELD_X_MIN && dstX <= PLAYFIELD_X_MAX &&
                dstY >= PLAYFIELD_Y_MIN && dstY <= PLAYFIELD_Y_MAX) {
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

          int bbX = PLAYFIELD_X_MIN + col * TILE_SIZE - cameraX;
          int bbY = PLAYFIELD_Y_MIN + row * TILE_SIZE - cameraY;

          for (int y = 0; y < TILE_SIZE; ++y) {
            for (int x = 0; x < TILE_SIZE; ++x) {
              int srcX = atlX + x;
              int srcY = atlY + y;
              if (srcY > srcMaxY) {
                srcY = srcMinY + (srcY - srcMaxY) - 1;
              }
              int dstX = bbX + x;
              int dstY = bbY + y;
              if (dstX >= PLAYFIELD_X_MIN && dstX <= PLAYFIELD_X_MAX &&
                  dstY >= PLAYFIELD_Y_MIN && dstY <= PLAYFIELD_Y_MAX) {
                backbuffer[dstY*BACKBUFFER_WIDTH + dstX] = spriteAtlas[srcY*SPRITE_ATLAS_WIDTH + srcX];
              }
            }
          }
        }
      }
    }

    if (deathForegroundVisibilityTurn > 0) {
      for (int row = 0; row < PLAYFIELD_HEIGHT_IN_HALF_TILES; ++row) {
        for (int col = 0; col < PLAYFIELD_WIDTH_IN_HALF_TILES; ++col) {
          bool isVisible = deathForeground[row*PLAYFIELD_WIDTH_IN_HALF_TILES + col];

          if (!isVisible) {
            continue;
          }

          int srcMinY = 16; // Wall sprite Y
          int srcMaxY = srcMinY + HALF_TILE_SIZE - 1;

          int atlX = 0;
          int atlY = srcMinY + deathForegroundOffset;

          int bbX = PLAYFIELD_X_MIN + col * HALF_TILE_SIZE;
          int bbY = PLAYFIELD_Y_MIN + row * HALF_TILE_SIZE;

          for (int y = 0; y < HALF_TILE_SIZE; ++y) {
            for (int x = 0; x < HALF_TILE_SIZE; ++x) {
              int srcX = atlX + x;
              int srcY = atlY + y;
              if (srcY > srcMaxY) {
                srcY = srcMinY + (srcY - srcMaxY) - 1;
              }
              int dstX = bbX + x;
              int dstY = bbY + y;
              if (dstX >= PLAYFIELD_X_MIN && dstX <= PLAYFIELD_X_MAX &&
                  dstY >= PLAYFIELD_Y_MIN && dstY <= PLAYFIELD_Y_MAX) {
                backbuffer[dstY*BACKBUFFER_WIDTH + dstX] = spriteAtlas[srcY*SPRITE_ATLAS_WIDTH + srcX];
              }
            }
          }
        }
      }
    }

    {
      static char text[64];
      if (diamondsCollected >= cave->diamondsNeeded[difficultyLevel]) {
        sprintf_s(text, sizeof(text), "   ***%d   00   %03d   %06d", cave->extraDiamondValue, caveTimeLeft, score);
      } else {
        sprintf_s(text, sizeof(text), "   %d*%d   00   %03d   %06d", cave->diamondsNeeded[difficultyLevel], cave->initialDiamondValue, caveTimeLeft, score);
      }
      drawText(text, 3, 2);
    }

    // Camera debugging
#if 0
    debugPrint("x: %d, y: %d, mx: %d, my: %d\n", cameraX, cameraY, MAX_CAMERA_X, MAX_CAMERA_Y);

    drawLine(CAMERA_START_RIGHT_HERO_X, 0, CAMERA_START_RIGHT_HERO_X, BACKBUFFER_HEIGHT - 1, COLOR_PINK);
    drawLine(CAMERA_STOP_RIGHT_HERO_X, 0, CAMERA_STOP_RIGHT_HERO_X, BACKBUFFER_HEIGHT - 1, COLOR_GREEN);

    drawLine(CAMERA_START_LEFT_HERO_X, 0, CAMERA_START_LEFT_HERO_X, BACKBUFFER_HEIGHT - 1, COLOR_PINK);
    drawLine(CAMERA_STOP_LEFT_HERO_X, 0, CAMERA_STOP_LEFT_HERO_X, BACKBUFFER_HEIGHT - 1, COLOR_GREEN);

    drawLine(0, CAMERA_START_DOWN_HERO_Y, BACKBUFFER_WIDTH - 1, CAMERA_START_DOWN_HERO_Y, COLOR_PINK);
    drawLine(0, CAMERA_STOP_DOWN_HERO_Y, BACKBUFFER_WIDTH - 1, CAMERA_STOP_DOWN_HERO_Y, COLOR_GREEN);

    drawLine(0, CAMERA_START_UP_HERO_Y, BACKBUFFER_WIDTH - 1, CAMERA_START_UP_HERO_Y, COLOR_PINK);
    drawLine(0, CAMERA_STOP_UP_HERO_Y, BACKBUFFER_WIDTH - 1, CAMERA_STOP_UP_HERO_Y, COLOR_GREEN);

    drawRect(heroScreenLeft, heroScreenTop, heroScreenRight, heroScreenBottom, COLOR_YELLOW);
#endif

    StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight,
                  0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT, backbuffer,
                  &backbufferBmpInf, DIB_RGB_COLORS, SRCCOPY);
  }
}
