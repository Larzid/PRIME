#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Util.h"
#include "Map.h"
#include "Monster.h"
#include "Object.h"

#include "MapBuilder.h"

#define ptox(_p) (_p % mColumns)
#define ptoy(_p) (_p / mColumns)
#define xytop(_x, _y) (_y * mColumns + _x)


// returns: number of squares excavated
int
shMapLevel::buildCaveRoom (int x1, int y1, int size)
{
    int x, y, i;
    shDirection dirs[4] = { kNorth, kSouth, kEast, kWest };
    shDirection dir;
    int n = 0;
    shVector <int> points;
    int radioactive = !RNG (6);

    points.add (xytop (x1, y1));

    for (n = 0; n < size;) {
        int p;
        if (0 == points.count ()) {
            return n;
        }

        p = points.removeByIndex (RNG (points.count ()));
        x = ptox (p);
        y = ptoy (p);

        if (radioactive) {
            mSquares[x][y].mFlags |= shSquare::kRadioactive;
        }

        if (!isFloor (x, y)) {
            SETSQ (x, y, kCavernFloor);
            ++n;
        } else if (!RNG (3)) {
            continue;
        }

        for (i = 0; i < 4; i++) {
            dir = dirs[i];
            int x2 = x;
            int y2 = y;

            int r = RNG (4);

            if (x2 <= r or x2 >= mColumns - r or
                y2 <= r/2 or y2 >= mRows - r/2 or
                abs (x2 - x1) > 7 or
                abs (y2 - y1) > 4)
            {
                continue;
            }

            if (moveForward (dir, &x2, &y2) and
                RNG (20) >= points.count ())
            {
                points.add (xytop (x2, y2));
            }
        }
    }
    return n;
}

void
shMapLevel::buildCaveTunnel (int x1, int y1, int x2, int y2)
{
    shDirection dirs[4] = { kNorth, kSouth, kEast, kWest };
    shDirection dir;
    int x = x1;
    int y = y1;

    while ((x != x2) or (y != y2)) {
        dir = RNG (3) ? vectorDirection (x, y, x2, y2) : dirs[RNG(4)];

        if (moveForward (dir, &x, &y)) {
            SETSQ (x, y, kCavernFloor);
            x1 = x;
            y1 = y;
            if (isInBounds (x+1, y)) {
                SETSQ (x+1, y, kCavernFloor);
            }
        } else {
            x = x1;
            y = y1;
        }
    }
}



// RETURNS: 1 on success, 0 on failure

int
shMapLevel::buildCave ()
{
    int i;
    int j;
    int x, y;
    int lighting = RNG (3) ? -1 : 1;
    int radioactive = RNG (mDLevel) > 5;

    mMapType = kRadiationCave;

    struct { int mX; int mY; } nodes[5][3];

    for (i = 0; i < 5; i++) {
        for (j = 0; j < 3; j++) {
            x = RNG (12) + 12 * i;
            y = 2 + RNG (5) + 5 * j;

            nodes[i][j].mX = x;
            nodes[i][j].mY = y;

            buildCaveRoom (x, y, RNG (20, 35));

            if (i > 0) {
                buildCaveTunnel (nodes[i-1][j].mX, nodes[i-1][j].mY, x, y);
            }
            if (j > 0 and (RNG (3) or i == 2 )) {
                buildCaveTunnel (nodes[i][j-1].mX, nodes[i][j-1].mY, x, y);
            }

        }
    }


    for (y = 0; y < mRows; y++) {
        for (x = 0; x < mColumns; x++) {
            if (radioactive) {
                mSquares[x][y].mFlags |= shSquare::kRadioactive;
            }

            if (!isFloor (x, y)) { /* Set squares neighboring floor to walls. */
        /* W  */if ((x > 0 and isFloor (x-1, y)) or
        /* E  */    (x < mColumns - 1 and isFloor (x+1, y)) or
        /* N  */    (y > 0 and isFloor (x, y-1)) or
        /* S  */    (y < mRows - 1 and isFloor (x, y+1)) or
        /* NW */    (x > 0 and y > 0 and isFloor (x-1, y-1)) or
        /* SW */    (x > 0 and y < mRows - 1 and isFloor (x-1, y+1)) or
        /* NE */    (x < mColumns - 1 and y > 0 and isFloor (x+1, y-1)) or
        /* SE */    (x < mColumns - 1 and y < mRows - 1 and isFloor (x+1, y+1)))
                {
                    if (RNG (5) < 2)
                        SETSQ (x, y, kCavernWall2);
                    else
                        SETSQ (x, y, kCavernWall1);
                }
                setLit (x, y, lighting, lighting, lighting, lighting);
            } else {
                setLit (x, y, lighting, lighting, lighting, lighting);
                mSquares[x][y].mRoomId = 0;
                if (x == 0 or x == mColumns - 1 or
                    y == 0 or y == mRows - 1)
                {
                    if (RNG (5) < 2)
                        SETSQ (x, y, kCavernWall2);
                    else
                        SETSQ (x, y, kCavernWall1);
                }
            }
        }
    }

    mRooms[0].mType = shRoom::kCavern;


    for (i = NDX (2, 4); i; --i) {
        findUnoccupiedSquare (&x, &y);
        switch (RNG (4)) {
        case 0:
            addTrap (x, y, shFeature::kPit); break;
        case 1:
            addTrap (x, y, shFeature::kAcidPit); break;
        case 2:
            if (isBottomLevel ()) {
                addTrap (x, y, shFeature::kAcidPit);
            } else {
                addTrap (x, y, shFeature::kTrapDoor);
            }
            break;
        case 3:
            addTrap (x, y, shFeature::kRadTrap); break;
        }
    }

    for (i = 0; i < 8; i++) {
        findUnoccupiedSquare (&x, &y);
        putObject (generateObject (), x, y);
    }
    if (!RNG (4)) {
        findUnoccupiedSquare (&x, &y);
        putObject (gen_obj_type (prob::RayGun), x, y);
    }

    return 1;
}







#if 0
/* here's some fractal noise crap that I was playing around with but
   decided not to use.
 */


static char lattice[257];

static double
noise (double p)
{
    int ip = ((int) (double) floor (p)) & 255;

    return lattice[ip] + (lattice[ip+1] - lattice[ip]) * (p - (double) ip);
}

static void
initnoise ()
{
    static int ready = 0;
    int i;

    if (ready) return;
    ready = 1;

    for (i = 0; i < 257; i++) {
        lattice[i] = RNG (256) - 128;
    }
}

static int
fractalnoise (int x, int y)
{
    double fx = x + y;
    double fy = y;
    double result =
        (noise (fx * 0.022) + noise (fy * 0.022)) / 10.0 +
        (noise (fx * 0.253) + noise (fy * 0.253));

    I->debug ("%f", result);
    return (int) result;
}

#endif
