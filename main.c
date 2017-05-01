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

//
// Data structures
//

typedef enum {
  up1,
  down1,
  down2,
  left1,
  right1,
  down1left,
  down1right,
  up1left,
  up1right,
} offsetType;

typedef enum {
  up,
  down,
  left,
  right,
} directionType;

typedef enum {
  turnLeft,
  turnRight,
  straightAhead,
} turningType;

typedef enum {
  boulderSound,
  diamondSound,
  pickedUpDiamondSound,
  crackSound,
  explosionSound,
  movingThroughSpace,
  movingThroughDirt,
} soundType;

typedef struct {
  int x;
  int y;
} positionType;

typedef enum {
  explodeToSpace,
  explodeToDiamonds,
} explosionType;

typedef enum {
  propertyRounded,
  propertyImpactExplosive,
} propertyType;

typedef struct {
  directionType direction;
  bool fireButtonDown;
} joystickDirectionRecord;

typedef enum {
  facingRight,
  facingLeft,
} rockfordFacingType;

//
// Constants
//

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(*array))
#define PI 3.14159265358979323846f

#define BONUS_LIFE_PRICE 500
#define TURN_DURATION 0.15f
#define DIAMOND_FRAME_COUNT 8
#define FIREFLY_FRAME_COUNT 4

#define CAMERA_STEP HALF_TILE_SIZE

#define CAMERA_START_RIGHT_X (PLAYFIELD_X_MAX - 6*HALF_TILE_SIZE + 1)
#define CAMERA_STOP_RIGHT_X (PLAYFIELD_X_MAX - 13*HALF_TILE_SIZE + 1)
#define CAMERA_START_LEFT_X (PLAYFIELD_X_MIN + 6*HALF_TILE_SIZE)
#define CAMERA_STOP_LEFT_X (PLAYFIELD_X_MIN + 14*HALF_TILE_SIZE)

#define CAMERA_START_DOWN_Y (PLAYFIELD_Y_MAX - 4*HALF_TILE_SIZE + 1)
#define CAMERA_STOP_DOWN_Y (PLAYFIELD_Y_MAX - 9*HALF_TILE_SIZE + 1)
#define CAMERA_START_UP_Y (PLAYFIELD_Y_MIN + 4*HALF_TILE_SIZE)
#define CAMERA_STOP_UP_Y (PLAYFIELD_Y_MIN + 9*HALF_TILE_SIZE)

#define MIN_CAMERA_X 0
#define MIN_CAMERA_Y 0
#define MAX_CAMERA_X (CAVE_WIDTH*TILE_SIZE - PLAYFIELD_WIDTH)
#define MAX_CAMERA_Y ((CAVE_HEIGHT-2)*TILE_SIZE - PLAYFIELD_HEIGHT)

#define TOO_MANY_AMOEBA 200
#define CAVE_TIME_FRAME_MAX 7

//
// Global game state
//

uint8_t caveData[CAVE_HEIGHT][CAVE_WIDTH];
objectType map[CAVE_HEIGHT][CAVE_WIDTH];

int caveNumber;
int difficultyLevel;

int caveTime;
int caveTimeLeft;
int caveTimeFrame;

int score;
int nextBonusLifeScore;
int lives;

int initialDiamondValue;
int extraDiamondValue;
int currentDiamondValue;

int diamondsNeeded;
int diamondsCollected;
bool gotEnoughDiamonds;

bool rightIsDown;
bool leftIsDown;
bool upIsDown;
bool downIsDown;
bool spaceIsDown;

int AnimationStage;
bool RockfordMoving;
positionType RockfordLocation;
bool magicWallIsOn;
rockfordFacingType RockfordAnimationFacingDirection;
int numRoundsSinceRockfordSeenAlive;

//
//
//

void RequestSound(soundType sound) {
  // TODO: implement
  UNREFERENCED_PARAMETER(sound);
}

//
// Place- functions
//

void PlaceObject(objectType obj, positionType pos) {
  map[pos.y][pos.x] = obj;
}

void PlaceSpace(positionType position) {
  PlaceObject(OBJ_SPACE, position);
}

void PlaceAmoeba(positionType position) {
  PlaceObject(OBJ_AMOEBA, position);
}

void PlaceFallingBoulder(positionType boulderPosition) {
  PlaceObject(OBJ_BOULDER_FALLING, boulderPosition);
}

void PlaceStationaryBoulder(positionType boulderPosition) {
  PlaceObject(OBJ_BOULDER_STATIONARY, boulderPosition);
}

void PlaceFallingDiamond(positionType diamondPosition) {
  PlaceObject(OBJ_DIAMOND_FALLING, diamondPosition);
}

void PlaceStationaryDiamond(positionType diamondPosition) {
  PlaceObject(OBJ_DIAMOND_STATIONARY, diamondPosition);
}

void PlaceExplosion(positionType explosionPosition, explosionType explodeToWhat, int explosionStage) {
  switch (explodeToWhat) {
    case explodeToSpace:
      switch (explosionStage) {
        case 0:
          PlaceObject(OBJ_EXPLODE_TO_SPACE_STAGE_0, explosionPosition);
          break;
        case 1:
          PlaceObject(OBJ_EXPLODE_TO_SPACE_STAGE_1, explosionPosition);
          break;
        case 2:
          PlaceObject(OBJ_EXPLODE_TO_SPACE_STAGE_2, explosionPosition);
          break;
        case 3:
          PlaceObject(OBJ_EXPLODE_TO_SPACE_STAGE_3, explosionPosition);
          break;
        case 4:
          PlaceObject(OBJ_EXPLODE_TO_SPACE_STAGE_4, explosionPosition);
          break;
        default:
          assert(!"Unknown explosion stage");
      }
      break;

    case explodeToDiamonds:
      switch (explosionStage) {
        case 0:
          PlaceObject(OBJ_EXPLODE_TO_DIAMOND_STAGE_0, explosionPosition);
          break;
        case 1:
          PlaceObject(OBJ_EXPLODE_TO_DIAMOND_STAGE_1, explosionPosition);
          break;
        case 2:
          PlaceObject(OBJ_EXPLODE_TO_DIAMOND_STAGE_2, explosionPosition);
          break;
        case 3:
          PlaceObject(OBJ_EXPLODE_TO_DIAMOND_STAGE_3, explosionPosition);
          break;
        case 4:
          PlaceObject(OBJ_EXPLODE_TO_DIAMOND_STAGE_4, explosionPosition);
          break;
        default:
          assert(!"Unknown explosion stage");
      }
      break;

    default:
      assert(!"Unknown explostion type");
  }
}

void PlaceFirefly(positionType position, directionType direction) {
  switch (direction) {
    case left:
      PlaceObject(OBJ_FIREFLY_POSITION_1, position);
      break;
    case up:
      PlaceObject(OBJ_FIREFLY_POSITION_2, position);
      break;
    case right:
      PlaceObject(OBJ_FIREFLY_POSITION_3, position);
      break;
    case down:
      PlaceObject(OBJ_FIREFLY_POSITION_4, position);
      break;
  }
  assert(!"Unknown direction");
}

void PlaceButterfly(positionType position, directionType direction) {
  switch (direction) {
    case down:
      PlaceObject(OBJ_BUTTERFLY_POSITION_1, position);
      break;
    case left:
      PlaceObject(OBJ_BUTTERFLY_POSITION_2, position);
      break;
    case up:
      PlaceObject(OBJ_BUTTERFLY_POSITION_3, position);
      break;
    case right:
      PlaceObject(OBJ_BUTTERFLY_POSITION_4, position);
      break;
  }
  assert(!"Unknown direction");
}

void PlacePreRockford(positionType position, int preRockfordStage) {
  switch (preRockfordStage) {
    case 1:
      PlaceObject(OBJ_PRE_ROCKFORD_STAGE_1, position);
      break;
    case 2:
      PlaceObject(OBJ_PRE_ROCKFORD_STAGE_2, position);
      break;
    case 3:
      PlaceObject(OBJ_PRE_ROCKFORD_STAGE_3, position);
      break;
    case 4:
      PlaceObject(OBJ_PRE_ROCKFORD_STAGE_4, position);
      break;
  }
  assert(!"Unknown stage");
}

void PlaceRockford(positionType position) {
  PlaceObject(OBJ_ROCKFORD, position);
}

void PlaceOutBox(positionType position) {
  PlaceObject(OBJ_FLASHING_OUTBOX, position);
}

//
//
//

objectType GetObjectAtPosition(positionType position) {
  return map[position.y][position.x];
}

positionType GetRelativePosition(positionType position, offsetType offset) {
  positionType result;
  result.x = position.x;
  result.y = position.y;
  switch (offset) {
    case left1:
      result.x = position.x - 1;
      result.y = position.y;
      break;
    case up1:
      result.x = position.x;
      result.y = position.y - 1;
      break;
    case right1:
      result.x = position.x + 1;
      result.y = position.y;
      break;
    case down1:
      result.x = position.x;
      result.y = position.y + 1;
      break;

    case down2:
      result.x = position.x;
      result.y = position.y + 2;
      break;

    case down1left:
      result.x = position.x - 1;
      result.y = position.y + 1;
      break;

    case down1right:
      result.x = position.x + 1;
      result.y = position.y + 1;
      break;

    case up1left:
      result.x = position.x - 1;
      result.y = position.y - 1;
      break;

    case up1right:
      result.x = position.x + 1;
      result.y = position.y - 1;
      break;

    default:
      assert(!"Unknown offset");
  }
  return result;
}

bool CheckFlyExplode(positionType position) {
  objectType type = GetObjectAtPosition(position);
  return
    type == OBJ_ROCKFORD ||
    type == OBJ_ROCKFORD_SCANNED ||
    type == OBJ_AMOEBA ||
    type == OBJ_AMOEBA_SCANNED;
}

bool FlyWillExplode(positionType fireflyPosition) {
  return
    CheckFlyExplode(GetRelativePosition(fireflyPosition, up1)) ||
    CheckFlyExplode(GetRelativePosition(fireflyPosition, left1)) ||
    CheckFlyExplode(GetRelativePosition(fireflyPosition, right1)) ||
    CheckFlyExplode(GetRelativePosition(fireflyPosition, down1));
}

void ScanExplosion(positionType explosionPosition, explosionType explodeToWhat, int explosionStage) {
  // Morph the explosion into the next stage of the explosion.
  assert((explosionStage >= 0) && (explosionStage <= 4));

  // Explosion stages zero to 3 morph into stages 1 to 4
  if (explosionStage <= 3) {
    PlaceExplosion(explosionPosition, explodeToWhat, explosionStage+1);
  } else {
    // Explosion stage 4 morphs into a space or a diamond as appropriate.
    if (explodeToWhat == explodeToSpace) {
      PlaceSpace(explosionPosition);
    } else {
      PlaceStationaryDiamond(explosionPosition);
    }
  }
}

void Explode1Space(positionType explosionPosition, explosionType explodeToWhat, int explosionStage) {
  // Explode one space to the required stage and type of explosion, checking
  // whether the object at this space can be exploded first.
  // Note that if the object that gets exploded happens to be Rockford, then naturally
  // Rockford will no longer be in existance. However, we don't explicitely check whether
  // we are exploding Rockford here; his absence will be noticed next scan frame
  // if we don't come across Rockford at any time during the next scan frame.
  assert((explosionStage == 0) || (explosionStage == 1));

  if (GetObjectAtPosition(explosionPosition) != OBJ_STEEL_WALL) {
    PlaceExplosion(explosionPosition, explodeToWhat, explosionStage);
  }
}

void Explode(positionType explosionPosition, explosionType explodeToWhat) {
  // Explode something at the specified position into the specified object type

  // The spaces prior to and including the current space in the scan sequence
  // are set to a stage 1 explosion.
  Explode1Space(GetRelativePosition(explosionPosition, up1left), explodeToWhat, 1);
  Explode1Space(GetRelativePosition(explosionPosition, up1), explodeToWhat, 1);
  Explode1Space(GetRelativePosition(explosionPosition, up1right), explodeToWhat, 1);
  Explode1Space(GetRelativePosition(explosionPosition, left1), explodeToWhat, 1);
  Explode1Space(explosionPosition, explodeToWhat, 1);

  // The spaces after the current scan position are set to a stage 0 explosion,
  // so that they will be a stage 1 explosion by the end of the scan frame.
  Explode1Space(GetRelativePosition(explosionPosition, right1), explodeToWhat, 0);
  Explode1Space(GetRelativePosition(explosionPosition, down1left), explodeToWhat, 0);
  Explode1Space(GetRelativePosition(explosionPosition, down1), explodeToWhat, 0);
  Explode1Space(GetRelativePosition(explosionPosition, down1right), explodeToWhat, 0);
  RequestSound(explosionSound);
}

positionType GetNextFlyPosition(positionType flyPosition, directionType flyDirection, turningType flyTurning) {
  positionType result = flyPosition;

  switch (flyDirection) {
    case up:
      switch (flyTurning) {
        case turnLeft:
          result.x--;
          break;
        case straightAhead:
          result.y--;
          break;
        case turnRight:
          result.x++;
          break;
      }
      break;

    case down:
      switch (flyTurning) {
        case turnLeft:
          result.x++;
          break;
        case straightAhead:
          result.y++;
          break;
        case turnRight:
          result.x--;
          break;
      }
      break;

    case left:
      switch (flyTurning) {
        case turnLeft:
          result.y++;
          break;
        case straightAhead:
          result.x--;
          break;
        case turnRight:
          result.y--;
          break;
      }
      break;

    case right:
      switch (flyTurning) {
        case turnLeft:
          result.y--;
          break;
        case straightAhead:
          result.x++;
          break;
        case turnRight:
          result.y++;
          break;
      }
      break;
  }

  return result;
}

directionType GetNewDirection(directionType flyDirection, turningType flyTurning) {
  directionType result = {0};
  switch (flyDirection) {
    case up:
      switch (flyTurning) {
        case turnLeft:
          result = left;
          break;
        case straightAhead:
          result = up;
          break;
        case turnRight:
          result = right;
          break;
      }
      break;

    case down:
      switch (flyTurning) {
        case turnLeft:
          result = right;
          break;
        case straightAhead:
          result = down;
          break;
        case turnRight:
          result = left;
          break;
      }
      break;

    case left:
      switch (flyTurning) {
        case turnLeft:
          result = down;
          break;
        case straightAhead:
          result = left;
          break;
        case turnRight:
          result = up;
          break;
      }
      break;

    case right:
      switch (flyTurning) {
        case turnLeft:
          result = up;
          break;
        case straightAhead:
          result = right;
          break;
        case turnRight:
          result = down;
          break;
      }
      break;
  }
  return result;
}

void ScanFirefly(positionType positionOfFirefly, directionType directionOfFirefly) {
  positionType NewPosition;
  directionType NewDirection;

  if (FlyWillExplode(positionOfFirefly)) {
    Explode(positionOfFirefly, explodeToSpace);
  } else {
        NewPosition = GetNextFlyPosition(positionOfFirefly, directionOfFirefly, turnLeft);
    if (GetObjectAtPosition(NewPosition) == OBJ_SPACE) {
      NewDirection = GetNewDirection(directionOfFirefly, turnLeft);
      PlaceFirefly(NewPosition, NewDirection);
      PlaceSpace(positionOfFirefly);
    } else {
      NewPosition = GetNextFlyPosition(positionOfFirefly, directionOfFirefly, straightAhead);
      if (GetObjectAtPosition(NewPosition) == OBJ_SPACE) {
        PlaceFirefly(NewPosition, directionOfFirefly);
        PlaceSpace(positionOfFirefly);
      } else {
        NewDirection = GetNewDirection(directionOfFirefly, turnRight);
        PlaceFirefly(positionOfFirefly, NewDirection);
      }
    }
  }
}

void ScanButterfly(positionType positionOfButterfly, directionType directionOfButterfly) {
  positionType NewPosition;
  directionType NewDirection;

  if (FlyWillExplode(positionOfButterfly)) {
    Explode(positionOfButterfly, explodeToDiamonds);
  } else {
    NewPosition = GetNextFlyPosition(positionOfButterfly, directionOfButterfly, turnRight);
    if (GetObjectAtPosition(NewPosition) == OBJ_SPACE) {
      NewDirection = GetNewDirection(directionOfButterfly, turnRight);
      PlaceButterfly(NewPosition, NewDirection);
      PlaceSpace(positionOfButterfly);
    } else {
      NewPosition = GetNextFlyPosition(positionOfButterfly, directionOfButterfly, straightAhead);
      if (GetObjectAtPosition(NewPosition) == OBJ_SPACE) {
        PlaceButterfly(NewPosition, directionOfButterfly);
        PlaceSpace(positionOfButterfly);
      } else {
        NewDirection = GetNewDirection(directionOfButterfly, turnLeft);
        PlaceButterfly(positionOfButterfly, NewDirection);
      }
    }
  }
}

void ScanPreRockford(positionType currentScanPosition, int preRockfordStage, int timeTillBirth) {
  assert(timeTillBirth >= 0);
  assert(preRockfordStage >= 1 && preRockfordStage <= 4);

  if (timeTillBirth == 0) {
    if (preRockfordStage <= 3) {
      PlacePreRockford(currentScanPosition, preRockfordStage+1);
    } else {
      PlaceRockford(currentScanPosition);
    }
  }
}

bool GetObjectProperty(objectType anObject, propertyType property) {
  switch (property) {
    case propertyRounded:
      return
        anObject == OBJ_BOULDER_STATIONARY ||
        anObject == OBJ_BOULDER_STATIONARY_SCANNED ||
        anObject == OBJ_DIAMOND_STATIONARY ||
        anObject == OBJ_DIAMOND_STATIONARY_SCANNED ||
        anObject == OBJ_BRICK_WALL;

    case propertyImpactExplosive:
      return
        anObject == OBJ_ROCKFORD ||
        anObject == OBJ_ROCKFORD_SCANNED ||
        anObject == OBJ_FIREFLY_POSITION_1 ||
        anObject == OBJ_FIREFLY_POSITION_2 ||
        anObject == OBJ_FIREFLY_POSITION_3 ||
        anObject == OBJ_FIREFLY_POSITION_4 ||
        anObject == OBJ_FIREFLY_POSITION_1_SCANNED ||
        anObject == OBJ_FIREFLY_POSITION_2_SCANNED ||
        anObject == OBJ_FIREFLY_POSITION_3_SCANNED ||
        anObject == OBJ_FIREFLY_POSITION_4_SCANNED ||
        anObject == OBJ_BUTTERFLY_POSITION_1 ||
        anObject == OBJ_BUTTERFLY_POSITION_2 ||
        anObject == OBJ_BUTTERFLY_POSITION_3 ||
        anObject == OBJ_BUTTERFLY_POSITION_4 ||
        anObject == OBJ_BUTTERFLY_POSITION_1_SCANNED ||
        anObject == OBJ_BUTTERFLY_POSITION_2_SCANNED ||
        anObject == OBJ_BUTTERFLY_POSITION_3_SCANNED ||
        anObject == OBJ_BUTTERFLY_POSITION_4_SCANNED;
    default:
      assert(!"Unknown property");
  }
  return false;
}

bool CanRollOff(objectType anObjectBelow) {
  return GetObjectProperty(anObjectBelow, propertyRounded);
}

bool ImpactExplosive(objectType anObject) {
  return GetObjectProperty(anObject, propertyImpactExplosive);
}

explosionType GetExplosionType(objectType anObject) {
  assert(ImpactExplosive(anObject));

  switch (anObject) {
    case OBJ_ROCKFORD:
    case OBJ_ROCKFORD_SCANNED:
    case OBJ_FIREFLY_POSITION_1:
    case OBJ_FIREFLY_POSITION_2:
    case OBJ_FIREFLY_POSITION_3:
    case OBJ_FIREFLY_POSITION_4:
    case OBJ_FIREFLY_POSITION_1_SCANNED:
    case OBJ_FIREFLY_POSITION_2_SCANNED:
    case OBJ_FIREFLY_POSITION_3_SCANNED:
    case OBJ_FIREFLY_POSITION_4_SCANNED:
      return explodeToSpace;

    case OBJ_BUTTERFLY_POSITION_1:
    case OBJ_BUTTERFLY_POSITION_2:
    case OBJ_BUTTERFLY_POSITION_3:
    case OBJ_BUTTERFLY_POSITION_4:
    case OBJ_BUTTERFLY_POSITION_1_SCANNED:
    case OBJ_BUTTERFLY_POSITION_2_SCANNED:
    case OBJ_BUTTERFLY_POSITION_3_SCANNED:
    case OBJ_BUTTERFLY_POSITION_4_SCANNED:
      return explodeToDiamonds;

    default:
      assert(!"Unknown object");
  }

  return false;
}

void ScanStationaryBoulder(positionType boulderPosition) {
  positionType NewPosition;
  objectType theObjectBelow;

  // If the boulder can fall, move it down and mark it as falling.
  NewPosition = GetRelativePosition(boulderPosition, down1);
  theObjectBelow = GetObjectAtPosition(NewPosition);
  if (theObjectBelow == OBJ_SPACE) {
    PlaceFallingBoulder(NewPosition);
    PlaceSpace(boulderPosition);
    RequestSound(boulderSound); // yes, even when it starts falling. This applies to diamonds too (requests diamondSound).
  } else {
    // Failing that, see if the boulder can roll
    if (CanRollOff(theObjectBelow)) {

      // Try rolling left
      NewPosition = GetRelativePosition(boulderPosition, left1);
      if ((GetObjectAtPosition(NewPosition) == OBJ_SPACE) && (GetObjectAtPosition(GetRelativePosition(boulderPosition, down1left)) == OBJ_SPACE)) {
        PlaceFallingBoulder(NewPosition);
        PlaceSpace(boulderPosition);
      } else {
        // Try rolling right
        NewPosition = GetRelativePosition(boulderPosition, right1);
        if ((GetObjectAtPosition(NewPosition) == OBJ_SPACE) && (GetObjectAtPosition(GetRelativePosition(boulderPosition, down1right)) == OBJ_SPACE)) {
          PlaceFallingBoulder(NewPosition);
          PlaceSpace(boulderPosition);
        }
      }
    }
  }
}

void ScanFallingBoulder(positionType boulderPosition) {
  // Local variables
  positionType NewPosition;
  objectType theObjectBelow;

  // If the boulder can continue to fall, move it down.
  NewPosition = GetRelativePosition(boulderPosition, down1);
  theObjectBelow = GetObjectAtPosition(NewPosition);
  if (theObjectBelow == OBJ_SPACE) {
    PlaceFallingBoulder(NewPosition);
    PlaceSpace(boulderPosition);
  }
  // If the object below is a magic wall, we activate it (if it's off), and
  // morph into a diamond two spaces below if it's now active. If the wall
  // is expired, we just disappear (with a sound still though).
  else if (theObjectBelow == OBJ_MAGIC_WALL) {
    if (magicWallIsOn == false) {
      magicWallIsOn = true;
    }
    if (magicWallIsOn == true) {
      NewPosition = GetRelativePosition(boulderPosition, down2);
      if (GetObjectAtPosition(NewPosition) == OBJ_SPACE) {
        PlaceFallingDiamond(NewPosition);
      }
    }
    PlaceSpace(boulderPosition);
    RequestSound(diamondSound);
  }
  // Failing that, we've hit something, so we play a sound and see if we can roll.
  else {
    RequestSound(boulderSound);
    if (CanRollOff(theObjectBelow)) {
      // Try rolling left
      NewPosition = GetRelativePosition(boulderPosition, left1);
      if ((GetObjectAtPosition(NewPosition) == OBJ_SPACE) && (GetObjectAtPosition(GetRelativePosition(boulderPosition, down1left)) == OBJ_SPACE)) {
        PlaceFallingBoulder(NewPosition);
        PlaceSpace(boulderPosition);
      }
      else {
        // Try rolling right
        NewPosition = GetRelativePosition(boulderPosition, right1);
        if ((GetObjectAtPosition(NewPosition) == OBJ_SPACE) && (GetObjectAtPosition(GetRelativePosition(boulderPosition, down1right)) == OBJ_SPACE)) {
          PlaceFallingBoulder(NewPosition);
          PlaceSpace(boulderPosition);
        }
        // The boulder is sitting on an object which it could roll off, but it can't
        // roll, so it comes to a stop.
        else {
          PlaceStationaryBoulder(boulderPosition);
        }
      }
    }
    // Failing all that, we see whether we've hit something explosive
    else if (ImpactExplosive(theObjectBelow)) {
      Explode(NewPosition, GetExplosionType(theObjectBelow));
    }
    // And lastly, failing everything, the boulder comes to a stop.
    else {
      PlaceStationaryBoulder(boulderPosition);
    }
  }
}

void ScanStationaryDiamond(positionType diamondPosition) {
  positionType NewPosition;
  objectType theObjectBelow;

  // If the diamond can fall, move it down and mark it as falling.
  NewPosition = GetRelativePosition(diamondPosition, down1);
  theObjectBelow = GetObjectAtPosition(NewPosition);
  if (theObjectBelow == OBJ_SPACE) {
    PlaceFallingDiamond(NewPosition);
    PlaceSpace(diamondPosition);
    RequestSound(diamondSound); // yes, even when it starts falling. This applies to diamonds too (requests diamondSound).
  } else {
    // Failing that, see if the diamond can roll
    if (CanRollOff(theObjectBelow)) {

      // Try rolling left
      NewPosition = GetRelativePosition(diamondPosition, left1);
      if ((GetObjectAtPosition(NewPosition) == OBJ_SPACE) && (GetObjectAtPosition(GetRelativePosition(diamondPosition, down1left)) == OBJ_SPACE)) {
        PlaceFallingDiamond(NewPosition);
        PlaceSpace(diamondPosition);
      } else {
        // Try rolling right
        NewPosition = GetRelativePosition(diamondPosition, right1);
        if ((GetObjectAtPosition(NewPosition) == OBJ_SPACE) && (GetObjectAtPosition(GetRelativePosition(diamondPosition, down1right)) == OBJ_SPACE)) {
          PlaceFallingDiamond(NewPosition);
          PlaceSpace(diamondPosition);
        }
      }
    }
  }
}

void ScanFallingDiamond(positionType diamondPosition) {
  // Local variables
  positionType NewPosition;
  objectType theObjectBelow;

  // If the diamond can continue to fall, move it down.
  NewPosition = GetRelativePosition(diamondPosition, down1);
  theObjectBelow = GetObjectAtPosition(NewPosition);
  if (theObjectBelow == OBJ_SPACE) {
    PlaceFallingDiamond(NewPosition);
    PlaceSpace(diamondPosition);
  }
  // If the object below is a magic wall, we activate it (if it's off), and
  // morph into a boulder two spaces below if it's now active. If the wall
  // is expired, we just disappear (with a sound still though).
  else if (theObjectBelow == OBJ_MAGIC_WALL) {
    if (magicWallIsOn == false) {
      magicWallIsOn = true;
    }
    if (magicWallIsOn == true) {
      NewPosition = GetRelativePosition(diamondPosition, down2);
      if (GetObjectAtPosition(NewPosition) == OBJ_SPACE) {
        PlaceFallingBoulder(NewPosition);
      }
    }
    PlaceSpace(diamondPosition);
    RequestSound(boulderSound);
  }
  // Failing that, we've hit something, so we play a sound and see if we can roll.
  else {
    RequestSound(diamondSound);
    if (CanRollOff(theObjectBelow)) {
      // Try rolling left
      NewPosition = GetRelativePosition(diamondPosition, left1);
      if ((GetObjectAtPosition(NewPosition) == OBJ_SPACE) && (GetObjectAtPosition(GetRelativePosition(diamondPosition, down1left)) == OBJ_SPACE)) {
        PlaceFallingDiamond(NewPosition);
        PlaceSpace(diamondPosition);
      }
      else {
        // Try rolling right
        NewPosition = GetRelativePosition(diamondPosition, right1);
        if ((GetObjectAtPosition(NewPosition) == OBJ_SPACE) && (GetObjectAtPosition(GetRelativePosition(diamondPosition, down1right)) == OBJ_SPACE)) {
          PlaceFallingDiamond(NewPosition);
          PlaceSpace(diamondPosition);
        }
        // The diamond is sitting on an object which it could roll off, but it can't
        // roll, so it comes to a stop.
        else {
          PlaceStationaryDiamond(diamondPosition);
        }
      }
    }
    // Failing all that, we see whether we've hit something explosive
    else if (ImpactExplosive(theObjectBelow)) {
      Explode(NewPosition, GetExplosionType(theObjectBelow));
    }
    // And lastly, failing everything, the diamond comes to a stop.
    else {
      PlaceStationaryDiamond(diamondPosition);
    }
  }
}

void AddLife() {
  assert(lives <= 9);

  if (lives < 9) {
    lives++;
    // TODO: Make space objects flash to indicate bonus life;
  }
}

void CheckForBonusLife()  {
  if (score >= nextBonusLifeScore) {
    AddLife();
    nextBonusLifeScore += BONUS_LIFE_PRICE;
  }
}

void ScanPreOutBox(positionType currentScanPosition, bool GotEnoughDiamonds) {
  // If Rockford has collected enough diamonds, we can change the 
  // pre-out box (ie an out box which has not yet been activated) 
  // into a flashing out box.

  if (GotEnoughDiamonds) {
    PlaceOutBox(currentScanPosition);
  }
}

void CheckEnoughDiamonds() {
  if (diamondsCollected == diamondsNeeded) {
    gotEnoughDiamonds = true;
    currentDiamondValue = extraDiamondValue;
    // TODO: Update statusbar;
    RequestSound(crackSound);
    // TODO: Request screen to flash white to indicate got enough diamonds;
  }
}

void PickUpDiamond() {
  // Player has picked up a diamond. Increase their score, increase their number 
  // of diamonds collected, and check whether they have enough diamonds now.

  RequestSound(pickedUpDiamondSound);
  score += currentDiamondValue;
  CheckForBonusLife();
  diamondsCollected++;
  CheckEnoughDiamonds();
}

joystickDirectionRecord GetJoystickPos() {
  joystickDirectionRecord result;

  if (rightIsDown) {
    result.direction = right;
  } else if (leftIsDown) {
    result.direction = left;
  } else if (upIsDown) {
    result.direction = up;
  } else if (downIsDown) {
    result.direction = down;
  }

  result.fireButtonDown = spaceIsDown;

  return result;
}

void RequestRockfordMovementSound(soundType sound) {
  RequestSound(sound);
}

int GetRandomNumber(int min, int max) {
  return min + rand() % (max - min + 1);
}

bool PushBoulder(positionType newBoulderPosition) {
  // There is a 12.5% (1 in 8) than Rockford will succeed in pushing the boulder.
  // Return true if boulder successfully pushed, false if not.

  // Local variables
  bool pushSuccessful;

  pushSuccessful = (GetRandomNumber(0, 7) == 0);
  if (pushSuccessful) {
    RequestSound(boulderSound);
    PlaceStationaryBoulder(newBoulderPosition);
  }

  return pushSuccessful;
}

bool MoveRockfordStage3(positionType newPosition,
                        joystickDirectionRecord JoyPos) {
  // See what object is in the space where Rockford is moving and deal with it
  // appropriately. Returns true if the movement was successful, false otherwise.

  // Local Variables
  bool movementSuccessful;
  objectType theObject;
  positionType NewBoulderPosition;

  // Determine what object is in the place where Rockford is moving.
  movementSuccessful = false;
  theObject = GetObjectAtPosition(newPosition);

  switch (theObject) {
    case OBJ_SPACE: // Space: move there, and play a sound (lower pitch white noise)
      movementSuccessful = true;
      RequestRockfordMovementSound(movingThroughSpace);
      break;

    case OBJ_DIRT: // Dirt: move there, and play a sound (higher pitch white noise)
      movementSuccessful = true;
      RequestRockfordMovementSound(movingThroughDirt);
      break;

    case OBJ_DIAMOND_STATIONARY: // Diamond: pick it up
    case OBJ_DIAMOND_STATIONARY_SCANNED:
      movementSuccessful = true;
      PickUpDiamond();
      break;

    case OBJ_FLASHING_OUTBOX: // OutBox: flag that we've got out of the cave
      movementSuccessful = true;
      // TODO: FlagThatWeAreExitingTheCave(and that we got out alive);
      break;

    case OBJ_BOULDER_STATIONARY: // Boulder: push it
    case OBJ_BOULDER_STATIONARY_SCANNED:
      if (JoyPos.direction == left) {
        NewBoulderPosition = GetRelativePosition(newPosition, left1);
        if (GetObjectAtPosition(NewBoulderPosition) == OBJ_SPACE) {
          movementSuccessful = PushBoulder(NewBoulderPosition);
        }
      } else if (JoyPos.direction == right) {
        NewBoulderPosition = GetRelativePosition(newPosition, right1);
        if (GetObjectAtPosition(NewBoulderPosition) == OBJ_SPACE) {
          movementSuccessful = PushBoulder(NewBoulderPosition);
        }
      }
      break;
  }

  // Return an indication of whether we were successful in moving.
  return movementSuccessful;
}

bool MoveRockfordStage2(positionType originalPosition,
                        positionType newPosition,
                        joystickDirectionRecord JoyPos) {
  // Part of the Move Rockford routine. Call MoveRockfordStage3 to do all the work.
  // All this routine does is check to see if the fire button was down, and
  // so either move Rockford to his new position or put a space where he would
  // have moved. Returns true if Rockford really did physically move.

  // Local variables
  bool ActuallyMoved;

  // Call a subroutine to move Rockford. It returns true if the movement was
  // successful (without regard to the fire button).
  ActuallyMoved = MoveRockfordStage3(newPosition, JoyPos);

  // If the movement was successful, we check the fire button to determine
  // whether Rockford actually physically moves to the new positon or not.
  if (ActuallyMoved) {
    if (JoyPos.fireButtonDown) {
      PlaceSpace(newPosition);
      ActuallyMoved = false;
    } else {
      PlaceRockford(newPosition);
      PlaceSpace(originalPosition);
    }
  }
  
  // Tell our caller whether or not Rockford physically moved to a new position.
  return ActuallyMoved;
}

void MoveRockfordStage1(positionType currentScanPosition,
                        joystickDirectionRecord JoyPos) {
  // Note: in this routine, if the user presses diagonally, the horizontal movement takes
  // precedence over the vertical movement; ie Rockford moves horizontally.

  // Local variables
  bool ActuallyMoved;
  positionType NewPosition = {0};

  // Determine Rockford's new location if he actually moves there (ie he isn't
  // blocked by a wall or something, and isn't holding the fire button down).
  switch (JoyPos.direction) {
    case down:
      RockfordMoving = true;
      NewPosition = GetRelativePosition(currentScanPosition, down1);
      break;
    case up:
      RockfordMoving = true;
      NewPosition = GetRelativePosition(currentScanPosition, up1);
      break;
    case right:
      RockfordMoving = true;
      RockfordAnimationFacingDirection = facingRight;
      NewPosition = GetRelativePosition(currentScanPosition, right1);
      break;
    case left:
      RockfordMoving = true;
      RockfordAnimationFacingDirection = facingLeft;
      NewPosition = GetRelativePosition(currentScanPosition, left1);
      break;
    default:
      RockfordMoving = false;
      break;
  }

  if (RockfordMoving) {
    // Call a subroutine to actually deal with this further.
    ActuallyMoved = MoveRockfordStage2(currentScanPosition, NewPosition, JoyPos);

    // If Rockford did in fact physically move, we update our record of Rockford's
    // position (used by the screen scrolling algorithm to know where to scroll).
    if (ActuallyMoved) {
      RockfordLocation = NewPosition;
    }
  }
}

void ScanRockford(positionType currentScanPosition) {
  assert(numRoundsSinceRockfordSeenAlive >= 0);
  joystickDirectionRecord JoyPos = GetJoystickPos();
  MoveRockfordStage1(currentScanPosition, JoyPos);
  numRoundsSinceRockfordSeenAlive = 0;
}

void AnimateRockford(bool *Tapping, bool *Blinking) {
  // Called by the animation routine every animation frame

  // If Rockford is currently moving, we display the right-moving or left-moving animation
  // sequence.
  if (RockfordMoving) {
    // Can't tap or blink while moving
    *Tapping = false;
    *Blinking = false;

    // Set up animation left or right as appropriate
    if (RockfordAnimationFacingDirection == facingRight) {
      // TODO: doing right-facing Rockford animation sequence
    } else {
      // TODO: doing left-facing Rockford animation sequence
    }

  }
  // If Rockford is not currently moving, we display a forward facing animation sequence.
  // Rockford might be idle, tapping, blinking, or both.
  else {
    // If we're at the beginning of an animation sequence, then check whether we
    // will blink or tap for this sequence
    if (AnimationStage == 1) {
      // 1 in 4 chance of blinking
      *Blinking = (GetRandomNumber(0, 3) == 0);
      // 1 in 16 chance of starting or stopping foot tapping
      if (GetRandomNumber(0, 15) == 0) {
        *Tapping = !*Tapping;
      }     
    }
    // TODO: doing forward-facing Rockford animation sequence (idle, blink, tap, or blink&tap)
  }
}

bool AmoebaRandomlyDecidesToGrow(int anAmoebaRandomFactor) {
  // Randomly decide whether this amoeba is going to attempt to grow or not. 
  // anAmoebaRandomFactor should normally be 127 (slow growth) but sometimes is 
  // changed to 15 (fast growth) if the amoeba has been alive too long.
  assert(anAmoebaRandomFactor == 15 || anAmoebaRandomFactor == 127);
  return (GetRandomNumber(0, anAmoebaRandomFactor) < 4); 
}

directionType GetRandomDirection() {
  int n = GetRandomNumber(0, 3);
  switch (n) {
    case 0:
      return up;
    case 1:
      return down;
    case 2:
      return left;
  }
  return right;
}

void ScanAmoeba(positionType positionOfAmoeba,
                int anAmoebaRandomFactor,
                bool amoebaSuffocatedLastFrame,
                bool *atLeastOneAmoebaFoundThisFrameWhichCanGrow,
                int totalAmoebaFoundLastFrame,
                int *numberOfAmoebaFoundThisFrame) {
  // Local variables
  directionType direction;
  positionType NewPosition;

  assert(anAmoebaRandomFactor > 0);
  assert(totalAmoebaFoundLastFrame > 0);
  assert(*numberOfAmoebaFoundThisFrame > 0);
  *numberOfAmoebaFoundThisFrame++;

  // If the amoeba grew too big last frame, morph into a boulder.
  if (totalAmoebaFoundLastFrame >= TOO_MANY_AMOEBA) {
    PlaceStationaryBoulder(positionOfAmoeba);
  } else {
    // If the amoeba suffocated last frame, morph into a diamond
    if (amoebaSuffocatedLastFrame) {
      PlaceStationaryDiamond(positionOfAmoeba);
    } else {
      // If we haven't yet found any amoeba this frame which can grow, we check to 
      // see whether this particular amoeba can grow.
      if (!*atLeastOneAmoebaFoundThisFrameWhichCanGrow) {
        objectType obj;

        obj = GetObjectAtPosition(GetRelativePosition(positionOfAmoeba, up1));
        if (obj == OBJ_SPACE || obj == OBJ_DIRT) {
          *atLeastOneAmoebaFoundThisFrameWhichCanGrow = true;
        }

        obj = GetObjectAtPosition(GetRelativePosition(positionOfAmoeba, left1));
        if (obj == OBJ_SPACE || obj == OBJ_DIRT) {
          *atLeastOneAmoebaFoundThisFrameWhichCanGrow = true;
        }

        obj = GetObjectAtPosition(GetRelativePosition(positionOfAmoeba, right1));
        if (obj == OBJ_SPACE || obj == OBJ_DIRT) {
          *atLeastOneAmoebaFoundThisFrameWhichCanGrow = true;
        }

        obj = GetObjectAtPosition(GetRelativePosition(positionOfAmoeba, down1));
        if (obj == OBJ_SPACE || obj == OBJ_DIRT) {
          *atLeastOneAmoebaFoundThisFrameWhichCanGrow = true;
        }
      }

      // If this amoeba decides to attempt to grow, it randomly chooses a direction, 
      // and if it can grow in that direction, does so.
      if (AmoebaRandomlyDecidesToGrow(anAmoebaRandomFactor)) {
        direction = GetRandomDirection();
        NewPosition = GetRelativePosition(positionOfAmoeba, direction);
        objectType obj = GetObjectAtPosition(NewPosition);
        if (obj == OBJ_SPACE || obj == OBJ_DIRT) {
          PlaceAmoeba(NewPosition);
        }
      }
    }
  } 
}

void initializeGame() {
  difficultyLevel = 0;
  score = 0;
  nextBonusLifeScore = BONUS_LIFE_PRICE;
  lives = 4;
  caveNumber = 0;

  rightIsDown = false;
  leftIsDown = false;
  upIsDown = false;
  downIsDown = false;
  spaceIsDown = false;
}

void initializeCave() {
  magicWallIsOn = false;

  caveTimeLeft = caveTime;
  caveTimeFrame = 0;

  diamondsCollected = 0;
  gotEnoughDiamonds = false;
  currentDiamondValue = initialDiamondValue;

  RockfordAnimationFacingDirection = facingRight;
  AnimationStage = 0;
  RockfordMoving = false;
  numRoundsSinceRockfordSeenAlive = 0;

  for (int y = 2; y <= 23; y++) {
    for(int x = 0; x <= 39; x++) {
      int mapY = y-2;
      int mapX = x;
      map[mapY][mapX] = caveData[y][x];
      if (map[mapY][mapX] == OBJ_PRE_ROCKFORD_STAGE_1) {
        RockfordLocation.x = mapX;
        RockfordLocation.y = mapY;
      }
    }
  }
}

void loadNewCave() {
  Cave *cave = getCave(++caveNumber);
  decodeCaveData(cave, caveData);

  initialDiamondValue = cave->initialDiamondValue;
  diamondsNeeded = cave->diamondsNeeded[difficultyLevel];
  extraDiamondValue = cave->extraDiamondValue;
  caveTime = cave->caveTime[difficultyLevel];

  initializeCave();
}

void updateMapCover() {
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
}

void updateCamera() {
  if (rockfordScreenRight > CAMERA_START_RIGHT_X) {
    cameraVelX = CAMERA_STEP;
  } else if (rockfordScreenLeft < CAMERA_START_LEFT_X) {
    cameraVelX = -CAMERA_STEP;
  }
  if (rockfordScreenBottom > CAMERA_START_DOWN_Y) {
    cameraVelY = CAMERA_STEP;
  } else if (rockfordScreenTop < CAMERA_START_UP_Y) {
    cameraVelY = -CAMERA_STEP;
  }

  if (rockfordScreenLeft >= CAMERA_STOP_LEFT_X && rockfordScreenRight <= CAMERA_STOP_RIGHT_X) {
    cameraVelX = 0;
  }
  if (rockfordScreenTop >= CAMERA_STOP_UP_Y && rockfordScreenBottom <= CAMERA_STOP_DOWN_Y) {
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

  int anAmoebaRandomFactor = 0;
  bool amoebaSuffocatedLastFrame = 0;
  bool atLeastOneAmoebaFoundThisFrameWhichCanGrow;
  int totalAmoebaFoundLastFrame = 0;
  int numberOfAmoebaFoundThisFrame;

  float turnTimer = 0;

  int cameraX = 0;
  int cameraY = 0;
  int cameraVelX = 0;
  int cameraVelY = 0;

  int diamondFrame = 0;
  int fireflyFrame = 0;

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

            case VK_SPACE:
              spaceIsDown = isDown;
              break;
          }
          break;

        default:
          TranslateMessage(&msg);
          DispatchMessage(&msg);
          break;
      }
    }

    int rockfordScreenLeft = PLAYFIELD_X_MIN + RockfordLocation.x * TILE_SIZE - cameraX;
    int rockfordScreenTop = PLAYFIELD_Y_MIN + RockfordLocation.y * TILE_SIZE - cameraY;
    int rockfordScreenRight = rockfordScreenLeft + TILE_SIZE;
    int rockfordScreenBottom = rockfordScreenTop + TILE_SIZE;

    turnTimer += dt;
    if (turnTimer >= TURN_DURATION) {
      turnTimer -= TURN_DURATION;

      diamondFrame++;
      if (diamondFrame == DIAMOND_FRAME_COUNT) {
        diamondFrame = 0;
      }

      fireflyFrame++;
      if (fireflyFrame == FIREFLY_FRAME_COUNT) {
        fireflyFrame = 0;
      }

      updateCamera();

      if (mapIsCovered()) {
        updateMapCover();
      } else {
        //
        // Foreground is not active
        //

        caveTimeFrame++;
        if (caveTimeFrame == CAVE_TIME_FRAME_MAX) {
          caveTimeFrame = 0;
          --caveTimeLeft;
          if (caveTimeLeft == 0) {
            isInit = true;
          }
        }

        for (int row = 0; row < CAVE_HEIGHT; ++row) {
          for (int col = 0; col < CAVE_WIDTH; ++col) {
            switch (map[row][col]) {
              case OBJ_FIREFLY_POSITION_1_SCANNED:
                map[row][col] = OBJ_FIREFLY_POSITION_1;
                break;
              case OBJ_FIREFLY_POSITION_2_SCANNED:
                map[row][col] = OBJ_FIREFLY_POSITION_2;
                break;
              case OBJ_FIREFLY_POSITION_3_SCANNED:
                map[row][col] = OBJ_FIREFLY_POSITION_3;
                break;
              case OBJ_FIREFLY_POSITION_4_SCANNED:
                map[row][col] = OBJ_FIREFLY_POSITION_4;
                break;
              case OBJ_BOULDER_STATIONARY_SCANNED:
                map[row][col] = OBJ_BOULDER_STATIONARY;
                break;
              case OBJ_BOULDER_FALLING_SCANNED:
                map[row][col] = OBJ_BOULDER_FALLING;
                break;
              case OBJ_DIAMOND_STATIONARY_SCANNED:
                map[row][col] = OBJ_DIAMOND_STATIONARY;
                break;
              case OBJ_DIAMOND_FALLING_SCANNED:
                map[row][col] = OBJ_DIAMOND_FALLING;
                break;
              case OBJ_BUTTERFLY_POSITION_1_SCANNED:
                map[row][col] = OBJ_BUTTERFLY_POSITION_1;
                break;
              case OBJ_BUTTERFLY_POSITION_2_SCANNED:
                map[row][col] = OBJ_BUTTERFLY_POSITION_2;
                break;
              case OBJ_BUTTERFLY_POSITION_3_SCANNED:
                map[row][col] = OBJ_BUTTERFLY_POSITION_3;
                break;
              case OBJ_BUTTERFLY_POSITION_4_SCANNED:
                map[row][col] = OBJ_BUTTERFLY_POSITION_4;
                break;
              case OBJ_ROCKFORD_SCANNED:
                map[row][col] = OBJ_ROCKFORD;
                break;
              case OBJ_AMOEBA_SCANNED:
                map[row][col] = OBJ_AMOEBA;
                break;
            }
          }
        }

        //
        // Scan map
        //

        for (int row = 0; row < CAVE_HEIGHT; ++row) {
          for (int col = 0; col < CAVE_WIDTH; ++col) {
            positionType scanPosition = MakePosition(row, col);
            switch (map[row][col]) {
              case OBJ_PRE_ROCKFORD_STAGE_1:
                ScanPreRockford(scanPosition, 1, timeTillBirth);
                break;
              case OBJ_PRE_ROCKFORD_STAGE_2:
                ScanPreRockford(scanPosition, 2, timeTillBirth);
                break;
              case OBJ_PRE_ROCKFORD_STAGE_3:
                ScanPreRockford(scanPosition, 3, timeTillBirth);
                break;
              case OBJ_PRE_ROCKFORD_STAGE_4:
                ScanPreRockford(scanPosition, 4, timeTillBirth);
                break;

              case OBJ_ROCKFORD:
                ScanRockford(scanPosition,
                             &RockfordLocation,
                             &RockfordAnimationFacingDirection,
                             &numRoundsSinceRockfordSeenAlive);
                break;

              case OBJ_AMOEBA:
                ScanAmoeba(scanPosition,
                           anAmoebaRandomFactor,
                           amoebaSuffocatedLastFrame,
                           &atLeastOneAmoebaFoundThisFrameWhichCanGrow,
                           totalAmoebaFoundLastFrame,
                           &numberOfAmoebaFoundThisFrame);
                break;
              
              case OBJ_BOULDER_STATIONARY:
                ScanStationaryBoulder(scanPosition);
                break;
              case OBJ_BOULDER_FALLING:
                ScanFallingBoulder(scanPosition, &magicWallIsOn);
                break;

              case OBJ_DIAMOND_STATIONARY:
                ScanStationaryDiamond(scanPosition);
                break;
              case OBJ_DIAMOND_FALLING:
                ScanFallingDiamond(scanPosition, &magicWallIsOn);
                break;

              case OBJ_PRE_OUTBOX:
                ScanPreOutBox(scanPosition);
                break;

              case OBJ_EXPLODE_TO_SPACE_STAGE_0:
                ScanExplosion(scanPosition, explodeToSpace, 0);
                break;
              case OBJ_EXPLODE_TO_SPACE_STAGE_1:
                ScanExplosion(scanPosition, explodeToSpace, 1);
                break;
              case OBJ_EXPLODE_TO_SPACE_STAGE_2:
                ScanExplosion(scanPosition, explodeToSpace, 2);
                break;
              case OBJ_EXPLODE_TO_SPACE_STAGE_3:
                ScanExplosion(scanPosition, explodeToSpace, 3);
                break;
              case OBJ_EXPLODE_TO_SPACE_STAGE_4:
                ScanExplosion(scanPosition, explodeToSpace, 4);
                break;
              case OBJ_EXPLODE_TO_DIAMOND_STAGE_0:
                ScanExplosion(scanPosition, explodeToDiamonds, 0);
                break;
              case OBJ_EXPLODE_TO_DIAMOND_STAGE_1:
                ScanExplosion(scanPosition, explodeToDiamonds, 1);
                break;
              case OBJ_EXPLODE_TO_DIAMOND_STAGE_2:
                ScanExplosion(scanPosition, explodeToDiamonds, 2);
                break;
              case OBJ_EXPLODE_TO_DIAMOND_STAGE_3:
                ScanExplosion(scanPosition, explodeToDiamonds, 3);
                break;
              case OBJ_EXPLODE_TO_DIAMOND_STAGE_4:
                ScanExplosion(scanPosition, explodeToDiamonds, 4);
                break;

              case OBJ_FIREFLY_POSITION_1:
                ScanFirefly(scanPosition, left);
                break;
              case OBJ_FIREFLY_POSITION_2:
                ScanFirefly(scanPosition, up);
                break;
              case OBJ_FIREFLY_POSITION_3:
                ScanFirefly(scanPosition, right);
                break;
              case OBJ_FIREFLY_POSITION_4:
                ScanFirefly(scanPosition, down);
                break;

              case OBJ_BUTTERFLY_POSITION_1:
                ScanButterfly(scanPosition, down);
                break;
              case OBJ_BUTTERFLY_POSITION_2:
                ScanButterfly(scanPosition, left);
                break;
              case OBJ_BUTTERFLY_POSITION_3:
                ScanButterfly(scanPosition, up);
                break;
              case OBJ_BUTTERFLY_POSITION_4:
                ScanButterfly(scanPosition, right);
                break;
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
        objectType tile = map[row][col];

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
            if (rockfordIsAppearing) {
              int atlCol = appearanceAnim[appearanceAnimFrame] % SPRITE_ATLAS_WIDTH_TILES;
              int atlRow = appearanceAnim[appearanceAnimFrame] / SPRITE_ATLAS_WIDTH_TILES;
              atlX = atlCol * TILE_SIZE;
              atlY = atlRow * TILE_SIZE;
            } else if (rockfordIsMoving) {
              atlX = 112 + rockfordMoveFrame * 16;
              atlY = 0;
              flipHorizontally = !rockfordIsFacingRight;
            } else {
              atlX = rockfordIdleFrame * 16;
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

    drawLine(CAMERA_START_RIGHT_X, 0, CAMERA_START_RIGHT_X, BACKBUFFER_HEIGHT - 1, COLOR_PINK);
    drawLine(CAMERA_STOP_RIGHT_X, 0, CAMERA_STOP_RIGHT_X, BACKBUFFER_HEIGHT - 1, COLOR_GREEN);

    drawLine(CAMERA_START_LEFT_X, 0, CAMERA_START_LEFT_X, BACKBUFFER_HEIGHT - 1, COLOR_PINK);
    drawLine(CAMERA_STOP_LEFT_X, 0, CAMERA_STOP_LEFT_X, BACKBUFFER_HEIGHT - 1, COLOR_GREEN);

    drawLine(0, CAMERA_START_DOWN_Y, BACKBUFFER_WIDTH - 1, CAMERA_START_DOWN_Y, COLOR_PINK);
    drawLine(0, CAMERA_STOP_DOWN_Y, BACKBUFFER_WIDTH - 1, CAMERA_STOP_DOWN_Y, COLOR_GREEN);

    drawLine(0, CAMERA_START_UP_Y, BACKBUFFER_WIDTH - 1, CAMERA_START_UP_Y, COLOR_PINK);
    drawLine(0, CAMERA_STOP_UP_Y, BACKBUFFER_WIDTH - 1, CAMERA_STOP_UP_Y, COLOR_GREEN);

    drawRect(rockfordScreenLeft, rockfordScreenTop, rockfordScreenRight, rockfordScreenBottom, COLOR_YELLOW);
#endif

    StretchDIBits(deviceContext, 0, 0, windowWidth, windowHeight,
                  0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT, backbuffer,
                  &backbufferBmpInf, DIB_RGB_COLORS, SRCCOPY);
  }
}
