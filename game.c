#include "game.h"
#include "graphics.h"
#include "cave.h"

static void gameUpdate(float dt) {
  UNREFERENCED_PARAMETER(dt);
  //Cave cave = decodeCave(0);
  // initializeMap
}

static void drawBorder() {
  drawFilledRect(0, 0, BACKBUFFER_WIDTH-1, BACKBUFFER_HEIGHT-1, 0);
}

static void clearViewport() {
  drawFilledRect(VIEWPORT_X_MIN, VIEWPORT_Y_MIN, VIEWPORT_X_MAX, VIEWPORT_Y_MAX, 1);
}

/* static void drawMap(Map *map) { */
/*   for (int y = 0; y < MAP_HEIGHT; ++y) { */
/*     for (int x = 0; x < MAP_WIDTH; ++x) { */
/*       switch (map[y][x]) { */
/*         case OBJ_STEEL_WALL: */
/*           break; */
/*       } */
/*     } */
/*   } */
/* } */

static void gameRender() {
  drawBorder();
  clearViewport();
  //drawMap();
  drawSprite(gSpriteSteelWall, 0, 0, 1, PALETTE_COLOR_1);
  drawSprite(gSpriteRockfordEye1, 0, 1, 1, PALETTE_COLOR_2);
  displayBackbuffer();
}

void gameUpdateAndRender(float dt) {
  gameUpdate(dt);
  gameRender();
}
