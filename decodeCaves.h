#ifndef DECODE_CAVES_H
#define DECODE_CAVES_H

/* **************************************** */
/* Types */
typedef unsigned char UBYTE;
typedef short objectType;

/* Global storage for the decoded cave data */
UBYTE caveData[40][24];

void DecodeCave(int caveIndex);

#endif
