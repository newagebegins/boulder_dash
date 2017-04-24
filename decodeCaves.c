#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "decodeCaves.h"
#include "cavedata.h"

/* Creature code conversion table */
/* Converts the C64 BoulderDash codes into the codes used by Jeff Bevis's
   Amiga implementation of BoulderDash. */
UBYTE creatureCode[64]=
{
    ' ', '.', 'w', 'm', 'P', 'P', '?', 'W',
    'q', 'o', 'Q', 'O', 'q', 'o', 'Q', 'O',
    'r', 'r', 'r', 'r', 'd', 'd', 'd', 'd',
    '?', '?', '?', '?', '?', '?', '?', '?',
    '?', '?', '?', '?', '3', 'X', '5', '6',
    '?', '?', '?', '?', '?', '?', '?', '?',
    'B', 'c', 'b', 'C', 'B', 'c', 'b', 'C',
    '7', '8', 'a', 'a', '?', '?', '?', '?' 
};

/* DrawLine data */
/* When drawing lines, you can draw in all eight directions. This table gives
   the offsets needed to move in each of the 8 directions. */
int ldx[8]={ 0,  1, 1, 1, 0, -1, -1, -1};
int ldy[8]={-1, -1, 0, 1, 1,  1,  0, -1};

void StoreObject(int x, int y, objectType anObject);
void DrawLine(objectType anObject, int x, int y, int aLength, int aDirection);
void DrawFilledRect(objectType anObject, int x, int y, int aWidth, int aHeight, objectType aFillObject);
void DrawRect(objectType anObject, int x, int y, int aWidth, int aHeight);
void NextRandom(int *RandSeed1, int *RandSeed2);

UBYTE* caves[] = { 0, cave1, cave2, cave3, cave4, cave5, cave6, cave7, cave8, cave9, cave10, cave11, cave12, cave13, cave14, cave15, cave16, cave17, cave18, cave19, cave20 };

/* **************************************** */
void DecodeCave(int caveIndex)
{
    UBYTE *aCaveData = caves[caveIndex];
    int RandSeed1, RandSeed2;
    int theWidth, theHeight, theFill, theLength, theDirection;
    int x, y;
    int caveDataIndex;
    objectType theObject;
    int theCode;

    RandSeed1 = 0;
    RandSeed2 = aCaveData[4];

/* Clear out the cave data to a null value */
    for(x = 0; x < 40; x++) {
        for (y = 0; y <= 23; y++) {
            StoreObject(x, y, 0x07);
        }
    }

/* Decode the random cave data */
    for(y = 3; y <= 23; y++) {
        for(x = 0; x <= 39; x++) {
            theObject = 1;  /* Dirt */
            NextRandom(&RandSeed1, &RandSeed2);
            for (caveDataIndex = 0; caveDataIndex <= 3; caveDataIndex++) {
                if (RandSeed1 < aCaveData[0x1C + caveDataIndex]) {
                    theObject = aCaveData[0x18 + caveDataIndex];
                }
            }
            StoreObject(x, y, theObject);
        }     
    }  

/* Decode the explicit cave data */
    for (caveDataIndex = 0x20; aCaveData[caveDataIndex] != 0xFF; caveDataIndex++) {
        theCode = aCaveData[caveDataIndex];
        theObject = theCode & 0x3F;

        switch(3 & (aCaveData[caveDataIndex] >> 6)) {
        case 0: /* PLOT */
            x = aCaveData[++caveDataIndex];
            y = aCaveData[++caveDataIndex];
            StoreObject(x, y, theObject);
            break;

        case 1: /* LINE */
            x = aCaveData[++caveDataIndex];
            y = aCaveData[++caveDataIndex];
            theLength = aCaveData[++caveDataIndex];
            theDirection = aCaveData[++caveDataIndex];
            DrawLine(theObject, x, y, theLength, theDirection);
            break;

        case 2: /* FILLED RECTANGLE */
            x = aCaveData[++caveDataIndex];
            y = aCaveData[++caveDataIndex];
            theWidth = aCaveData[++caveDataIndex];
            theHeight = aCaveData[++caveDataIndex];
            theFill = aCaveData[++caveDataIndex];
            DrawFilledRect(theObject, x, y, theWidth, theHeight, (objectType)theFill);
            break;

        case 3: /* OPEN RECTANGLE */
            x = aCaveData[++caveDataIndex];
            y = aCaveData[++caveDataIndex];
            theWidth = aCaveData[++caveDataIndex];
            theHeight = aCaveData[++caveDataIndex];
            DrawRect(theObject, x, y, theWidth, theHeight);
            break;
        }
    }

/* SteelBounds */
    DrawRect(0x07, 0, 2, 40, 22);
}



void StoreObject(int x, int y, objectType anObject)
{
    assert(((x >= 0) && (x <= 39)));
    assert(((y >= 0) && (y <= 23)));
    assert(((anObject >= 0) && (anObject <= 63)));

    caveData[x][y] = creatureCode[anObject];
}

void DrawLine(objectType anObject, int x, int y, int aLength, int aDirection)
{
    int counter;

    assert(((anObject >= 0) && (anObject <= 63)));
    assert(((x >= 0) && (x <= 39)));
    assert(((y >= 0) && (y <= 23)));
    assert(((aLength >= 1) && (aLength <= 40)));
    assert(((aDirection >= 0) && (aDirection <= 7)));

    for(counter = 1; counter <= aLength; counter++) {
        StoreObject(x, y, anObject);
        x += ldx[aDirection];
        y += ldy[aDirection];
    }
}

void DrawFilledRect(objectType anObject, int x, int y, int aWidth, int aHeight, objectType aFillObject)
{
    int counter1;
    int counter2;

    assert(((anObject >= 0) && (anObject <= 63)));
    assert(((x >= 0) && (x <= 39)));
    assert(((y >= 0) && (y <= 23)));
    assert(((aWidth >= 1) && (aWidth <= 40)));
    assert(((aHeight >= 1) && (aHeight <= 24)));
    assert(((aFillObject >= 0) && (aFillObject <= 63)));

    for(counter1 = 0; counter1 < aWidth; counter1++) {
        for(counter2 = 0; counter2 < aHeight; counter2++) {
            if ((counter2 == 0) || (counter2 == aHeight-1)) {
                StoreObject(x+counter1, y+counter2, anObject);
            } else {
                StoreObject(x+counter1, y+counter2, aFillObject);
            }
        }
        StoreObject(x+counter1, y, anObject);
        StoreObject(x+counter1, y+aHeight-1, anObject);
    }
}

void DrawRect(objectType anObject, int x, int y, int aWidth, int aHeight)
{
    int counter1;

    assert(((anObject >= 0) && (anObject <= 63)));
    assert(((x >= 0) && (x <= 39)));
    assert(((y >= 0) && (y <= 23)));
    assert(((aWidth >= 1) && (aWidth <= 40)));
    assert(((aHeight >= 1) && (aHeight <= 24)));

    for(counter1 = 0; counter1 < aWidth; counter1++) {
        StoreObject(x + counter1, y, anObject);
        StoreObject(x + counter1, y + aHeight-1, anObject);
    }
    for(counter1 = 0; counter1 < aHeight; counter1++) {
        StoreObject(x, y + counter1, anObject);
        StoreObject(x + aWidth-1, y + counter1, anObject);
    }
}

void NextRandom(int *RandSeed1, int *RandSeed2)
{
    short TempRand1;
    short TempRand2;
    short carry;
    short result;

    assert(((*RandSeed1 >= 0x00) && (*RandSeed1 <= 0xFF)));
    assert(((*RandSeed2 >= 0x00) && (*RandSeed2 <= 0xFF)));

    TempRand1 = (*RandSeed1 & 0x0001) * 0x0080;	/* Bugfix! */
    TempRand2 = (*RandSeed2 >> 1) & 0x007F;

    result = (short)((*RandSeed2) + (*RandSeed2 & 0x0001) * 0x0080);
    carry = (result > 0x00FF);
    result = result & 0x00FF;

    result = result + carry + 0x13;
    carry = (result > 0x00FF);
    *RandSeed2 = result & 0x00FF;

    result = (short)(*RandSeed1 + carry + TempRand1);
    carry = (result > 0x00FF);
    result = result & 0x00FF;

    result = result + carry + TempRand2;
    *RandSeed1 = result & 0x00FF;

    assert(((*RandSeed1 >= 0x00) && (*RandSeed1 <= 0xFF)));
    assert(((*RandSeed2 >= 0x00) && (*RandSeed2 <= 0xFF)));
}
