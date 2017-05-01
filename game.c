#include "data_sprites.h"
#include "data_caves.h"
#include "game.h"
#include "graphics.h"

void gameUpdateAndRender(float dt) {
  UNREFERENCED_PARAMETER(dt);

  // Draw border
  drawFilledRect(0, 0, BACKBUFFER_WIDTH-1, BACKBUFFER_HEIGHT-1, 0);
  // Clear viewport
  drawFilledRect(VIEWPORT_X_MIN, VIEWPORT_Y_MIN, VIEWPORT_X_MAX, VIEWPORT_Y_MAX, 1);

  displayBackbuffer();
}
