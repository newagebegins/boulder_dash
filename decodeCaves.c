/* decodeCaves.c */
/* This program decodes the Boulder Dash I cave data from the Commodore 64
   implementation into human-readable instructions (with minimal HTML
   formatting) and an ASCII diagram.
   
   Boulder Dash I by: Peter Liepa
   This program written by:
       Jeff Bevis <bevis@ecn.purdue.edu>
       Peter Broadribb <peterb@perth.dialix.oz.au>
   28 Aug 1995
   
   01 Oct 1995: Fixed bug in NextRandom(); I have now checked the output
   of NextRandom() against the output of the original 6510 code, and they
   appear to be generating the same numbers. However, the cave data,
   although _almost_ correct, doesn't seem exactly right. I'm puzzled. [PB]
   
   */



#include <stdio.h>
#include <stdlib.h>
/* #define NDEBUG /* Uncomment if you don't want assertion */
#include <assert.h>

#include "decodeCaves.h"


/* **************************************** */
/* Compilation flags */
#define INCLUDE_HUMAN_READABLE 0    /* To include human-readable stuff in the output */


/* **************************************** */
/* Global data */

/* Coded cave data */
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


/* Human-readable names for the 64 different C64 object codes */
char *objects[]=
{
    "zSpace", "zDirt", "zBrick", "zMagic", "zPreOut", "zOutBox", "*", "zSteel",
    "zFFly1", "zFFly2", "zFFly3", "zFFly4", "zFFly1M", "zFFly2M", "zFFly3M",
    "zFFly4M", "zBouS", "zBouSM", "zBouF", "zBouFM", "zDiaS", "zDiaSM", "zDiaF",
    "zDiaFM", "", "", "", "zExp1S", "zExp2S", "zExp3S", "zExp4S", "zExp5S",
    "zExp1D", "zExp2D", "zExp3D", "zExp4D", "zExp5D", "zPRFd1", "zPRFd2", "zPRFd3",
    "zPRFd4", "", "", "", "", "", "", "", "zBFly1", "zBFly2",
    "zBFly3", "zBFly4", "zBFly1M", "zBFly2M", "zBFly3M", "zBFly4M", "zRFd",
    "zRFdM", "zAmoe", "zAmoeM", "", "", "*", "*"
};


/* DrawLine data */
/* When drawing lines, you can draw in all eight directions. This table gives
   the offsets needed to move in each of the 8 directions. */
int ldx[8]={ 0,  1, 1, 1, 0, -1, -1, -1};
int ldy[8]={-1, -1, 0, 1, 1,  1,  0, -1};


/* Human-readable versions of the 8 directions */
char *directions[]=
{
    "up","up/right","right","down/right","down","down/left","left","up/left"
};


/* C64 color names */
char *colors[]=
{
    "Black", "White", "Red", "Cyan", "Purple", "Green", "Blue", "Yellow", "Orange",
    "Brown", "Light red", "Gray 1", "Gray 2", "Light green", "Light blue", "Gray 3"
};

char *caveNames[]=
{
    "Intro",      "Rooms",        "Maze",   "Butterflies",
    "Guards",     "Firefly dens", "Amoeba", "Enchanted wall",
    "Greed",      "Tracks",       "Crowd",  "Walls",
    "Apocalypse", "Zigzag",       "Funnel", "Enchanted boxes"
};

char *caveDescriptions[]=
{
    "Pick up jewels and exit before time is up",
    "Pick up jewels, but you must move boulders to get all jewels",
    "Pick up jewels. You must get every jewel to exit",
    "Drop boulders on butterflies to create jewels",
    "The jewels are there for grapping, but they are guarded by the deadly fireflies",
    "Each firefly is guarding a jewel",
    "Surround the amoeba with boulders, so it can't grow anymore. Pick up jewels that are created when it suffocates",
    "Activate the enchanted wall and create as many jewels as you can",
    "You have to get a lot of jewels here, lucky there are so many",
    "Get the jewels, avoid the fireflies",
    "You must move a lot of boulders around in some tight spaces",
    "You must blast hrough walls to get at some of the jewels. Drop a boulder on a firefly at the right time and place to do this",
    "Bring the butterflies and amoeba together and watch the jewels fly",
    "Magically transform the butterflies into jewels, but don't waste any boulders and watch out the fireflies",
    "There is an enchanted wall at the bottom of the rock tunnel",
    "The top of each square room is an enchanted wall, but you'll have to blast your way inside"
};


/* Output file */
FILE *theFile;




/* **************************************** */
/* Prototypes */
void main(void);
void StoreObject(int x, int y, objectType anObject);
void DrawLine(objectType anObject, int x, int y, int aLength, int aDirection);
void DrawFilledRect(objectType anObject, int x, int y, int aWidth, int aHeight, objectType aFillObject);
void DrawRect(objectType anObject, int x, int y, int aWidth, int aHeight);
void NextRandom(int *RandSeed1, int *RandSeed2);



/* **************************************** */
#if 0
void main(void)
{
    theFile = fopen("decodedCaves.html","w");
	fprintf(theFile, "<HTML>\n");
	fprintf(theFile, "<HEAD><TITLE>BoulderDash Decoded cave data</TITLE>\n");
	fprintf(theFile, "</HEAD>\n");
	fprintf(theFile, "\n");
	fprintf(theFile, "<BODY>\n");
	fprintf(theFile, "<H1>BoulderDash Decoded cave data</H1>\n");
	fprintf(theFile, "\n");
	fprintf(theFile, "<UL>\n");
	fprintf(theFile, "\t<LI>Cave <A HREF=\"#Cave A\">A</A> <A HREF=\"#Cave B\">B</A> <A HREF=\"#Cave C\">C</A> <A HREF=\"#Cave D\">D</A> <A HREF=\"#Intermission 1\">Intermission 1</A>\n");
	fprintf(theFile, "\t<LI>Cave <A HREF=\"#Cave E\">E</A> <A HREF=\"#Cave F\">F</A> <A HREF=\"#Cave G\">G</A> <A HREF=\"#Cave H\">H</A> <A HREF=\"#Intermission 2\">Intermission 2</A>\n");
	fprintf(theFile, "\t<LI>Cave <A HREF=\"#Cave I\">I</A> <A HREF=\"#Cave J\">J</A> <A HREF=\"#Cave K\">K</A> <A HREF=\"#Cave L\">L</A> <A HREF=\"#Intermission 3\">Intermission 3</A>\n");
	fprintf(theFile, "\t<LI>Cave <A HREF=\"#Cave M\">M</A> <A HREF=\"#Cave N\">N</A> <A HREF=\"#Cave O\">O</A> <A HREF=\"#Cave P\">P</A> <A HREF=\"#Intermission 4\">Intermission 4</A>\n");
	fprintf(theFile, "</UL>\n");
	fprintf(theFile, "\n");
	fprintf(theFile, "Decoded cave data in human-readable text, including ascii representation. Warning: There may be mistakes in the decode. The C source code to generate the following decodes are available in the files ");
	fprintf(theFile, "<A HREF=\"decodecaves.c\">decodecaves.c</A> and <A HREF=\"cavedata.h\">cavedata.h</A>). Note that when decoding the cave, the random objects are placed first, then a bounding rectangle of steel wall ");
	fprintf(theFile, "is put in (effectively an implicit DrawRect(0,2,40,22) before the cave data), then the cave data is decoded. For each cave, a graphical representation is given for difficulty level 1. The characters ");
	fprintf(theFile, "used in the graphical representation are the ones used by a implementation of BoulderDash on the Amiga.\n");


    DecodeCave(cave1);
    DecodeCave(cave2);
    DecodeCave(cave3);
    DecodeCave(cave4);
    DecodeCave(cave5);
    DecodeCave(cave6);
    DecodeCave(cave7);
    DecodeCave(cave8);
    DecodeCave(cave9);
    DecodeCave(cave10);
    DecodeCave(cave11);
    DecodeCave(cave12);
    DecodeCave(cave13);
    DecodeCave(cave14);
    DecodeCave(cave15);
    DecodeCave(cave16);
    DecodeCave(cave17);
    DecodeCave(cave18);
    DecodeCave(cave19);
    DecodeCave(cave20);

	fprintf(theFile, "\n");
	fprintf(theFile, "\n");
	fprintf(theFile, "<HR>\n");
	fprintf(theFile, "<ADDRESS>Web page design by <A HREF=\"mailto:peterb@perth.dialix.oz.au\">Peter Broadribb</A></ADDRESS>\n");
	fprintf(theFile, "</BODY>\n");
	fprintf(theFile, "</HTML>\n");
    fclose(theFile);
}
#endif

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

/* Output human-readable version of the first 0x20 bytes of cave data. */
    #if INCLUDE_HUMAN_READABLE
        if (aCaveData[0] <= 16) {
            fprintf(theFile,"\n\n<H2>Cave %d (<A NAME=\"Cave %c\">Cave %c</A>: %s)</H2>\n%s\n",aCaveData[0],aCaveData[0]+'A'-1,aCaveData[0]+'A'-1,caveNames[aCaveData[0]-1],caveDescriptions[aCaveData[0]-1]);
        } else {
            fprintf(theFile,"\n\n<H2>Cave %d (<A NAME=\"Intermission %d\">Intermission %d</A>)</H2>\n",aCaveData[0],aCaveData[0]-16,aCaveData[0]-16);
        }
        fprintf(theFile,"<PRE>%.2X                  Cave %.2d\n",aCaveData[0],aCaveData[0]);
        fprintf(theFile,"%.2X                  Magic wall/amoeba slow growth for: %d seconds\n",aCaveData[1],aCaveData[1]);
        fprintf(theFile,"%.2X                  Diamonds worth: %d points\n",aCaveData[2],aCaveData[2]);
        fprintf(theFile,"%.2X                  Extra diamonds worth: %d points\n",aCaveData[3],aCaveData[3]);
        fprintf(theFile,"%.2X %.2X %.2X %.2X %.2X      Randomiser seed values for difficulty levels 1-5\n",aCaveData[4],aCaveData[5],aCaveData[6],aCaveData[7],aCaveData[8]);
        fprintf(theFile,"%.2X %.2X %.2X %.2X %.2X      Diamonds needed: %d, %d, %d, %d, %d (for difficulty levels 1-5)\n",aCaveData[9],aCaveData[10],aCaveData[11],aCaveData[12],aCaveData[13],aCaveData[9],aCaveData[10],aCaveData[11],aCaveData[12],aCaveData[13]);
        fprintf(theFile,"%.2X %.2X %.2X %.2X %.2X      Cave time: %d, %d, %d, %d, %d seconds\n",aCaveData[14],aCaveData[15],aCaveData[16],aCaveData[17],aCaveData[18],aCaveData[14],aCaveData[15],aCaveData[16],aCaveData[17],aCaveData[18]);
        fprintf(theFile,"%.2X                  Background color 1: %s\n",aCaveData[19],colors[aCaveData[19]]);
        fprintf(theFile,"%.2X                  Background color 2: %s\n",aCaveData[20],colors[aCaveData[20]]);
        fprintf(theFile,"%.2X                  Foreground color: %s\n",aCaveData[21],colors[aCaveData[21]]);
        fprintf(theFile,"%.2X %.2X               Unused\n",aCaveData[22],aCaveData[23]);
        fprintf(theFile,"%.2X %.2X %.2X %.2X         Random objects:\n",aCaveData[24],aCaveData[25],aCaveData[26],aCaveData[27]);
        if (aCaveData[28]) {
            fprintf(theFile,"%.2X %.2X %.2X %.2X             %-7s: %3d/256 = %2d%%\n",aCaveData[28],aCaveData[29],aCaveData[30],aCaveData[31], objects[aCaveData[24]],aCaveData[28],(int)aCaveData[28]*100/256);
        } else {
            fprintf(theFile,"%.2X %.2X %.2X %.2X             first code unused (0%%)\n",aCaveData[28],aCaveData[29],aCaveData[30],aCaveData[31]);
        }
        if (aCaveData[29]) {
            fprintf(theFile,"                        %-7s: %3d/256 = %2d%%\n", objects[aCaveData[25]],aCaveData[29],(int)aCaveData[29]*100/256);
        } else {
            fprintf(theFile,"                        second code unused (0%%)\n");
        }
        if (aCaveData[30]) {
            fprintf(theFile,"                        %-7s: %3d/256 = %2d%%\n", objects[aCaveData[26]],aCaveData[30],(int)aCaveData[30]*100/256);
        } else {
            fprintf(theFile,"                        third code unused (0%%)\n");
        }
        if (aCaveData[31]) {
            fprintf(theFile,"                        %-7s: %3d/256 = %2d%%\n", objects[aCaveData[27]],aCaveData[31],(int)aCaveData[31]*100/256);
        } else {
            fprintf(theFile,"                        fourth code unused (0%%)\n");
        }
        fprintf(theFile,"Cave layout:\n");
    #endif

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
            #if INCLUDE_HUMAN_READABLE
                fprintf(theFile,"%.2X %.2X %.2X            StoreChar %s at (%2d,%2d)\n",theCode,x,y,objects[theCode & 0x3f],x,y);
            #endif
            StoreObject(x, y, theObject);
            break;

        case 1: /* LINE */
            x = aCaveData[++caveDataIndex];
            y = aCaveData[++caveDataIndex];
            theLength = aCaveData[++caveDataIndex];
            theDirection = aCaveData[++caveDataIndex];
            #if INCLUDE_HUMAN_READABLE
                fprintf(theFile,"%.2X %.2X %.2X %.2X %.2X      Line of %s from (%2d,%2d); length = %d; direction = %s\n",theCode,x,y,theLength,theDirection,objects[theCode & 0x3f],x,y,theLength,directions[theDirection]);
            #endif
            DrawLine(theObject, x, y, theLength, theDirection);
            break;

        case 2: /* FILLED RECTANGLE */
            x = aCaveData[++caveDataIndex];
            y = aCaveData[++caveDataIndex];
            theWidth = aCaveData[++caveDataIndex];
            theHeight = aCaveData[++caveDataIndex];
            theFill = aCaveData[++caveDataIndex];
            #if INCLUDE_HUMAN_READABLE
                fprintf(theFile,"%.2X %.2X %.2X %.2X %.2X %.2X   FilledRect of %s from (%2d,%2d); length = %d; height = %d; fill = %s\n",theCode,x,y,theWidth,theHeight,aCaveData[caveDataIndex],objects[theCode & 0x3f],x,y,theWidth,theHeight,objects[aCaveData[caveDataIndex]]);
            #endif
            DrawFilledRect(theObject, x, y, theWidth, theHeight, (objectType)theFill);
            break;

        case 3: /* OPEN RECTANGLE */
            x = aCaveData[++caveDataIndex];
            y = aCaveData[++caveDataIndex];
            theWidth = aCaveData[++caveDataIndex];
            theHeight = aCaveData[++caveDataIndex];
            #if INCLUDE_HUMAN_READABLE
                fprintf(theFile,"%.2X %.2X %.2X %.2X %.2X      Rect of %s from (%2d,%2d); length = %d; height = %d\n",theCode,x,y,theWidth,theHeight,objects[theCode & 0x3f],x,y,theWidth,theHeight);
            #endif
            DrawRect(theObject, x, y, theWidth, theHeight);
            break;
        }
    }

/* SteelBounds */
    DrawRect(0x07, 0, 2, 40, 22);

/* Print out the resulting aCaveData */
    #if INCLUDE_HUMAN_READABLE
        fprintf(theFile,"FF                  End of cave data.");
    #endif
    #if 0
    fprintf(theFile,"\n\nMap #%d\n",aCaveData[0]);
    for (y = 2; y <= 23; y++) {
        fprintf(theFile, "%03d ", y);
        for(x = 0; x <= 39; x++) {
            fprintf(theFile, "%c", caveData[x][y]); 
        }
        if (y == 23) {
            fprintf(theFile, "</PRE>");
        }
        fprintf(theFile, "\n");
    }
    #endif
}



/* **************************************** */
void StoreObject(int x, int y, objectType anObject)
{
    assert(((x >= 0) && (x <= 39)));
    assert(((y >= 0) && (y <= 23)));
    assert(((anObject >= 0) && (anObject <= 63)));

    caveData[x][y] = creatureCode[anObject];
}



/* **************************************** */
void DrawLine(objectType anObject, int x, int y, int aLength, int aDirection)
{
/* Local variables */
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



/* **************************************** */
void DrawFilledRect(objectType anObject, int x, int y, int aWidth, int aHeight, objectType aFillObject)
{
/* Local variables */
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



/* **************************************** */
void DrawRect(objectType anObject, int x, int y, int aWidth, int aHeight)
{
/* Local variables */
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



/* **************************************** */
void NextRandom(int *RandSeed1, int *RandSeed2)
/* This is the mathematical random number generator from the Commodore 64
   implementation of Boulder Dash I. The 6510 disassembly is given in
   comments, and the C translation follows. */
{
    short TempRand1;
    short TempRand2;
    short carry;
    short result;

    assert(((*RandSeed1 >= 0x00) && (*RandSeed1 <= 0xFF)));
    assert(((*RandSeed2 >= 0x00) && (*RandSeed2 <= 0xFF)));

/*
            7085 NextRandom.00  LDA RandSeed1
            7087                ROR
            7088                ROR
            7089                AND #$80
            708B                STA TempRand1
        
            Note: ROR on the 6510 works like this:
              7-->6-->5-->4-->3-->2-->1-->0-->C
              ^                               |
              |_______________________________|
            In other words, it's a nine-bit rotate. Thus it takes two RORs to shift
            the low bit (bit zero) into the high bit (bit 7).
*/
    TempRand1 = (*RandSeed1 & 0x0001) * 0x0080;	/* Bugfix! */


        /*
            708E                LDA RandSeed2
            7090                ROR
            7091                AND #$7F
            7093                STA TempRand2
        */
    TempRand2 = (*RandSeed2 >> 1) & 0x007F;


        /*
            7096                LDA RandSeed2
            7098                ROR
            7099                ROR
            709A                AND #$80
            709C                CLC
            709D                ADC RandSeed2
        */
    result = (short)((*RandSeed2) + (*RandSeed2 & 0x0001) * 0x0080);
    carry = (result > 0x00FF);
    result = result & 0x00FF;


        /*
            709F                ADC #$13
            70A1                STA RandSeed2
        */
    result = result + carry + 0x13;
    carry = (result > 0x00FF);
    *RandSeed2 = result & 0x00FF;


        /*
            70A3                LDA RandSeed1
            70A5                ADC TempRand1
        */
    result = (short)(*RandSeed1 + carry + TempRand1);
    carry = (result > 0x00FF);
    result = result & 0x00FF;


        /*
            70A8                ADC TempRand2
            70AB                STA RandSeed1
            70AD                RTS
        */
    result = result + carry + TempRand2;
    *RandSeed1 = result & 0x00FF;

    assert(((*RandSeed1 >= 0x00) && (*RandSeed1 <= 0xFF)));
    assert(((*RandSeed2 >= 0x00) && (*RandSeed2 <= 0xFF)));
}



/* **************************************** */
