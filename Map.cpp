#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Util.h"
#include "Map.h"
#include "Monster.h"
#include "Object.h"
#include "Hero.h"
#include "MapBuilder.h"
#include "Distribution.h"
#include "Effect.h"
#if 0
char gASCIITiles[256] = {
    ' ', '|', '-', '+', '+', '+', '+', '+', '+', '+', '|', '|', 'x', '.'
};
#else
char gASCIITiles[256] = {
    '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', '#', 'x', '.'
};
#endif

#define SETSQ(_x, _y, _w) mSquares[_x][_y].mTerr = _w

const char *
stringDirection (shDirection d)
{
    static const char *txt[12] = {
        "N", "NE", "E", "SE", "S", "SW", "W", "NW", "U", "D", "O", "0"
    };

    return txt[d];
}


/* returns move-cost distance in feet between two squares */
int
rlDistance (int x1, int y1, int x2, int y2)
{
    int horiz = x2 > x1 ? x2 - x1 : x1 - x2;
    int vert = y2 > y1 ? y2 - y1 : y1 - y2;

    if (vert > horiz) {
        return (int) (5 * (sqrt ((double) 2.0) * horiz + vert - horiz));
    } else {
        return (int) (5 * (sqrt ((double) 2.0) * vert + horiz - vert));
    }
}



//RETURNS: distance in feet between 2 squares
int
distance (int x1, int y1, int x2, int y2)
{
    return (int) (sqrt ((double) ((x1 - x2) * (x1 - x2) * 25 +
                                  (y1 - y2) * (y1 - y2) * 25)));
}


//RETURNS: distance in feet between 2 entities
int
distance (shCreature *e1, shCreature *e2)
{
    return distance (e1->mX, e1->mY, e2->mX, e2->mY);
}


int
distance (shCreature *e1, int x1, int y1)
{
    return distance (e1->mX, e1->mY, x1, y1);
}


/* MODIFIES: *x, *y, and *z to reflect movement of one square in direction d
   RETURNS:  true if the new square is inbounds, false o/w
*/
int
shMapLevel::moveForward (shDirection d, int *x, int *y,
                         int *z /* = NULL */)
{
    switch (d) {
    case kNorth:
        *y -= 1; break;
    case kEast:
        *x += 1; break;
    case kSouth:
        *y += 1; break;
    case kWest:
        *x -= 1; break;
    case kNorthEast:
        --*y; ++*x; break;
    case kSouthEast:
        ++*y; ++*x; break;
    case kSouthWest:
        ++*y; --*x; break;
    case kNorthWest:
        --*y; --*x; break;
    case kUp:
        if (z) ++*z; break;
    case kDown:
        if (z) --*z; break;
    case kNoDirection:
    case kOrigin:
        break;
    default:
        abort ();
    }
    return (*x >= 0 and *x < mColumns and *y >= 0 and *y < mRows);
}


shDirection
vectorDirection (int x1, int y1)
{
    return vectorDirection (0, 0, x1, y1);
}


shDirection
vectorDirection (int x1, int y1, int x2, int y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;

    if (0 == dx) {
        return dy < 0 ? kNorth :
               dy > 0 ? kSouth : kNoDirection;
    }
    double slope = dy / dx;

    if (slope > 2) {
        return dy < 0 ? kNorth : kSouth;
    } else if (slope > 0.5) {
        return dy < 0 ? kNorthWest : kSouthEast;
    } else if (slope > -0.5) {
        return dx < 0 ? kWest : kEast;
    } else if (slope > -2) {
        return dy < 0 ? kNorthEast : kSouthWest;
    } else {
        return dy < 0 ? kNorth : kSouth;
    }
}

/* returns: the direction from the first square to the second, if the squares
            are lined up in one of the 8 primary directions; o/w kNoDirection
 */
shDirection
linedUpDirection (int x1, int y1, int x2, int y2)
{
    if (x1 == x2) {
        return (y1 < y2) ? kSouth : kNorth;
    } else if (y1 == y2) {
        return (x1 < x2) ? kEast : kWest;
    } else if (x1 - y1 == x2 - y2) {
        return (x1 < x2) ? kSouthEast : kNorthWest;
    } else if (x1 + y1 == x2 + y2) {
        return (x1 < x2) ? kNorthEast : kSouthWest;
    } else {
        return kNoDirection;
    }
}


shDirection
linedUpDirection (shCreature *e1, shCreature *e2)
{
    return linedUpDirection (e1->mX, e1->mY, e2->mX, e2->mY);
}



int
areAdjacent (int x1, int y1, int x2, int y2)
{
    int dx = x2 - x1;
    int dy = y2 - y1;

    return ! (dx > 1 or dx < -1 or dy > 1 or dy < -1);
}

//RETURNS: distance in feet between 2 creatures
int
areAdjacent (shCreature *c1, shCreature *c2)
{
    return areAdjacent (c1->mX, c1->mY, c2->mX, c2->mY);
}


shFeature *
shMapLevel::getFeature (int x, int y)
{
    for (int i = 0; i < mFeatures.count (); ++i) {
        shFeature *f = mFeatures.get (i);
        if (f->mX == x and f->mY == y) {
            return f;
        }
    }
    return NULL;
}

shFeature *
shMapLevel::getKnownFeature (int x, int y)
{
    shFeature *f = getFeature (x, y);
    if (f) { /* Secret doors are drawn as walls.  No checks needed. */
        if (f->mTrapUnknown and /* Hide unknown traps. */
            /* Exception: berserk doors are drawn as plain doors. */
            shFeature::kDoorOpen != f->mType and
            shFeature::kDoorClosed != f->mType)
        {
            f = NULL;
        }
    }
    return f;
}


const char *
shFeature::getDoorLockName ()
{
    shObjectIlk *ilk = keyNeededForDoor ();
    if (!ilk) return "juicy";
    char *buf = GetBuf ();
    /* Do not use return 'master' unless it has been indentified already. */
    if (ilk->mId == kObjMasterKeycard and (ilk->mFlags & kIdentified)) {
        strcpy (buf, ilk->mReal.mName);
    } else {
        strcpy (buf, ilk->mAppearance.mName);
    }
    char *overwrite = strrchr (buf, ' '); /* Cut off 'keycard'. */
    overwrite[0] = 0;
    return buf;
}


const char *
shFeature::getShortDescription ()
{
    switch (mType) {
    case kStairsUp: return "stairs up";
    case kStairsDown: return "stairs down";
    case kComputerTerminal: return "computer terminal";
    case kRadTrap: return "radiation trap";
    case kACMESign: return "loose ACME sign";
    case kPit:
        return mTrapUnknown ? "pit trap" :
            (mTrapMonUnknown == 2 ? "taped pit" : "pit");
    case kAcidPit:
        return mTrapUnknown ? "acid pit trap" :
            (mTrapMonUnknown == 2 ? "taped acid pit" : "acid pit");
    case kSewagePit: return "deep spot";
    case kTrapDoor: return "trap door";
    case kFloorGrating: return "floor grating";
    case kBrokenGrating: return "broken floor grating";
    case kHole: return "hole";
    case kMachinery: return "piece of machinery";
    case kDoorOpen: return "door";
    case kDoorClosed: return "door";
    case kVat: return "sludge vat";
    case kWeb: return "web";
    case kPortableHole: return "portable hole";
    case kDoorHiddenHoriz:
    case kDoorHiddenVert:
    case kMovingHWall:
        return "wall";
    case kPortal:
        return "unimplemented feature";
    case kMaxFeatureType:
        return "something buggy";
    }
    return "description missing";
}


const char *
shFeature::getDescription ()
{
    switch (mType) {
    case kComputerTerminal:
    case kRadTrap:
    case kPit:
    case kAcidPit:
    case kTrapDoor:
    case kHole:
    case kDoorHiddenHoriz:
    case kDoorHiddenVert:
    case kMovingHWall:
    case kFloorGrating:
    case kBrokenGrating:
    case kWeb:
    case kPortal:
    case kMaxFeatureType:
        return getShortDescription ();
    /* TODO: Stair description should vary depending on location.
             In Space Base they are manholes with ladders. */
    case kStairsUp: return "staircase leading upwards";
    case kStairsDown: return "staircase leading downwards";
    case kSewagePit: return "dangerously deep pool of sewage";
    case kPortableHole: return "closed portable hole";
    case kMachinery: return "pistons and machinery";
    case kACMESign: return "loose sign of A Company that Makes Everything";
    case kDoorOpen:
    case kDoorClosed:
    {
        char *buf = GetBuf ();
        const char *an = "a ";
        const char *berserk = "";
        const char *autom = "";
        const char *lock = "";
        if (isBerserkDoor () and !mTrapUnknown)
            berserk = "malfunctioning ";
        if (isAutomaticDoor ())  autom = "automatic ";
        if (isAutomaticDoor () and !isBerserkDoor ()) an = "an ";
        if (isRetinaDoor ()) {
            lock = " with retina scanner";
        } else if (isLockDoor ()) {
            char *buf = GetBuf ();
            sprintf (buf, " with %s%s code lock",
                     isLockBrokenDoor () ? "broken " : "", getDoorLockName ());
            lock = buf;
        }
// Length test:
// You see here malfunctioning automatic door with broken orange code lock.
        snprintf (buf, SHBUFLEN, "%s%s%sdoor%s",
            an, berserk, autom, lock);
        return buf;
    }
    case kVat:
        if (!mVat.mAnalyzed) {
            return "a sludge vat";
        } else {
            char *buf = GetBuf ();
            const char *descriptor;
            switch (mVat.mHealthy) {
            case -3: descriptor = "very toxic"; break;
            case -2: descriptor = "toxic"; break;
            case -1: descriptor = "somewhat toxic"; break;
            case  0: descriptor = "unhealthy"; break;
            case +1: descriptor = "somewhat healthy"; break;
            case +2: descriptor = "healthy"; break;
            case +3: descriptor = "very healthy"; break;
            default:
                if (mVat.mHealthy < -3)
                    descriptor = "vile";
                else if (mVat.mHealthy > +3)
                    descriptor = "fabulous";
                else
                    descriptor = "unknown";
            }
            snprintf (buf, SHBUFLEN, "a vat of %s sludge", descriptor);
            return buf;
        }
    }
    return getShortDescription ();
}

void
shFeature::splash (void)
{   /* Ok, this is not acid blood but damage can be the same. */
    assert (mType == kAcidPit);
    Level->attackEffect (kAttAcidBlood1, NULL, mX, mY, kNoDirection, NULL);
}

int
shMapLevel::isRadioactive (int x, int y)
{
    int n = 0;
    shFeature *f = getFeature (x, y);
    if (f and shFeature::kVat == f->mType and f->mVat.mRadioactive)
        n += 10;

    for (int i = 0; i < mFeatures.count (); ++i) {
        f = mFeatures.get (i);
        if (shFeature::kRadTrap == f->mType and
            areAdjacent (f->mX, f->mY, x, y))
        {
            ++n;
        }
    }

    for (int tx = maxi (0, x-1); tx <= mini (MAPMAXCOLUMNS-1, x+1); ++tx) {
        for (int ty = maxi (0, y-1); ty <= mini (MAPMAXROWS-1, y+1); ++ty) {
            shCreature *c = getCreature (tx, ty);
            /* TODO: Introduce radioactive monster flag. */
            if (c and (c->isA (kMonVatSlime) or c->isA (kMonRadspider) or
                       c->isA (kMonRadbug) or c->isA (kMonRadscorpion) or
                       c->isA (kMonGhoul)))
                ++n;
        }
    }

    shObjectVector *v = getObjects (x, y);
    if (v)
        for (int i = 0; i < v->count (); ++i) {
            shObject *obj = v->get (i);
            if (obj->isRadioactive ())
                n += obj->mCount;
        }

    if (mSquares[x][y].mFlags & shSquare::kRadioactive)
        n += 10;

    return n;
}

void
shMapLevel::reveal (int partial)
{
    for (int y = 0; y < MAPMAXROWS; ++y) {
        for (int x = 0; x < MAPMAXCOLUMNS; ++x) {
            if (!partial or RNG (3)) {
                if (!isFloor (x, y) or isLit (x, y, x, y) or Hero.cr ()->intr (kBlind)) {
                    remember (x, y, getSquare (x, y)->mTerr);
                }
                shFeature *kf = getKnownFeature (x, y);
                shFeature *f = getFeature (x, y);
                if (kf) {
                    remember (x, y, kf);
                } else if (f and f->isHiddenDoor ()) {
                    remember (x, y, f);
                }
            }
            I->drawMem (x, y);
        }
    }
}


void
shMapLevel::debugDraw ()
{
    int x, y;
    printf ("\n\n\n");
    for (y = 0; y < mRows; y++) {
        for (x = 0; x < mColumns; x++) {
            putc (gASCIITiles[mSquares[x][y].mTerr], stdout);
        }
        putc ('\n', stdout);
    }
}


bool
shMapLevel::isLit (int x, int y, int vx, int vy)
{
    if (mSquares[x][y].mFlags & shSquare::kLit)
        return true;
    int flag = mSquares[x][y].mFlags & shSquare::kDark;
    if (!flag)  //fully lit
        return true;
    if (shSquare::kDark == flag)  //fully dark
        return false;

    bool res = false;
    if (vy<=y) { //N
        if (vx <= x)
            res = !(flag & shSquare::kDarkNW);
        else
            res = !(flag & shSquare::kDarkNE);
    } else {    // S
        if (vx <= x)
            res = !(flag & shSquare::kDarkSW);
        else
            res = !(flag & shSquare::kDarkSE);
    }
    if (res)
        return res;

    /* A door square is tricky.  A partially lit square becomes fully lit
       when its door is opened.  KLUDGE: If the Hero remembers a door is
       here, then we'll say it's fully lit so that she can see when the
       door is closed.
    */

    shFeature *f = getKnownFeature (x, y);
    if (f and f->isDoor ()) {
        if (f->isOpenDoor () or
            mRemembered[y][x].mFeat == shFeature::kDoorOpen)
        {
            return true;
        }
    }
    return false;
}

/*  nw,ne,sw,se are directions from which the square is seen.
    -1: dark when seen from that side
     1: light
     0: leave as is
*/

void
shMapLevel::setLit (int x, int y, int nw, int ne, int sw, int se)
{
    if (nw > 0) {
        mSquares[x][y].mFlags &= ~shSquare::kDarkNW;
    } else if (nw < 0) {
        mSquares[x][y].mFlags |= shSquare::kDarkNW;
    }
    if (ne > 0) {
        mSquares[x][y].mFlags &= ~shSquare::kDarkNE;
    } else if (ne < 0) {
        mSquares[x][y].mFlags |= shSquare::kDarkNE;
    }
    if (sw > 0) {
        mSquares[x][y].mFlags &= ~shSquare::kDarkSW;
    } else if (sw < 0) {
        mSquares[x][y].mFlags |= shSquare::kDarkSW;
    }
    if (se > 0) {
        mSquares[x][y].mFlags &= ~shSquare::kDarkSE;
    } else if (se < 0) {
        mSquares[x][y].mFlags |= shSquare::kDarkSE;
    }

    /* Smell: this is wrong place to put it.  Either drawLevel should
       recognize darkened areas in FOV and forget them or computeVisibility
       should do it. */
    if (ne + nw + se + sw <= -4) {
        if (Hero.cr () and !Hero.cr ()->intr (kBlind)) {
            forgetTerrain (x, y);
        }
    }
}


void
shMapLevel::layCorridor (int x1, int y1, int x2, int y2)
{

    if (y1 > y2) {
        int swap = y1; y1 = y2; y2 = swap;
    }
    if (x1 > x2) {
        int swap = x1; x1 = x2; x2 = swap;
    }
    if (x1 == x2) {
        for (int i = y1; i <= y2; ++i) {
            SETSQ (x1, i, kFloor);
            SETSQ (x1 - 1, i, kVWall); // west wall
            SETSQ (x1 + 1, i, kVWall); // east wall
        }
    } else if (y1 == y2) {
        for (int i = x1; i <= x2; ++i) {
            SETSQ (i, y1, kFloor);
            SETSQ (i, y1 - 1, kHWall); // north wall
            SETSQ (i, y1 + 1, kHWall); // south wall
        }
    } else {
        abort ();
    }
}

void
shMapLevel::layRoom (int x1, int y1, int x2, int y2)
{
    SETSQROOM (x1, y1, kNWCorner, mNumRooms);
    SETSQROOM (x1, y2, kSWCorner, mNumRooms);
    SETSQROOM (x2, y1, kNECorner, mNumRooms);
    SETSQROOM (x2, y2, kSECorner, mNumRooms);
    for (int i = x1 + 1; i < x2; ++i) {
        SETSQROOM (i, y1, kHWall, mNumRooms);
        SETSQROOM (i, y2, kHWall, mNumRooms);
    }
    for (int i = y1 + 1; i < y2; ++i) {
        SETSQROOM (x1, i, kVWall, mNumRooms);
        SETSQROOM (x2, i, kVWall, mNumRooms);
    }

    for (int i = x1 + 1; i < x2; ++i) {
        for (int j = y1 + 1; j < y2; ++j) {
            SETSQROOM (i, j, kFloor, mNumRooms);
        }
    }

    mRooms[mNumRooms].mType = shRoom::kNormal;
    mNumRooms++;
}


void
shMapLevel::setRoomId (int x1, int y1, int x2, int y2, int id)
{
    for (int y = y1; y <= y2; ++y)
        for (int x = x1; x <= x2; ++x)
            SETROOM (x, y, id);
}



void
shMapLevel::buildDenseLevel ()
{
    int x, y, l;

    x = RNG (40);
    y = RNG (8) + 6;
    l = 15 + RNG (20);

    printf ("%d %d %d\n", x, y, l);

    layCorridor (x, y, x + l, y);
    layRoom (x + RNG (5), RNG (5), x + l - RNG (5), y - 1);
    SETSQ (x + l/2, y-1, kFloor);
}


shMapLevel *
shMapLevel::buildBranch (MapType type,
                         int depth,
                         int dlevel,
                         shMapLevel **end,
                         int ascending /* = 0 */ )
{
    shMapLevel *first, *prev, *L;
    first = prev = L = NULL;

    for (int i = 0; i < depth; ++i) {
        L = new shMapLevel (dlevel, type);
        Maze.add (L);
        if (i > 0) {
            if (ascending)
                L->addDownStairs (-1, -1, prev, -1, -1);
            else
                prev->addDownStairs (-1, -1, L, -1, -1);
        } else {
            first = L;
        }
        prev = L;
        dlevel++;
    }
    if (end)
        *end = L;
    return first;
}


void
shMapLevel::buildMaze ()
{
    const int TOWNLEVEL = 10;
    const int BUNKERLEVELS = 16;
    const int CAVELEVELS = 6;
    const int SEWERLEVELS = 6;
    const int CAVEBRANCH = 15;
    const int MAINFRAMELEVELS = 4;

    int x, y;
    shMapLevel *town, *cavemouth, *rabbit, *plant, *arena, *head, *tail;

    shObject *fakes[5] = {
        new shObject (kObjFakeOrgasmatron1),
        new shObject (kObjFakeOrgasmatron2),
        new shObject (kObjFakeOrgasmatron3),
        new shObject (kObjFakeOrgasmatron4),
        new shObject (kObjFakeOrgasmatron5)};
    shuffle (fakes, 5, sizeof (shObject *));

    Maze.add (NULL);  /* dummy zeroth level */

    /* main branch and town */
    buildBranch (kBunkerRooms, TOWNLEVEL - 1, 1, &tail);
    town = new shMapLevel (TOWNLEVEL, kTown);
    Maze.add (town);
    tail->addDownStairs (-1, -1, town, 7, 3);
    head = buildBranch (kBunkerRooms, BUNKERLEVELS - TOWNLEVEL, TOWNLEVEL + 1,
                        &tail);
    town->addDownStairs (6, 17, head, -1, -1);
    /* rabbit level */
    rabbit = new shMapLevel (BUNKERLEVELS + 1, kRabbit);
    Maze.add (rabbit);
    tail->addDownStairs (-1, -1, rabbit, 20, 10);
    /* radiation caves */
    cavemouth = Maze.get (CAVEBRANCH);
    head = buildBranch (kRadiationCave, CAVELEVELS, CAVEBRANCH, &tail);
    cavemouth->addDownStairs (-1, -1, head, -1, -1);
    shCreature *bofh = shCreature::monster (kMonBOFH);
    do {
        tail->findSuitableStairSquare (&x, &y);
    } while (tail->isOccupied (x, y));
    tail->putCreature (bofh, x, y);

    /* mainframe */
    head = buildBranch (kMainframe, MAINFRAMELEVELS, BUNKERLEVELS + 1, &tail,
                        1 /* ascending branch */);
    head->findSuitableStairSquare (&x, &y);
    head->putObject (fakes[0], x, y);

    do {
        tail->findSuitableStairSquare (&x, &y);
    } while (tail->isOccupied (x, y));
    tail->spawnTeam (x, y, &prob::ShodanGroup);
    tail->findSuitableStairSquare (&x, &y);
    tail->putObject (fakes[1], x, y);
    tail->findSuitableStairSquare (&x, &y);
    tail->putObject (fakes[2], x, y);
    tail->findSuitableStairSquare (&x, &y);
    tail->putObject (fakes[3], x, y);
    /* sewer */
    head = buildBranch (kSewer, SEWERLEVELS, TOWNLEVEL + 1, &tail);
    town->addDownStairs (53, 14, head, -1, -1);
    /* waste processing plant */
    plant = new shMapLevel (TOWNLEVEL + SEWERLEVELS + 1, kSewerPlant);
    plant->putObject (fakes[4], RNG (41, 46), RNG (1, 7));
    Maze.add (plant);
    tail->addDownStairs (-1, -1, plant, 3, 2);
    /* test level */
    arena = new shMapLevel (0, kTest);
    Maze.add (arena);
}


void
shMapLevel::reset ()
{
    memset (&mSquares, 0, sizeof (mSquares));
    memset (&mObjects, 0, sizeof (mObjects));
    memset (&mCreatures, 0, sizeof (mCreatures));
    memset (&mVisibility, 0, sizeof (mVisibility));
    memset (&mEffects, 0, sizeof (mEffects));
    mEffectList.reset ();
    for (int y = 0; y < MAPMAXROWS; ++y)
        for (int x = 0; x < MAPMAXCOLUMNS; ++x)
        {
            forgetCreature (x, y);
            forgetObject   (x, y);
            forgetFeature  (x, y);
            forgetTerrain  (x, y);
        }
    mRooms[0].mType = shRoom::kNotRoom;

    mRows = MAPMAXROWS;
    mColumns = MAPMAXCOLUMNS;
    mRows = 20;
    mColumns = 64;
    mFlags = 0;
    mNumRooms = 1;
}

//constructor

shMapLevel::shMapLevel (int level, MapType type)
    : mCrList (), mFeatures (), mExits ()
{
    mType = type;
retry:
    debug.log ("building level %d", level);
    reset ();
    mDLevel = level;

    switch (type) {
    case kTown:
        snprintf (mName, 12, "Robot Town");
        buildTown ();
        return;
    case kRadiationCave:
        snprintf (mName, 12, "Gamma Caves");
        buildCave ();
        return;
    case kMainframe:
        snprintf (mName, 12, "Mainframe");
        buildMainframe ();
        return;
    case kRabbit:
        snprintf (mName, 12, "Rabbit Hole");
        buildRabbitLevel ();
        return;
    case kSewer:
        snprintf (mName, 12, "Sewer");
        buildSewer ();
        return;
    case kSewerPlant:
        snprintf (mName, 12, "Sewer Plant");
        buildSewerPlant ();
        return;
    case kTest:
        snprintf (mName, 12, "Test Range");
        buildArena ();
        return;
    case kBunkerRooms:
    default:
        snprintf (mName, 12, "Space Base");
        if (0 == buildBunkerRooms ()) {
            goto retry;
        }
        return;
    }
}

shCreature *
shMapLevel::getShopKeeper (int x, int y)
{
    if (!isInShop (x, y)) return NULL;

    for (int i = 0; i < mCrList.count (); ++i) {
        shCreature *c = mCrList.get (i);
        if (c->isA (kMonClerkbot) and
            c->mShopKeeper.mShopId == mSquares[x][y].mRoomId)
        {
            return c;
        }
    }
    return NULL;
}


shCreature *
shMapLevel::getGuard ()
{
    for (int i = 0; i < mCrList.count (); ++i) {
        shCreature *c = mCrList.get (i);
        if (c->isA (kMonGuardbot) and
            (Hero.cr ()->canSee (c) or areAdjacent (c, Hero.cr ())) and
            !c->isPet ())
        {
            return c;
        }
    }
    return NULL;
}


shCreature *
shMapLevel::getDoctor (int x, int y)
{
    if (!isInHospital (x, y))  return NULL;

    for (int i = 0; i < mCrList.count (); ++i) {
        shCreature *c = mCrList.get (i);
        if (c->isA (kMonDocbot) and
            c->mDoctor.mRoomID == mSquares[x][y].mRoomId)
        {
            return c;
        }
    }
    return NULL;
}


shCreature *
shMapLevel::getMelnorme ()
{
    for (int i = 0; i < mCrList.count (); ++i) {
        shCreature *c = mCrList.get (i);
        if (c->isA (kMonMelnorme)) {
            return c; /* Assumption: only one Melnorme per level. */
        }
    }
    return NULL;
}


shObject *
shMapLevel::findObject (int x, int y, shObjId ilk)
{
    shObjectVector *v = getObjects (x, y);

    if (!v)  return NULL;

    for (int i = 0; i < v->count (); ++i)
        if (v->get (i) -> isA (ilk))
            return v->get (i);

    return NULL;
}

/* Might delete obj! */
shObject *
shMapLevel::putObject (shObject *obj, int x, int y)
{
    assert (obj);
    /* If there is a feature an interaction may start. */
    shFeature *f = getFeature (x, y);
    if (f) {
        /* Heaps of junk fill pits. */
        if (f->isPit () and obj->isA (kObjJunk)) {
            bool acidpit = f->mType == shFeature::kAcidPit;
            bool seen = Hero.cr ()->canSee (x, y);
            bool unknown = f->mTrapUnknown;
            if (seen) {
                if (!unknown) {
                    I->p ("%s is plugged.", AN (f));
                } else {
                    I->p ("%s disappears!", THE (obj));
                }
                if (acidpit) {
                    I->nonlOnce ();
                    I->p ("  Acid splashes around!");
                }
            }
            if (acidpit)  f->splash ();
            removeFeature (f);
            delete obj;
            /* Pit is gone.  Untrap any creature that was stuck there. */
            shCreature *c = getCreature (x, y);
            if (c and c->mTrapped.mInPit) {
                c->mTrapped.mInPit = 0;
                c->mZ = 0;
            }
            return NULL;
        }
        obj = f->dropObject (NULL, obj);
        if (!obj)  return NULL;
    }
    /* Object lasted through.  Add to existing pile or make new stack. */
    if (!mObjects[x][y])
        mObjects[x][y] = new shObjectVector ();
    /* Try merging with any existing object in stack. */
    for (int i = 0; i < mObjects[x][y]->count (); ++i) {
        shObject *fobj = mObjects[x][y]->get (i);
        if (fobj->canMerge (obj)) {
            fobj->merge (obj);
            return fobj;
        }
    }
    /* Save location data. */
    obj->mLocation = shObject::kFloor;
    obj->mX = x;
    obj->mY = y;
    obj->mOwner = NULL;
    mObjects[x][y]->add (obj);
    return obj;
}

/* Returns number of monsters transported. */
int
shMapLevel::attractWarpMonsters (int x, int y)
{
    shCreature *c;
    int n = 0;

    for (int i = 0; i < mCrList.count (); i++) {
        c = mCrList.get (i);
        if (c->feat (kWarpish) and RNG (3)) {
            int xx = x;
            int yy = y;
            if (!findNearbyUnoccupiedSquare (&xx, &yy)) {
                c->transport (xx, yy);
                ++n;
            }
        }
    }
    return n;
}


int
shMapLevel::warpCreature (shCreature *c, shMapLevel *newlevel)
{
    int res, x, y;
    int oldx = c->mX;
    int oldy = c->mY;

    res = -1;
    do {
        newlevel->findUnoccupiedSquare (&x, &y);
        if (newlevel->getFeature (x, y)) {
            /* just to be safe */
            continue;
        }
        if (c->isHero ()) {
            c->oldLocation (x, y);
            Level = newlevel;
        }
        removeCreature (c);
        res = newlevel->putCreature (c, x, y);
        if (1 == res) {
            return 1;
            break;
        }
    } while (-1 == res);
    if (c->isHero ()) {
        Hero.checkForFollowers (this, oldx, oldy);
    }
    return 0;
}


/* returns non-zero if the creature at x,y dies */
int
shMapLevel::checkTraps (int x, int y, int savedcmod)
{
    if (!isOccupied (x, y)) return 0;
    shCreature *c = getCreature (x, y);
    shFeature *f = getFeature (x, y);

    if (!f)  return 0;

    if (!f->mTrapUnknown)  savedcmod -= 5;

    switch (f->mType) {
    case shFeature::kDoorOpen:
        if (shFeature::kBerserk & f->mDoor.mFlags and
            0 == countObjects (x, y) and
            RNG (3))
        { /* berserk door! */
            I->drawScreen ();
            if (c->isHero ()) {
                f->mTrapUnknown = 0;
                c->interrupt ();
                if (c->reflexSave (20 + savedcmod)) {
                    I->p ("The door slams shut!  You jump out of the way!");
                    return 0;
                }
                I->p ("The door slams shut on you!");
            } else if (Hero.cr ()->canSee (x, y)) {
                I->p ("The door slams shut on %s!", THE (c));
                f->mTrapUnknown = 0;
                f->mTrapMonUnknown = 0;
            } else {
                I->p ("You hear a crunching sound.");
                f->mTrapMonUnknown = 0;
            }
            if (c->sufferDamage (kAttDoorSlam)) {
                if (!c->isHero() and Hero.cr ()->canSee (c)) {
                    c->pDeathMessage (THE (c), kSlain);
                }
                c->die (kSlain, "a slamming door");
                return 1;
            }
        }
        break;
    case shFeature::kPit:
    {
        if (c->isHero ()) {
            c->interrupt ();
            if (c->intr (kFlying)) {
                if (!f->mTrapUnknown) {
                    I->p ("You fly over a pit.");
                }
                return 0;
            }
            f->mTrapUnknown = 0;
            f->mTrapMonUnknown = 0;
            if (c->reflexSave (20 + savedcmod)) {
                I->p ("You escape a pit trap.");
                return 0;
            }
            I->p ("You fall into a pit!");
        } else {
            if (c->intr (kFlying)) {
                return 0;
            } else if (c->isA (kMonAstromechDroid)) {
                f->mTrapMonUnknown = 0;
                if (Hero.cr ()->canSee (x, y)) {
                    I->p ("%s engages thrusters to avoid falling into %s pit.",
                        THE (c), f->mTrapUnknown ? "a" : "the");
                    f->mTrapUnknown = 0;
                }
                return 0;
            }
            if (Hero.cr ()->canSee (x, y)) {
                I->p ("%s falls into a pit!", THE (c));
                if (f->mTrapUnknown) {
                    I->drawScreen ();
                }
                f->mTrapUnknown = 0;
                f->mTrapMonUnknown = 0;
            }
        }
        c->mTrapped.mInPit = NDX (2, 6);
        c->mZ = -1;
        if (c->sufferDamage (kAttPitTrapFall)) {
            if (!c->isHero () and Hero.cr ()->canSee (c)) {
                c->pDeathMessage (THE (c), kSlain);
                c->die (kSlain, "pitfall");
            } else {
                c->die (kMisc, "Fell into a pit");
            }
            return 1;
        }
        break;
    }
    case shFeature::kAcidPit:
    {
        if (c->isHero ()) {
            c->interrupt ();
            if (c->intr (kFlying)) {
                if (!f->mTrapUnknown) {
                    I->p ("You fly over an acid pit.");
                }
                return 0;
            }
            f->mTrapUnknown = 0;
            if (c->reflexSave (20 + savedcmod)) {
                I->p ("You escape an acid pit trap.");
                return 0;
            }
            I->p ("You fall into a pit!");
            I->p ("You land in a pool of acid!");
            f->mTrapMonUnknown = 0;
        } else {
            if (c->intr (kFlying)) {
                return 0;
            } else if (c->isA (kMonAstromechDroid)) {
                f->mTrapMonUnknown = 0;
                if (Hero.cr ()->canSee (x, y)) {
                    I->p ("%s engages thrusters to avoid falling into %s pit.",
                        THE (c), f->mTrapUnknown ? "a" : "the");
                    f->mTrapUnknown = 0;
                }
                return 0;
            }
            if (Hero.cr ()->canSee (x, y)) {
                I->p ("%s falls into a pool of acid!", THE (c));
                if (f->mTrapUnknown) {
                    I->drawScreen ();
                }
                f->mTrapUnknown = 0;
                f->mTrapMonUnknown = 0;
            }
        }
        c->mTrapped.mInPit = NDX (2, 6);
        c->mZ = -1;
        if (c->sufferDamage (kAttAcidPitBath)) {
            if (!c->isHero () and Hero.cr ()->canSee (c)) {
                c->pDeathMessage (THE (c), kSlain);
                c->die (kMisc, "Dissolved in acid");
            } else {
                c->die (kSlain, "acid");
            }
            return 1;
        } else {
            c->setTimeOut (TRAPPED, FULLTURN, 0);
        }
        break;
    }
    case shFeature::kSewagePit:
    {
        if (c->isHero ()) {
            if (c->intr (kFlying)) {
                return 0;
            }
            c->interrupt ();
            if (f->mTrapUnknown) {
                f->mTrapUnknown = 0;
                I->p ("You stumble into a deep spot!");
            } else if (c->reflexSave (20 + savedcmod)) {
                I->p ("You avoid a deep spot.");
                return 0;
            }
            f->mTrapMonUnknown = 0;
            if (c->intr (kCanSwim)) {
                I->p ("You swim in the sewage.");
                break;
            } else {
                I->p ("You sink under the surface!");
                if (!c->intr (kAirSupply) and !c->intr (kBreathless)) {
                    I->p ("You're holding your breath!");
                    c->mTrapped.mDrowning = c->mAbil.Con ();
                }
            }
        } else {
            if (c->intr (kFlying) or c->intr (kCanSwim)) {
                return 0;
            }
            if (Hero.cr ()->canSee (x, y)) {
                if (!c->intr (kAirSupply) and !c->intr (kBreathless)) {
                    I->p ("%s splashes frantically in the sewage.", THE (c));
                    c->mTrapped.mDrowning = c->mAbil.Con () / 2;
                } else {
                    I->p ("%s splashes in the sewage", THE (c));
                }
                if (f->mTrapUnknown) {
                    I->drawScreen ();
                }
                f->mTrapUnknown = 0;
                f->mTrapMonUnknown = 0;
            }
        }
        c->mTrapped.mInPit = NDX (3, 5);
        c->mZ = -1;
        c->setTimeOut (TRAPPED, FULLTURN, 0);
        break;
    }
    case shFeature::kTrapDoor:
    {
        if (c->isHero ()) {
            if (c->intr (kFlying)) {
                if (!f->mTrapUnknown) {
                    I->p ("You fly over a trap door.");
                }
                return 0;
            }
            f->mTrapUnknown = 0;
            f->mTrapMonUnknown = 0;
            if (c->reflexSave (20 + savedcmod)) {
                c->interrupt ();
                I->p ("You escape a trap door.");
                return 0;
            }
            I->p ("A trap door opens underneath you!");
            I->drawScreen ();
        } else {
            if (c->intr (kFlying)) {
                return 0;
            }
            if (Hero.cr ()->canSee (x, y)) {
                Hero.interrupt ();
                I->p ("A trap door opens underneath %s!", THE (c));
                f->mTrapUnknown = 0;
                f->mTrapMonUnknown = 0;
            }
        }
        {
            int newx;
            int newy;
            shMapLevel *dest = this;
            for (dest = getLevelBelow ();
                 dest->getLevelBelow () and !dest->noDig () and !RNG (3);
                 dest = dest->getLevelBelow ())
            ;
            dest->findUnoccupiedSquare (&newx, &newy);
            if (c->isHero ()) {
                c->oldLocation (newx, newy);
                I->pauseXY (c->mX, c->mY);
                Level = dest;
            }
            removeCreature (c);
            dest->putCreature (c, newx, newy);
            if (c->isHero ()) {
                Hero.checkForFollowers (this, x, y);
            }
            return 0;
        }
        break;
    }
    case shFeature::kHole:
    case shFeature::kBrokenGrating:
    {
        if (c->isHero ()) {
            if (c->intr (kFlying)) {
                if (!f->mTrapUnknown) {
                    I->p ("You fly over a %s.", f->getShortDescription ());
                }
                return 0;
            }
            f->mTrapUnknown = 0;
            if (c->reflexSave (15 + savedcmod)) {
                Hero.interrupt ();
                I->p ("You avoid a %s.", f->getShortDescription ());
                return 0;
            }
            I->p ("You fall into a %s!", f->getShortDescription ());
            f->mTrapMonUnknown = 0;
        } else {
            if (c->intr (kFlying)) {
                return 0;
            }
            f->mTrapMonUnknown = 0;
            if (Hero.cr ()->canSee (x, y)) {
                Hero.interrupt ();
                I->p ("%s falls through a %s!", THE (c), f->getShortDescription ());
                f->mTrapUnknown = 0;
            }
        }
        {
            int newx;
            int newy;
            shMapLevel *dest = this;
            for (dest = getLevelBelow ();
                 dest->getLevelBelow () and !RNG (3);
                 dest = dest->getLevelBelow ())
            ;
            dest->findLandingSquare (&newx, &newy);
            if (c->isHero ()) {
                c->oldLocation (newx, newy);
                I->pauseXY (c->mX, c->mY);
                Level = dest;
            }
            removeCreature (c);
            dest->putCreature (c, newx, newy);
            if (c->isHero ()) {
                Hero.checkForFollowers (this, x, y);
            }
            return 0;
        }
        break;
    }
    case shFeature::kRadTrap:
    {
        if (c->isHero ()) {
            Hero.interrupt ();
            if (!c->intr (kBlind)) {
                I->p ("You are bathed in a green glow!");
                f->mTrapUnknown = 0;
            } else { /* Just a suspicion.  Hero must search for the trap. */
                I->p ("You hear strange mechanical click above you.");
            }
        } else {
            if (Hero.cr ()->canSee (x, y)) {
                Hero.interrupt ();
                I->p ("%s is bathed in a green glow!", THE (c));
                f->mTrapUnknown = 0;
            }
        }
        f->mTrapMonUnknown = 0;
        if (c->sufferDamage (kAttRadTrap)) {
            c->die (kMisc, "Radiation trap.");
            return 1;
        }
        break;
    }
    case shFeature::kACMESign:
    {
        if (RNG (2)) {
            if (Hero.cr ()->canSee (x, y)) {
                I->p ("Suddenly an ACME sign falls from the ceiling!");
            } else if (c->isHero ()) {
                I->p ("With a shrill something falls from the ceiling!");
            }
            if (RNG (2)) {
                if (Hero.cr ()->canSee (c) or c->isHero ())
                    I->p ("%s dodge%s!", THE (c), c->isHero () ? "" : "s");
            } else if (c->sufferDamage (kAttACMESign)) {
                c->die (kKilled, NULL, NULL, kAttACMESign, "falling ACME sign");
                return 1;
            }
            removeFeature (f); /* Single use trap. */
        } else {
            /* Fail to activate. */
            if (!c->isHero ())
                f->mTrapMonUnknown = 0;
        }
        break;
    }
    case shFeature::kWeb:
    {
        if (c->isHero ()) {
            I->p ("You walk into a web!");
            f->mTrapUnknown = 0;
        } else {
            if (Hero.cr ()->canSee (x, y))
                f->mTrapUnknown = 0;
            f->mTrapMonUnknown = 0;
        }
        c->mTrapped.mWebbed = NDX (2, 6);
        break;
    }
    default:
        break;
    }
    return 0;
}

/*RETURNS: 0 on success
           1 the creature died
           -1 failed otherwise
*/
/* Fast move does not trigger doors or traps.  It is caller's
   responsibility to update environment. */
int
shMapLevel::moveCreature (shCreature *c, int x, int y, bool fast)
{
    if (!isFloor (x, y) or mCreatures[x][y] or isObstacle (x, y)) {
        return -1;
    }

    /* TODO: accomplish this by memmove. */
    for (int i = TRACKLEN - 1; i > 0; --i) {
        c->mTrack[i] = c->mTrack[i-1];
    }
    c->mTrack[0].mX = c->mX;
    c->mTrack[0].mY = c->mY;

    mCreatures[c->mX][c->mY] = NULL;
    mCreatures[x][y] = c;
    if (!fast)
        checkDoors (c->mX, c->mY);
    if (c->isHero ())
        c->oldLocation (x, y);
    c->mLastX = c->mX;
    c->mLastY = c->mY;
    c->mLastLevel = this;
    c->mX = x;
    c->mY = y;
    /* May have moved out of a pit. */
    if (c->mZ < 0)  c->mZ = 0;
    c->mLastMoveTime = Clock;
    if (!fast and checkTraps (c->mX, c->mY))
        return 1;
    if (!fast)
        checkDoors (x, y);
    if (c->isHero ()) {
        c->newLocation ();
        if (!fast)  Hero.lookAtFloor ();
    }
    return 0;
}

/* RETURNS: 0 on success,
            1 if the creature dies
            -1 if failed otherwise
*/
int
shMapLevel::putCreature (shCreature *c, int x, int y)
{
    if (!isFloor (x, y) or mCreatures[x][y] or isObstacle (x, y)) {
        return -1;
    }

    if (c->mX != -10 and c->mY != -10) {
        /* Change last time only if this is transport action.
           Creatures that are put down for the first time should not
           have their LastMoveTime touched. */
        c->mLastMoveTime = Clock;
    }
    c->mLevel = this;
    c->mX = x;
    c->mY = y;
    c->mZ = c->intr (kFlying);
    mCreatures[x][y] = c;
    mCrList.add (c);
    checkDoors (x, y);
    if (c->isHero ()) {
        c->newLocation ();
        Hero.lookAtFloor ();
        if (!(kHeroHasVisited & mFlags)) {
            mFlags |= kHeroHasVisited;
            if (mDLevel != 1) {
                Hero.earnScore (500);
            }
            if (isTownLevel () and !Hero.getStoryFlag ("entered town")) {
                Hero.setStoryFlag ("entered town", 1);
                Hero.earnScore (500);
            }
            if (isMainframe () and !Hero.getStoryFlag ("entered mainframe")) {
                Hero.setStoryFlag ("entered mainframe", 1);
				I->p ("There's a cyber psycho bitch");
				I->p ("who's been hoarding the power");
				I->p ("of the Bizarro Orgasmatron!");
                Hero.earnScore (2000);
            }
            spawnMonsters ();
            if (c->intr (kAutoSearching))  c->doSearch ();
        }
    }
    return 0;
}


shCreature *
shMapLevel::removeCreature (shCreature *c)
{
    int x = c->mX;
    int y = c->mY;

    if (-1 == mCrList.remove (c)) {
        /* sometimes called when the creature is not actually on the level */
        return NULL;
    }
    /* Closing automatic door may cause remembered creature glyph to remain
       there despite hero saw it die/disappear. */
    if (c->feat (kSessile) and Hero.cr ()->canSee (c)) {
        forgetCreature (c->mX, c->mY);
    }
    mCreatures[x][y] = NULL;
    checkDoors (x, y);
    //if (&Hero == c) {
    //    Hero.oldLocation (x, y);
    //}
    c->mLastX = x;
    c->mLastY = y;
    c->mLastLevel = this;
    return c;
}

static prob::Table *
pickAMonsterIlk (int low, int high)
{
    /* Highest monster tiers may be empty. */
    while (prob::MonByLevel[high] == NULL)
        --high;

    /* Count weights. */
    int weight_sum = 0;
    for (int i = low; i <= high; ++i) {
        prob::Table *table = prob::MonByLevel[i];
        while (table->chance != 0) {
            weight_sum += table->chance;
            ++table;
        }
    }

    /* Randomly pick something. */
    int pick = RNG (weight_sum);
    int i = low;
    while (pick >= 0) {
        prob::Table *table = prob::MonByLevel[i];
        while (pick >= 0 and table->chance != 0) {
            pick -= table->chance;
            ++table;
        }
        if (pick < 0)
            return --table;
        ++i;
    }

    assert (false); /* Should never arrive here. */
    return NULL;
}

int
shMapLevel::spawnGroup (int bx, int by, int num, shMonId mon)
{
    /* Cap group size on low floors. */
    if (Hero.cr ())
        num = mini (num, mDLevel + Hero.cr ()->mCLevel + RNG (2));

    int spawned = 0;
    while (num--) {
        int x = bx, y = by;
        if (findNearbyUnoccupiedSquare (&x, &y)) {
            return spawned;
        }
        shCreature *monster = shCreature::monster (mon);
        if (!putCreature (monster, x, y))
            ++spawned;
    }
    return spawned;
}

int
shMapLevel::spawnTeam (const int x, const int y, const prob::Table *table)
{
    /* Weighted choice from sublist. */
    if (table->type == 's') {
        int count = RNG (table->at_least, table->at_most);
        int ret = 0;

        for (int i = 0; i < count; ++i) {
            prob::Table *ptr = table->sub;
            int weight_sum = 0;
            while (ptr->chance != 0) {
                weight_sum += ptr->chance;
                ++ptr;
            }

            ptr = table->sub;
            int choice = RNG (weight_sum);
            while (choice >= 0) {
                choice -= ptr->chance;
                ++ptr;
            }
            ret += spawnTeam (x, y, --ptr);
        }
        return ret;

    /* Create specific monster (group). */
    } else if (table->type == 'm') {
        int count = RNG (table->at_least, table->at_most);
        return spawnGroup (x, y, count, table->monster);

    /* Create monster team.  Execute each entry from sublist. */
    } else if (table->type == 'M') {
        int spawned = 0;
        prob::Table *team = table->sub;
        /* Chance is not treated like weight -- it is a percentage
           chance of executing this row. */
        while (team->chance != 0) {
            if (RNG (100) < (int)team->chance)
                spawned += spawnTeam (x, y, team);

            ++team;
        }
        return spawned;
    }

    assert (false);
    return 1;
}

bool
shMapLevel::findSpawnSquare (int &x, int &y)
{
    x = y = -1;
    do {
        if (0 != findUnoccupiedSquare (&x, &y)) {
            return false;
        }
        /* Make sure not to spawn monsters too close to hero. */
    } while (!Hero.cr () or
             Hero.cr ()->mLevel != this or
             distance (x, y, Hero.cr ()->mX, Hero.cr ()->mY) < 8 * 5);

    return true;
}

/* Returns number of monsters spawned. */
int
shMapLevel::spawnNTeams (int teams, const prob::Table *table)
{
    int spawned = 0, x, y;

    while (teams--) {
        findSpawnSquare (x, y);
        spawned += spawnTeam (x, y, table);
    }

    return spawned;
}

/* Returns number of monsters spawned. */
int
shMapLevel::spawnMonsters ()
{

    switch (mMapType) {
    case kRabbit: case kTest:
        /* Never create any monsters there. */
        return 0;
    case kTown:
        return spawnNTeams (RNG (12, 15), &prob::RobotTown);
    case kRadiationCave:
        return spawnNTeams (RNG (10, 15), &prob::GammaCaves);
    case kSewer:
        return spawnNTeams (RNG (8, 12), &prob::Sewers);
    case kMainframe:
        return spawnNTeams (RNG (11, 13), &prob::Mainframe);
    case kSewerPlant:
    case kBunkerRooms:
    {
        int high = (mDLevel + Hero.cr ()->mCLevel - 1) / 2;
        int low = high / 2;
        int population = 10 + mDLevel / 4;

        int x, y, spawned = 0, tries = 0;
        while (spawned < population and tries < 100) {
            findSpawnSquare (x, y);
            prob::Table *table = pickAMonsterIlk (low, high);
            spawned += spawnTeam (x, y, table);
            ++tries;
        }

        return spawned;
    }
    }

    assert (false);
    return 1;
}


void
shMapLevel::removeFeature (shFeature *f)
{
    if (f->isDoor ())
        doorRemoved (f);
    mFeatures.remove (f);
    delete f;
}


void
shMapLevel::clearSpecialEffects ()
{
    mEffectList.reset ();

    for (int x = 0; x < MAPMAXCOLUMNS; ++x)
        for (int y = 0; y < MAPMAXROWS; ++y)
            mEffects[x][y] = eff::none;
}

/* Returns index of the last drawn effect. */
int
shMapLevel::setSpecialEffect (int x, int y, eff::type e)
{
    if (e < eff::last_effect)
        mEffects[x][y] = e;

    if (e == eff::none)
        return -1;

    Effect_on_map ef;
    ef.e = e;
    ef.x = x;
    ef.y = y;
    mEffectList.add (ef);
    return mEffectList.count () - 1;
}

void
shMapLevel::clearSpecialEffect (int index)
{
    if (index == -1)
        return;
    assert (index >= 0 and index < mEffectList.count ());

    Effect_on_map ef = mEffectList.get (index);
    mEffects[ef.x][ef.y] = eff::none;
    ef.e = eff::none;
    mEffectList.set (index, ef);
    drawSq (ef.x, ef.y);
}

int
shMapLevel::effectsInEffect ()
{
    return mEffectList.count ();
}

bool
shMapLevel::is_open_sq (const int x, const int y)
{
    return isFloor (x, y) and !isObstacle (x, y);
}

bool
shMapLevel::is_free_sq (const int x, const int y)
{
    return is_open_sq (x, y) and !isOccupied (x, y);
}

bool
shMapLevel::is_free_safe_sq (const int x, const int y)
{
    if (is_free_sq (x, y) and !isOccupied (x, y)) {
        shFeature *f = getFeature (x, y);
        if (!f or !f->isTrap ())
            return true;
    }
    return false;
}

bool
shMapLevel::is_ok_stair_sq (const int x, const int y)
{
    if (isInRoom (x, y) and
        !isInShop (x, y) and
        !getFeature (x, y) and
        !getCreature (x, y))
    {
        switch (mMapType) {
        case kBunkerRooms:
        {
            shRoom *room = getRoom (x, y);
            /* At most one exit (i.e. stair) in a room. */
            for (int i = 0; i < mExits.count (); ++i) {
                shFeature *f = mExits.get (i);
                if (room == getRoom (f->mX, f->mY))
                    return false;
            }
            break;
        }
        case kMainframe:
        case kSewer:
            if (!stairsOK (x, y))
                return false;
            break;
        default:
            ;
        }
        return true;
    }
    return false;
}

void
shMapLevel::findSuitableStairSquare (int *x, int *y)
{
    while (true) {
        *x = RNG (mColumns);
        *y = RNG (mRows);

        if (is_ok_stair_sq (*x, *y))
            return;
    }
}


/* connect this level to one below it with a staircase*/

void
shMapLevel::addDownStairs (int x, int y,
                           shMapLevel *destlev, int destx, int desty)
{
    shFeature *stairsup = new shFeature ();
    shFeature *stairsdown = new shFeature ();
    shFeature *f;

    stairsup->mType = shFeature::kStairsUp;
    stairsdown->mType = shFeature::kStairsDown;

    if (-1 == x) {
        findSuitableStairSquare (&x, &y);
    }
    if (-1 == destx) {
        destlev->findSuitableStairSquare (&destx, &desty);
    }

    stairsdown->mX = x;
    stairsdown->mY = y;
    stairsdown->mDest.mX = destx;
    stairsdown->mDest.mY = desty;
    stairsdown->mDest.mLevel = Maze.find (destlev);
    f = getFeature (x, y);
    if (f) {
        removeFeature (f);
    }
    addFeature (stairsdown);
    mExits.add (stairsdown);

    stairsup->mX = destx;
    stairsup->mY = desty;
    stairsup->mDest.mX = x;
    stairsup->mDest.mY = y;
    stairsup->mDest.mLevel = Maze.find (this);
    f = destlev->getFeature (destx, desty);
    if (f) {
        destlev->removeFeature (f);
    }
    destlev->addFeature (stairsup);
    destlev->mExits.add (stairsup);
}


int
shMapLevel::findSquare (int *x, int *y)
{
    *x = RNG (mColumns);
    *y = RNG (mRows);
    return 0;
}


int
shMapLevel::findLandingSquare (int *x, int *y)
{
    do {
        findUnoccupiedSquare (x, y);
    } while (!landingOK (*x, *y));
    return 0;
}


/* Searches for free square. */
int
shMapLevel::findOpenSquare (int *x, int *y)
{
    int attempts = 100;

    do {
        *x = RNG (mColumns);
        *y = RNG (mRows);
        if (isFloor (*x, *y) and !isOccupied (*x, *y) and !isObstacle (*x, *y))
            return 0;  /* Found! */
    } while (attempts--);
    return -1;
}


/* Searches for free square without obstacle or trap. */
int
shMapLevel::findUnoccupiedSquare (int *x, int *y)
{
    int attempts = 100;

    do {
        *x = RNG (mColumns);
        *y = RNG (mRows);
        if (isFloor (*x, *y) and
            !isOccupied (*x, *y) and !isObstacle (*x, *y))
        {
            shFeature *f = getFeature (*x, *y);
            if (!f or !f->isTrap ())
                return 0; /* Found no feature or harmless feature. */
        }
    } while (attempts--);
    return -1;
}


int
shMapLevel::findAdjacentUnoccupiedSquare (int *x, int *y)
{
    shDirection dirlist[8] =
        { kNorth, kSouth, kEast, kWest,
          kNorthWest, kSouthWest, kNorthEast, kSouthEast };
    int x1, y1;
    int i;

    shuffle (&dirlist[0], 8, sizeof (shDirection));

    for (i = 0; i < 8; i++) {
        x1 = *x; y1 = *y;
        if (moveForward (dirlist[i], &x1, &y1)) {
            if ((isFloor (x1, y1)) and
                (0 == isOccupied (x1, y1)) and
                (0 == isObstacle (x1, y1)))
            {
                *x = x1;
                *y = y1;
                return 0;
            }
        }
    }
    return -1;
}


int
shMapLevel::countAdjacentCreatures (int ox, int oy)
{
    int cnt = 0;

    for (int x = ox - 1; x <= ox + 1; x++)
        for (int y = oy - 1; y <= oy + 1; y++) {
            if (x == ox and y == oy)
                continue;
            if (isInBounds (x, y) and getCreature (x, y))
                ++cnt;
        }

    return cnt;
}

bool
shMapLevel::find_close_free_sq (int &x, int &y)
{
    if (0 == findAdjacentUnoccupiedSquare (&x, &y))
        return true;

    int attempts = 25;

    const int r = 4;
    int sx = maxi (0, x - r), ex = mini (mColumns - 1, x + r);
    int sy = maxi (0, y - r), ey = mini (mRows    - 1, y + r);

    do {
        x = RNG (sx, ex);
        y = RNG (sy, ey);

        if (isFloor (x, y) and
            !isOccupied (x, y) and
            !isObstacle (x, y))
        {
            return true;
        }
    } while (attempts--);
    return false;
}

int
shMapLevel::findNearbyUnoccupiedSquare (int *x, int *y)
{
    int attempts = 100;

    if (0 == findAdjacentUnoccupiedSquare (x, y)) {
        return 0;
    }
    do {
        *x += RNG (3) - 1;
        *y += RNG (3) - 1;

        if (*x < 0) *x = 1;
        if (*y < 0) *y = 1;
        if (*x >= mColumns) *x = mColumns - 2;
        if (*y >= mRows) *y = mRows - 2;

        if (isFloor (*x, *y) and
            !isOccupied (*x, *y) and
            !isObstacle (*x, *y))
        {
            return 0;
        }
    } while (attempts--);
    return -1;
}


void
shMapLevel::dig (int x, int y)
{
    if (!isInBounds (x, y) or noDig ())
        return;
    shTerrainType floor, vwall, hwall;
    shTerrainType nwcorner, necorner, swcorner, secorner;
    switch (mMapType) {
        case kBunkerRooms: case kTown: case kRabbit: case kTest:
            floor = kFloor;
            vwall = kVWall;
            hwall = kHWall;
            nwcorner = kNWCorner;
            necorner = kNECorner;
            swcorner = kSWCorner;
            secorner = kSECorner;
            break;
        case kRadiationCave:
            floor = kCavernFloor;
            vwall = hwall = kCavernWall1;
            nwcorner = necorner = swcorner = secorner = kCavernWall1;
            break;
        case kSewer: case kSewerPlant:
            floor = kSewerFloor;
            vwall = hwall = kSewerWall1;
            nwcorner = necorner = swcorner = secorner = kSewerWall1;
            break;
        case kMainframe:
            floor = kVirtualFloor;
            vwall = hwall = kVirtualWall1;
            nwcorner = necorner = swcorner = secorner = kVirtualWall1;
            break;
        default: /* Make it obvious something is not right. */
            floor = (shTerrainType) RNG (kFloor, kVirtualFloor);
            vwall = (shTerrainType) RNG (kHWall, kVirtualWall2);
            hwall = (shTerrainType) RNG (kHWall, kVirtualWall2);
            nwcorner = (shTerrainType) RNG (kHWall, kVirtualWall2);
            necorner = (shTerrainType) RNG (kHWall, kVirtualWall2);
            swcorner = (shTerrainType) RNG (kHWall, kVirtualWall2);
            secorner = (shTerrainType) RNG (kHWall, kVirtualWall2);
            break;
    }
    mSquares[x][y].mTerr = floor;
    mSquares[x][y].mFlags |= shSquare::kHallway;
    /* Orthogonal directions. */
    if (isInBounds (x-1, y) and mSquares[x-1][y].mTerr == kStone)
        mSquares[x-1][y].mTerr = vwall;
    if (isInBounds (x+1, y) and mSquares[x+1][y].mTerr == kStone)
        mSquares[x+1][y].mTerr = vwall;
    if (isInBounds (x, y-1) and mSquares[x][y-1].mTerr == kStone)
        mSquares[x][y-1].mTerr = hwall;
    if (isInBounds (x, y+1) and mSquares[x][y+1].mTerr == kStone)
        mSquares[x][y+1].mTerr = hwall;
    /* Corners. */
    if (isInBounds (x-1, y-1) and mSquares[x-1][y-1].mTerr == kStone)
        mSquares[x-1][y-1].mTerr = nwcorner;
    if (isInBounds (x+1, y-1) and mSquares[x+1][y-1].mTerr == kStone)
        mSquares[x+1][y-1].mTerr = necorner;
    if (isInBounds (x-1, y+1) and mSquares[x-1][y+1].mTerr == kStone)
        mSquares[x-1][y+1].mTerr = swcorner;
    if (isInBounds (x+1, y+1) and mSquares[x+1][y+1].mTerr == kStone)
        mSquares[x+1][y+1].mTerr = secorner;
}


shMapLevel *
shMapLevel::getLevelBelow ()
{
    int i;
    shVector <shFeature *> v;
    shFeature *f;

    for (i = 0; i < mExits.count (); i++) {
        f = mExits.get (i);
        if (shFeature::kStairsDown == f->mType) {
            v.add (f);
        }
    }
    if (v.count ()) {
        f = v.get (RNG (v.count ()));
        return Maze.get (f->mDest.mLevel);
    } else {
        return NULL;
    }
}

const char *
shSquare::the ()
{
    switch (mTerr) {
    case kStone:
    case kCavernWall1: case kCavernWall2:
        return "the cavern wall";
    case kVWall:     case kHWall:     case kNWCorner:
    case kNECorner:  case kSWCorner:  case kSECorner:
    case kNTee:      case kSTee:      case kWTee:
    case kETee:
    case kSewerWall1: case kSewerWall2:
    case kVirtualWall1:  case kVirtualWall2:
        return "the wall";
    case kGlassPanel:
        return "reinforced glass panel";
    case kBrokenLightAbove: /* Intentional. */
    case kFloor:
    case kSewerFloor:
    case kVirtualFloor:
        return "the floor";
    case kCavernFloor:
        return "the cavern floor";
    case kSewage:
        return "the sewage";
    default:
        return "the unknown terrain feature";
    }
}

const char *
shSquare::an ()
{
    char *buf = GetBuf ();
    const char *tmp = the ();
    tmp += 4; /* Skip "the " part. */
    snprintf (buf, SHBUFLEN, "%s %s", isvowel (tmp[0]) ? "an" : "a", tmp);
    return buf;
}
