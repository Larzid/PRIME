#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Util.h"
#include "Map.h"
#include "Monster.h"
#include "Object.h"

#include "MapBuilder.h"

int
shMapLevel::isClear (int x1, int y1, int x2, int y2)
{
    int x, y;

    for (x = x1; x <= x2; x++) {
        if (x < 0 or x >= mColumns) {
            continue;
        }
        for (y = y1; y <= y2; y++) {
            if (y < 0 or y >= mRows) {
                continue;
            }
            if (kFloor == GETSQ (x, y)) {
                return 0;
            }
        }
    }
    return 1;
}

//RETURNS 1 if there are n squares of uncarved rock in direction d, 0 o/w
int
shMapLevel::enoughClearance (int x, int y, shDirection d, int m, int n)
{
    int x1, x2;
    int y1, y2;

    switch (d) {
    case kNorth:
        if (y < 5) return 0;
        x1 = x - n;
        x2 = x + n;
        y1 = y - m;
        y2 = y - 1;
        break;
    case kSouth:
        if (y > mRows - 5) return 0;
        x1 = x - n;
        x2 = x + n;
        y1 = y + 1;
        y2 = y + m;
        break;
    case kEast:
        if (x > mColumns - 6) return 0;
        x1 = x + 1;
        x2 = x + m;
        y1 = y - n;
        y2 = y + n;
        break;
    case kWest:
        if (x < 6) return 0;
        x1 = x - m;
        x2 = x - 1;
        y1 = y - n;
        y2 = y + n;
        break;
    default:
        abort ();
    }
    return isClear (x1, y1, x2, y2);
}



//RETURNS: 1 if we successfully sealed the corridor
int
shMapLevel::buildTwistyCorridor (int x, int y, shDirection d)
{
    int nx = x;
    int ny = y;
    int i;
    int cnt = 0;
    int subcnt;

    if (0 == enoughClearance (x, y, d, 5, 5)) {
        return 0;
    }

    nx = x;
    ny = y;
    for (i = 0; 1; i++) {
        if (0 == moveForward (d, &nx, &ny)) {
            return cnt;
        }
//      if ((0 == i)
        if (kFloor == GETSQ (nx, ny)) {
            return cnt;
        }

        SETSQ (nx, ny, kFloor);
        SETSQFLAG (nx, ny, kHallway);
/*
        if (isHorizontal (d)) {
            if (ny > 0) SETSQ (nx, ny - 1, kHWall);
            if (ny + 1 < mRows) SETSQ (nx, ny + 1, kHWall);
        }
        else {
            if (nx > 0) SETSQ (nx - 1, ny, kVWall);
            if (nx + 1 < mColumns) SETSQ (nx + 1, ny, kVWall);
        }
*/
//      draw ();
//      I->getChar ();
        ++cnt;
        if (0 != RNG (2)) {
            continue;
        }
        if (isHorizontal (d) and RNG (2)) {
            continue;
        }
        switch (RNG (i >= 5 ? 10 : 7)) {
        case 9:
            if ((subcnt = buildTwistyCorridor (nx, ny, leftQuarterTurn (d)))) {
                return cnt + subcnt;
            }
            continue;
        case 8:
            cnt += buildTwistyCorridor (nx, ny, rightQuarterTurn (d));
            /* fall through */
        case 7:
            if ((subcnt = buildTwistyCorridor (nx, ny, leftQuarterTurn (d)))) {
                return cnt + subcnt;
            }
            if (0 == enoughClearance (nx, ny, d, 1, 5)) {
                return cnt;
            }
        case 6:
            cnt += buildTwistyCorridor (nx, ny, leftQuarterTurn (d));
            if (0 == enoughClearance (nx, ny, d, 1, 5)) {
                return cnt;
            }
            break;
        case 5:
            cnt += buildTwistyCorridor (nx, ny, rightQuarterTurn (d));
            /* fall through */
        case 4:
            cnt += buildTwistyCorridor (nx, ny, leftQuarterTurn (d));
            if (0 == enoughClearance (nx, ny, d, 1, 5)) {
                return cnt;
            }
            break;
        default:
            continue;
        }
    }
}

int
shMapLevel::fiveByFiveClearance (int x, int y)
{
    int maxy = mRows;
    int i, j;

    if ((y + 3 >= mRows) or (x + 4 >= mColumns)) {
        return 0;
    }

    for (i = x; i < x + 5; i++) {
        for (j = y; j < mRows; j++) {
            if (kFloor == GETSQ (i, j)) {
                if (j < y + 4) {
                    return 0;
                }
                else if (j < maxy) {
                    maxy = j;
                }
            }
        }
    }
    return maxy;
}



//RETURNS: # of doors
int
shMapLevel::buildRoomOrElRoom (int sx, int sy, int ey)
{
    int ex;
    int x;
    int y;
    int ndoors = 0;
    int doortries = 0;
    int ra = 0 == RNG (20);

    for (ex = sx + 2; ex < mColumns; ex++) {
        if ((ex > sx + 4) and (0 == RNG(7))) {
            /* check for nearby hallway */
            if (isClear (ex, sy, mColumns, ey)) {
                do {
                    --sx; --ex;
                } while (isClear (sx, sy, sx, ey));
                ++sx;
                break;
            }

            while (1) {
                if (RNG (2)) {
                    if (isClear (sx, ey, ex, ey)) {
                        ey++; sy++;
                        if (ey >= mRows) {
                            return -1;
                        }
                    }
                    else {
                        break;
                    }
                }
                else if (isClear (ex, sy, ex, ey)) {
                    ex++; sx++;
                }
                else {
                    break;
                }
            }
        }
        if (!isClear (ex, sy, ex, ey - 1)) {
            break;
        }
    }

    for (x = sx + 1; x < ex -1; x++) {
        for (y = sy + 1; y < ey -1; y++) {
            SETSQ (x, y, kFloor);
            if (ra) {
                SETSQFLAG (x, y, kRadioactive);
            }
        }
    }

    /* add walls */
    for (x = sx + 1; x < ex - 1; x++) {
        SETSQ (x, sy, kHWall);
        SETSQ (x, ey - 1, kHWall);
    }
    for (y = sy + 1; y < ey - 1; y++) {
        SETSQ (sx, y, kVWall);
        SETSQ (ex - 1, y, kVWall);
    }
    SETSQ (sx, sy, kNWCorner);
    SETSQ (sx, ey - 1, kSWCorner);
    SETSQ (ex - 1, sy, kNECorner);
    SETSQ (ex - 1, ey - 1, kSECorner);

    /* add doors */

    while ((doortries < 122) and
           (ndoors < 1 + RNG (ex - sx) / 4))
    {
        if (RNG (2)) {
            x = RNG (sx + 1, ex - 2);
            y = RNG (2) ? sy : ey -1;
            if (TESTSQ (x, y - 1, kFloor) and
                TESTSQ (x, y + 1, kFloor))
            {
                SETSQ (x, y, kFloor);
                addDoor (x, y, 0 == RNG (4), 0 == RNG (5), -1);
                ++ndoors;
            }
        }
        else {
            y = RNG (sy + 1, ey - 1);
            x = RNG (2) ? sx : ex - 1;
            if (TESTSQ (x - 1, y, kFloor) and
                TESTSQ (x + 1, y, kFloor))
            {
                SETSQ (x, y, kFloor);
                addDoor (x, y, 0 == RNG (4), 0 == RNG (5), -1);
                ++ndoors;
            }
        }
        ++doortries;
    }

    return ndoors;
}


void
shMapLevel::buildSnuggledRooms ()
{
    int x;
    int y;
    int ey;

    for (y = 0; y < mRows; y++) {
        for (x = 0 ; x < mColumns; x++) {
            if (RNG (4)) {
                x += RNG (2);
            }
            ey = fiveByFiveClearance (x, y);
            if (ey) {
                if (ey > y + 9) {
                    ey = y + 5;
                }
                else if (ey > y + 5) {
                    ey = RNG (y + 5, ey);
                }
//              draw ();
//              I->getChar ();
                buildRoomOrElRoom (x, y, ey);

                x += 5;
            }
        }
    }
}


void
shMapLevel::wallCorridors ()
{
    int x;
    int y;

    for (x = 0; x < mColumns; x++) {
        for (y = 0; y < mColumns; y++) {
            if (kStone == GETSQ (x, y)) {
                int below = 0;
                int above = 0;
                int left = 0;
                int right = 0;

                left = TESTSQFLAG (x - 1, y, kHallway);
                right = TESTSQFLAG (x + 1, y, kHallway);
                above = TESTSQFLAG (x, y - 1, kHallway);
                below = TESTSQFLAG (x, y + 1, kHallway);

                if (above or below) {
                    SETSQ (x, y, kHWall);
                }
                else if (right or left) {
                    SETSQ (x, y, kVWall);
                }
            }
        }
    }
}


void
shMapLevel::buildTwistyRooms ()
{
    int i = 1000;
    int n = 0;
    int x, y;

    n = buildTwistyCorridor (RNG (mColumns / 3, 2 * mColumns / 3),
                             4, kSouth);
    while (--i and n < 4 * (mRows + mColumns)) {
        do {
            x = RNG (mColumns);
            y = RNG (mRows);
        } while (!TESTSQ (x, y, kFloor));
        n += buildTwistyCorridor (x, y, (shDirection) (2 * RNG (4)));
        if (n > 4 * (mRows + mColumns)) break;
    }


    buildSnuggledRooms ();
//    wallCorridors ();

    n = RNG (1, 6) + RNG (1, 6) + RNG (1, 6) + RNG (1, 6);
    for (i = 0; i < n; i++) {
        do {
            x = RNG (mColumns);
            y = RNG (mRows);
        } while (!TESTSQ (x, y, kFloor)  or
                 isObstacle (x, y));
        putObject (generateObject (), x, y);
    }

}
