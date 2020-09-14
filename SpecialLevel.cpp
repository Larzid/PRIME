#include <stdlib.h>
#include <string.h>
#include "Util.h"
#include "Hero.h"
#include "Map.h"
#include "Monster.h"
#include "Object.h"

#include "MapBuilder.h"


/******

 ##########               #######       #######                  
 #........#        ########X#X#X#########.....#                  
 #.....<..#        #????(#########=+/]!)#.....#                  
 #........#        #?????#.......#=?+()!#.....#                  
 ###'######        #..X..#.......#...X..#'#####                  
   #.#             ###'######+#######'###.'.++#                  
   #.#  ########   #....................+.#X++#                  
   #.####......#   #....................#.#######                
   #....+......#   #........#######.....#.#{.{.{#  ######        
   #.####......#####........#.X...#.....#.#.....#  #....#        
   #.#  #....X.+............#.....#.....#.###'######....#        
   #.#  #......#####........#.....#.....................#        
   #.#  ########   #........###'###.....####'#######....#        
   #.#             #....................#.....#    #.>..#        
####+#####         #....................'.....#    #....#        
#........#         ###########...########.....#    ######        
#.....>..#         #.........#...#.(((((#######                  
#........#         #.........'...'X(((((#                        
##########         ######################                        

*********/

void
shMapLevel::buildTown ()
{
    const int LOCS = 6;
    int shopLoc[LOCS][4] =
    {
        {19, 2, 25, 6},        /* software store */
        {33, 2, 40, 6},        /* general store */
        {40, 1, 46, 5},        /* next to general store */
        {33, 16, 40, 19},      /* hardware store */
        {19, 16, 29, 19},      /* bottom left empty shop */
        {28, 9, 34, 13}        /* center room */
    };
//#define PANELS
#ifdef PANELS
    const int panels[][2] =
    {
        {20, 6}, {24, 6},      /* software store */
        {34, 6}, {35, 6}, {39, 6},    /* general store */
        {42, 9}, {42, 10}, {43, 11}, {47, 11},    /* vats room */
        {35, 16}, {36, 16}, {37, 16}, {38, 16},   /* hardware store */
        /* bottom left empty shop */
        {21, 16}, {22, 16}, {24, 16}, {25, 16}, {27, 16}, {28, 16},
        /* center room */
        {30, 9}, {31, 9}, {32, 9}, {28, 11}, {34, 11}, {29, 13}, {33, 13}
    };
    int npanels = sizeof (panels) / (2 * sizeof (int));
#endif

    mMapType = kTown;

    layRoom (19, 6, 40, 19);       /* main room */
    layRoom (28, 9, 34, 13);       /* center room */
    layRoom (1, 1, 10, 5);         /* upper left */
    layRoom (8, 7, 15, 13);        /* guard room */
    layRoom (0, 15, 9, 19);        /* bottom left */
    layRoom (19, 2, 25, 6);        /* software store */
    layRoom (25, 3, 33, 6);        /* middle top */
    layRoom (33, 2, 40, 6);        /* general store */
    layRoom (19, 16, 29, 19);      /* bottom left empty shop */
    layRoom (33, 16, 40, 19);      /* hardware store */
    layRoom (40, 1, 46, 5);        /* next to general store */
    layRoom (42, 8, 48, 11);       /* room with three vats */
    layRoom (42, 5, 46, 8);        /* room directly above vats */
    layRoom (51, 9, 56, 16);       /* sewer entrance room */
    layRoom (40, 13, 46, 17);      /* next to hardware store */
    layRoom (26, 1, 28, 3);        /* left guardbot niche */
    layRoom (28, 1, 30, 3);        /* middle guardbot niche */
    layRoom (30, 1, 32, 3);        /* right guardbot niche */

    layCorridor (4, 5, 4, 15);
    layCorridor (5, 9, 8, 9);
    layCorridor (15, 11, 19, 11);

    layCorridor (40, 12, 51, 12);
    layCorridor (41, 6, 41, 11);


    addDoor (4, 5, 1, 0, 0, -1);
    addDoor (4, 15, 1, 0, 0, -1);
    addDoor (8, 9, 0, 0, 0, 0);
    addDoor (15, 11, 0, 0, 0, 0);
    addDoor (19, 11, 0, 0, 0, 0);

    addDoor (22, 6, 1, 0, 0, 0);
    addDoor (29, 6, 1, 0, 0, 0);
    addDoor (37, 6, 1, 0, 0, 0);

    addDoor (40, 7, 0, 0, 0, 0);
    addDoor (41, 5, 1, 0, 0, 0);
    addDoor (42, 6, 1, 0, 0, 0);
    addDoor (40, 15, 0, 0, 0, 0);

    addDoor (45, 11, 1, 0, 0, 0);
    addDoor (44, 13, 1, 0, 0, 0);

    addDoor (29, 18, 1, 0, 0, 0);
    addDoor (33, 18, 1, 0, 0, 0);
    addDoor (31, 13, 1, 0, 0, 0);

    addDoor (27, 3, 1, 0, 0, 1, 0, 0); /* Guardbot niche doors. */
    addDoor (29, 3, 1, 0, 0, 1, 0, 0);
    addDoor (31, 3, 1, 0, 0, 1, 0, 0);

    mundaneRoom (1, 1, 10, 5);
    mundaneRoom (8, 7, 15, 13);
    mundaneRoom (0, 15, 9, 19);
    addVat (43, 9);
    addVat (45, 9);
    addVat (47, 9);

#ifdef PANELS
    for (int i = 0; i < npanels; ++i)
        SETSQ (panels[i][0], panels[i][1], kGlassPanel);
#endif

    /* Assign shops to predefined locations. */
    shuffle (shopLoc, LOCS, sizeof (int [4]));
    makeShop (shopLoc[0][0], shopLoc[0][1], shopLoc[0][2], shopLoc[0][3],
        shRoom::kSoftwareStore);
    makeShop (shopLoc[1][0], shopLoc[1][1], shopLoc[1][2], shopLoc[1][3],
        shRoom::kGeneralStore);
    makeShop (shopLoc[2][0], shopLoc[2][1], shopLoc[2][2], shopLoc[2][3],
        shRoom::kHardwareStore);
    makeHospital (shopLoc[3][0], shopLoc[3][1], shopLoc[3][2], shopLoc[3][3]);
    if (RNG (2)) {
        makeShop (shopLoc[4][0], shopLoc[4][1], shopLoc[4][2], shopLoc[4][3],
            shRoom::kWeaponStore);
    } else {
        makeShop (shopLoc[4][0], shopLoc[4][1], shopLoc[4][2], shopLoc[4][3],
            shRoom::kArmorStore);
    }
    makeShop (shopLoc[5][0], shopLoc[5][1], shopLoc[5][2], shopLoc[5][3],
        shRoom::kCanisterStore);

    /* Smallest store to generate not too many implants. */
    makeShop (42, 5, 46, 8, shRoom::kImplantStore);

    /* Robot Town *has to* have a guaranteed computer.  Games without
       a single computer generated could be possible otherwise. */
    shObject *comp = NULL;
    /* First, check for existing computers. */
    for (int x = 0; x < MAPMAXCOLUMNS; ++x)
        for (int y = 0; y < MAPMAXROWS; ++y) {
            shObjectVector *v = getObjects (x, y);
            if (!v)  continue;
            for (int i = 0; i < v->count (); ++i)
                if (v->get (i)->has_subtype (computer)) {
                    comp = v->get (i);
                    goto done_searching_for_computer;
                }
        }
    done_searching_for_computer: ;
    if (!comp) { /* Oh, there was no computer randomly generated. */
        comp = gen_obj_type (prob::Computer);
        comp->set (obj::unpaid);
        /* Select a location (watch out for walls!). */
        int x = RNG (shopLoc[0][0] + 1, shopLoc[0][2] - 1);
        int y = RNG (shopLoc[0][1] + 1, shopLoc[0][3] - 1);
        putObject (comp, x, y);
        debug.log ("Explicitly placing a computer at (%d,%d).", x, y);
    } /* Supply the machine with an OS. */
    if (comp->mEnhancement == kNoSys)
        comp->mEnhancement = RNG (kAbysmalSys, kSuperbSys);

    /* Backup guards in case of emergency. */
    for (int i = 0; i < 3; ++i) {
        shCreature *guard = shCreature::monster (kMonGuardbot);
        putCreature (guard, 27 + 2 * i, 2);
        guard->mGuard.mToll = -1;
        guard->mGuard.mChallengeIssued = 0;
    }

    /* Guardian that will extract toll from visitors to Robot Town. */
    shCreature *guard = shCreature::monster (kMonGuardbot);
    putCreature (guard, 12, 11);
    guard->mGuard.mSX = 15;
    guard->mGuard.mSY = 0;
    guard->mGuard.mEX = 59;
    guard->mGuard.mEY = 19;
    guard->mGuard.mToll = 300;
    guard->mGuard.mChallengeIssued = 0;
}

/******

The Killer Rabbit Level

*********/

void
shMapLevel::buildRabbitLevel ()
{
    mMapType = kRabbit;

    layRoom (15, 5, 45, 15);
    layRoom (45, 9, 47, 11);
    addDoor (45, 10, 0, 0, 1, 0, 1, 1, 1);

    shObject *pills = new shObject (kObjBluePill);
    pills->mCount = 2;
    putObject (pills, 46, 10);

    shCreature *rabbit = shCreature::monster (kMonKillerRabbit);
    putCreature (rabbit, 40, 10);

    mFlags |= shMapLevel::kNoTransport;
    mFlags |= shMapLevel::kNoDig;
}

/******

The Waste Treatment Plant Level

guaranteed items: radiation suit, ordinary jumpsuit, fusion power plant


#############################################
#
#
#           1         2         3         4         5         6
  0123456789012345678901234567890123456789012345678901234567890123

0   ####   ####        ####   ####        ########################
1  #....# #....# ######~~~~# #~~~~######  #======#...............#
2  #.<..# #....# #....+~~~~# #~~~~+....#  #~~~~~~#.~~~~~~~~~~~~~~~
3  #....# #....# #..###~~~~# #~~~~###..####~~~~~~#.~~~~~~~~~~~~~~~
4   #..#   #..#  #..#  ####   ####  #.....+~~~~~~+.~~~~~~~~~~~~~~~
5   #..#   #..#  #..#               #..####~~~~~~#.~~~~~~~~~~~~~~~
6   #..#   #..#  #..#  ####   ####  #..#  #~~~~~~#.~~~~~~~~~~~~~~~
7   #..#####..#  #..###....# #~~~~###..#  #======#............~~~#
8   #.........#  #....+....# #~~~~+....#  ########............~~~#
9   ########..#  #..###....# #~~~~###..#         #............~~~#
0          #..#  #..#  ####   ####  #..###########...........#~~~#
1    ####  #..#  #..#               #............+...........#~~~#
2   #~~~~# #..####..#################..#############+#####+###~~~#
3   #~~~~# #...........................#         #.....#.....#~~~#
4   #~~~~# #############################         #..[..#..[..#~~~#
5    #~~#                                        #.....#.....#~~~#
6 ####~~######################################################~~~#
7 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
8 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
9 ################################################################

  0123456789012345678901234567890123456789012345678901234567890123

*********/


void
shMapLevel::buildSewerPlant ()
{
    mMapType = kSewerPlant;

    fillRect (2, 0, 5, 0, kSewerWall1);
    fillRect (9, 0, 12, 0, kSewerWall1);
    fillRect (21, 0, 24, 0, kSewerWall1);
    fillRect (28, 0, 31, 0, kSewerWall1);
    fillRect (1, 1, 6, 3, kSewerWall1);
    fillRect (8, 1, 13, 3, kSewerWall1);
    fillRect (2, 12, 7, 14, kSewerWall1);
    fillRect (3, 11, 6, 15, kSewerWall1);
    fillRect (2, 4, 5, 9, kSewerWall1);
    fillRect (9, 1, 12, 12, kSewerWall1);
    fillRect (6, 7, 8, 9, kSewerWall1);
    fillRect (9, 12, 37, 14, kSewerWall1);
    fillRect (15, 1, 18, 12, kSewerWall1);
    fillRect (18, 1, 25, 3, kSewerWall1);
    fillRect (21, 4, 24, 4, kSewerWall1);
    fillRect (19, 7, 25, 9, kSewerWall1);
    fillRect (21, 6, 24, 10, kSewerWall1);
    fillRect (27, 1, 33, 3, kSewerWall1);
    fillRect (34, 1, 37, 12, kSewerWall1);
    fillRect (28, 4, 32, 4, kSewerWall1);
    fillRect (27, 7, 33, 9, kSewerWall1);
    fillRect (28, 6, 31, 10, kSewerWall1);
    fillRect (37, 3, 39, 5, kSewerWall1);
    fillRect (37, 10, 46, 12, kSewerWall1);
    fillRect (40, 0, 47, 8, kSewerWall1);
    fillRect (47, 0, 63, 15, kSewerWall1);
    fillRect (0, 16, 63, 19, kSewerWall1);

    fillRect (2, 1, 5, 3, kSewerFloor);
    fillRect (3, 4, 4, 7, kSewerFloor);
    fillRect (3, 8, 9, 8, kSewerFloor);
    fillRect (3, 12, 6, 13, kSewerFloor);
    fillRect (4, 15, 5, 16, kSewerFloor);
    fillRect (3, 12, 6, 14, kSewerFloor);
    fillRect (4, 15, 5, 16, kSewerFloor);
    fillRect (9, 1, 12, 3, kSewerFloor);
    fillRect (10, 4, 11, 12, kSewerFloor);
    fillRect (16, 2, 20, 2, kSewerFloor);
    fillRect (10, 13, 34, 13, kSewerFloor);
    fillRect (16, 3, 17, 12, kSewerFloor);
    fillRect (21, 1, 24, 3, kSewerFloor);
    fillRect (18, 8, 20, 8, kSewerFloor);
    fillRect (21, 7, 24, 9, kSewerFloor);
    fillRect (28, 1, 31, 3, kSewerFloor);
    fillRect (32, 2, 34, 2, kSewerFloor);
    fillRect (28, 7, 31, 9, kSewerFloor);
    fillRect (32, 8, 34, 8, kSewerFloor);
    fillRect (35, 2, 36, 13, kSewerFloor);
    fillRect (37, 4, 40, 4, kSewerFloor);
    fillRect (37, 11, 47, 11, kSewerFloor);
    SETSQ (40, 4, kSewerFloor);
    SETSQ (47, 4, kSewerFloor);
    SETSQ (50, 12, kSewerFloor);
    SETSQ (56, 12, kSewerFloor);
    fillRect (41, 1, 46, 7, kSewerFloor);
    fillRect (48, 1, 62, 11, kSewerFloor);
    fillRect (49, 2, 63, 6, /* kVoid*/ kSewage);
    fillRect (48, 13, 52, 15, kSewerFloor);
    fillRect (54, 13, 58, 15, kSewerFloor);
    fillRect (60, 7, 62, 16, kSewerFloor);
    fillRect (0, 17, 62, 18, kSewerFloor);

    fillRect (59, 10, 59, 11, kSewerWall1);

    fillRect (0, 17, 62, 18, kSewage);
    fillRect (60, 6, 62, 16, kSewage);
    fillRect (21, 1, 24, 3, kSewage);
    fillRect (28, 1, 31, 3, kSewage);
    fillRect (28, 7, 31, 9, kSewage);
    fillRect (3, 12, 6, 14, kSewage);
    fillRect (4, 15, 5, 16, kSewage);

    addDoor (20, 2, 0, 0, 0, 0);
    addDoor (32, 2, 0, 0, 0, 0);
    addDoor (20, 8, 0, 0, 0, 0);
    addDoor (32, 8, 0, 0, 0, 0);
    addDoor (40, 4, 0, 0, 1, 0, 0, 1);
    addDoor (47, 4, 0, 0, 1, 0, 0, 1);
    addDoor (47, 11, 0, 0, 0, 0);
    addDoor (50, 12, 1, 0, 1, 0);
    addDoor (56, 12, 1, 0, 0, 0);

    flagRect (0, 17, 63, 19, shSquare::kDarkNW , 1);
    flagRect (0, 17, 63, 19, shSquare::kDarkNE , 1);
    flagRect (0, 16, 59, 19, shSquare::kDarkSW , 1);
    flagRect (0, 16, 59, 19, shSquare::kDarkSE , 1);

    flagRect (2, 11, 7, 17, shSquare::kDark, 1);
    flagRect (59, 12, 59, 19, shSquare::kDarkNE, 1);
    flagRect (59, 12, 59, 19, shSquare::kDarkSE, 1);
    flagRect (60, 12, 63, 19, shSquare::kDark, 1);

    ++mNumRooms;
    for (int x = 40; x <= 47; ++x)
        for (int y = 0; y <= 8; ++y)
            SETROOM (x, y, mNumRooms);

    /* Fill garbage compactor with random objects. */
    for (int i = 0; i < 13; ++i)
        putObject (generateObject (), RNG (41, 46), RNG (1, 7));

    {   /* One room has radiation suit and the other has ordinary jumpsuit. */
        shObject *ordsuit = new shObject (kObjOrdinaryJumpsuit);
        ordsuit->setBuggy ();
        shObject *radsuit = new shObject (kObjRadiationSuit);
        radsuit->setDebugged ();
        if (RNG (2)) {
            putObject (radsuit, 50, 14);
            putObject (ordsuit, 56, 14);
        } else {
            putObject (ordsuit, 50, 14);
            putObject (radsuit, 56, 14);
        }
    }

    {
        shObject *plant = new shObject (kObjFusionPlant);
        plant->setDebugged ();
        putObject (plant, 3 + RNG (4), 12 + RNG (3));
    }

    fillRect (41, 1, 46, 7, kSewage);
    makeGarbageCompactor (40, 0, 47, 8);
    flagRect (0, 0, 63, 19, shSquare::kNoLanding, 1);
    flagRect (41, 1, 46, 7, shSquare::kNoLanding, 0);
}



void
shMapLevel::flagRect (int sx, int sy, int ex, int ey,
                      shSquare::shSquareFlags flag, int value)
{
    for (int x = sx; x <= ex; ++x)
        for (int y = sy; y <= ey; ++y)
            if (value) {
                mSquares[x][y].mFlags |= flag;
            } else {
                mSquares[x][y].mFlags &= ~flag;
            }
}



void
shHero::enterCompactor ()
{
    if (Level->mTimeOuts.mCompactor < Clock - FULLTURN) {
        Level->mTimeOuts.mCompactor = Clock + FULLTURN * 3;
        Level->magDoors (1);
    }
}


void
shHero::leaveCompactor ()
{
}

/*************\
| Arena Level |
\*************/
// was used for testing flamethrower kCone area attack
// now awaits for kSpread to be implemented and tested

void
shMapLevel::buildArena ()
{
    mMapType = kTest;

    layRoom (0, 0, 63, 19);

    /* Randomly litter one half of arena with single walls. */
    for (int i = 0; i < 40; ++i) {
        int x = RNG (1, 31);
        int y = RNG (1, 18);
        // Select random kind of wall.
        mSquares[x][y].mTerr = (shTerrainType) RNG (kCavernWall1, kVirtualWall2);
    }
}
