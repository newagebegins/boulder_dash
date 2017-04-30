#include <windows.h>
#include <stdint.h>
#include <assert.h>
#include "cave.h"

void main() {
  HANDLE spritesFile = CreateFile("sprites.bmp", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  assert(spritesFile != INVALID_HANDLE_VALUE);

  HANDLE dataFile = CreateFile("boulder_dash.bdd", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  assert(dataFile != INVALID_HANDLE_VALUE);

  uint8_t buffer[2] = {'a','b'};

  BOOL result = WriteFile(dataFile, buffer, sizeof(buffer), NULL, NULL);
  assert(result);
}
