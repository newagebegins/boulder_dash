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

void RequestSound(soundType sound) {
  // TODO: implement
  UNREFERENCED_PARAMETER(sound);
}

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
} positionType;

positionType MakePosition(int x, int y) {
  positionType result;
  result.x = x;
  result.y = y;
  return result;
}

objectType map[CAVE_HEIGHT][CAVE_WIDTH];

objectType GetObjectAtPosition(positionType position) {
  return map[position.y][position.x];
}

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

typedef enum {
  explodeToSpace,
  explodeToDiamonds,
} explosionType;

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

typedef enum {
  propertyRounded,
  propertyImpactExplosive,
} propertyType;

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
  // If the specified object is one which a boulder or diamond can roll off,
  // return true otherwise return false.

  // First of all, only objects which have the property of being "rounded" are
  // are ones which things can roll off. Secondly, if the object is a boulder
  // or diamond, the boulder or diamond must be stationary, not falling.

  // We're going to assume that GetObjectProperty() automatically returns "true"
  // for objBoulderStationary, objDiamondStationary, objBrickWall, and returns "false"
  // for everything else (including objBoulderFalling and objDiamondFalling).

  return GetObjectProperty(anObjectBelow, propertyRounded);
}

bool ImpactExplosive(objectType anObject) {
  // If the specified object has the property of being something that can
  // explode, return true otherwise return false.
  // ImpactExplosive objects are: Rockford, Firefly, Butterfly.
  return GetObjectProperty(anObject, propertyImpactExplosive);
}

explosionType GetExplosionType(objectType anObject) {
  // Assuming that the specified object is in fact explosive, returns the type
  // of explosion (explodeToSpace or explodeToDiamonds)
  // Explosive objects are: Rockford, Firefly, Butterfly.
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

typedef enum {
  kMagicWallOff,
  kMagicWallOn,
} magicWallStatusType;

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

void ScanFallingBoulder(positionType boulderPosition, magicWallStatusType *magicWallStatus) {
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
    if (*magicWallStatus == kMagicWallOff) {
      *magicWallStatus = kMagicWallOn;
    }
    if (*magicWallStatus == kMagicWallOn) {
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

void ScanFallingDiamond(positionType diamondPosition, magicWallStatusType *magicWallStatus) {
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
    if (*magicWallStatus == kMagicWallOff) {
      *magicWallStatus = kMagicWallOn;
    }
    if (*magicWallStatus == kMagicWallOn) {
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

typedef struct {
  int score;
  int nextBonusLifeScore;
  int lives;
  int currentDiamondValue;
  int diamondsCollected;
  bool gotEnoughDiamonds;
} playerType;

typedef struct {
  int diamondsNeeded;
  int extraDiamondValue;
} caveDataType;

typedef struct {
  directionType direction;
  bool fireButtonDown;
} joystickDirectionRecord;

playerType CurrentPlayerData;
caveDataType CaveData;

void AddLife() {
  assert(CurrentPlayerData.lives <= 9);

  if (CurrentPlayerData.lives < 9) {
    CurrentPlayerData.lives++;
    // TODO: Make space objects flash to indicate bonus life;
  }
}

void CheckForBonusLife()  {
  // Check to see whether the score has passed a 500 or a 1000 point boundary 
  // (in other words, you get a bonus life every 500 points).

  if (CurrentPlayerData.score >= CurrentPlayerData.nextBonusLifeScore) {
    AddLife();
    CurrentPlayerData.nextBonusLifeScore += 500;
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
  if (CurrentPlayerData.diamondsCollected == CaveData.diamondsNeeded) {
    CurrentPlayerData.gotEnoughDiamonds = true;
    CurrentPlayerData.currentDiamondValue = CaveData.extraDiamondValue;
    // TODO: Update statusbar;
    RequestSound(crackSound);
    // TODO: Request screen to flash white to indicate got enough diamonds;
  }
}

void PickUpDiamond() {
  // Player has picked up a diamond. Increase their score, increase their number 
  // of diamonds collected, and check whether they have enough diamonds now.

  RequestSound(pickedUpDiamondSound);
  CurrentPlayerData.score += CurrentPlayerData.currentDiamondValue;
  CheckForBonusLife();
  CurrentPlayerData.diamondsCollected++;
  CheckEnoughDiamonds();
}

joystickDirectionRecord GetNextDemoMovement() {
  // TODO: implement
  joystickDirectionRecord result = {0};
  return result;
}


bool rightIsDown = false;
bool leftIsDown = false;
bool upIsDown = false;
bool downIsDown = false;
bool spaceIsDown = false;

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

bool RockfordMoving;

typedef enum {
  facingRight,
  facingLeft,
} rockfordFacingType;

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
                        positionType *RockfordLocation,
                        joystickDirectionRecord JoyPos,
                        rockfordFacingType *RockfordAnimationFacingDirection) {
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
      *RockfordAnimationFacingDirection = facingRight;
      NewPosition = GetRelativePosition(currentScanPosition, right1);
      break;
    case left:
      RockfordMoving = true;
      *RockfordAnimationFacingDirection = facingLeft;
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
      *RockfordLocation = NewPosition;
    }
  }
}

void ScanRockford(positionType currentScanPosition,
                  positionType *RockfordLocation,
                  rockfordFacingType *RockfordAnimationFacingDirection,
                  bool demoMode,
                  int *numRoundsSinceRockfordSeenAlive) {
  // We have come across Rockford during the scan routine. Read the joystick or
  // demo data to find out where Rockford wants to go, and call a subroutine to
  // actually do it.

  assert(*numRoundsSinceRockfordSeenAlive >= 0);

  // Local variables
  joystickDirectionRecord JoyPos;

  // If we're in demo mode, we get our joystick movements from the demo data
  if (demoMode) {
    JoyPos = GetNextDemoMovement();
  } else {
    // Otherwise if we're in a real game, we get our joystick movements from
    // the current player's input device (joystick, keyboard, whatever).
    JoyPos = GetJoystickPos();
  }

  // Call a subroutine to actually deal with the joystick movement.
  MoveRockfordStage1(currentScanPosition, RockfordLocation, JoyPos, RockfordAnimationFacingDirection);

  // Rockford has been seen alive, so reset the counter indicating the number
  // of rounds since Rockford was last seen alive.
  *numRoundsSinceRockfordSeenAlive = 0;
}

int AnimationStage;

void AnimateRockford(bool *Tapping,
                     bool *Blinking,
                     bool RockfordAnimationFacingDirection) {
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

int kTooManyAmoeba;

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
  // kTooManyAmoeba = 200 for original Boulder Dash.
  if (totalAmoebaFoundLastFrame >= kTooManyAmoeba) {
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

  positionType RockfordLocation;
  rockfordFacingType RockfordAnimationFacingDirection;
  bool demoMode = false;
  int numRoundsSinceRockfordSeenAlive;

  int anAmoebaRandomFactor = 0;
  bool amoebaSuffocatedLastFrame = 0;
  bool atLeastOneAmoebaFoundThisFrameWhichCanGrow;
  int totalAmoebaFoundLastFrame = 0;
  int numberOfAmoebaFoundThisFrame;

  bool gotEnoughDiamonds = false;
  int caveTimeLeft = 0;
  int caveTimeTurn = 0;
  int caveTimeTurnMax = 7;
  int cameraX = 0;
  int cameraY = 0;
  float turnTimer = 0;

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
  int explosionFrame = 0;
  int explosionAnim[] = {0,1,0,2};
  int diamondFrame = 0;
  int fireflyFrame = 0;
  int timeTillBirth = 12;

  int score = 0;
  int diamondsCollected = 0;
  int borderColor = COLOR_BLACK;

  int foregroundVisibilityTurnMax = NO_ANIMATIONS ? 1 : 28;
  // TODO
  //int deathForegroundVisibilityTurnMax = NO_ANIMATIONS ? 1 : 25;

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
  magicWallStatusType magicWallStatus;

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
      magicWallStatus = kMagicWallOff;

      for (int y = 2; y <= 23; y++) {
        for(int x = 0; x <= 39; x++) {
          int mapY = y-2;
          int mapX = x;
          map[mapY][mapX] = caveData[y][x];
          if (map[mapY][mapX] == OBJ_PRE_ROCKFORD_STAGE_1) {
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
                             demoMode,
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
                ScanFallingBoulder(scanPosition, &magicWallStatus);
                break;

              case OBJ_DIAMOND_STATIONARY:
                ScanStationaryDiamond(scanPosition);
                break;
              case OBJ_DIAMOND_FALLING:
                ScanFallingDiamond(scanPosition, &magicWallStatus);
                break;

              case OBJ_PRE_OUTBOX:
                ScanPreOutBox(scanPosition, gotEnoughDiamonds);
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
