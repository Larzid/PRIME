#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "Util.h"
#include "Map.h"
#include "Monster.h"
#include "Object.h"

#include "MapBuilder.h"


int
shMapLevel::buildBunkerRoom (int sx, int sy, int ex, int ey)
{
    int x, y;

//    I->debug ("bunker room %d %d : %d %d", sx, sy, ex, ey);

    for (x = sx + 1; x <= ex -1; x++) {
        for (y = sy + 1; y <= ey -1; y++) {
            SETSQ (x, y, kFloor);
        }
    }
    return 1;
}


// RETURNS: 1 on success, 0 on failure

int
shMapLevel::buildBunkerRooms ()
{
#define MSX 10
#define MSY 4
#define MAXROOMS 20
/* macro sq 5x6 cells of regular squares */
    int macrosq[MSX][MSY];
    int x, y;
    int i, n;
    int retry;
    int firstroom;
    struct {
        int sx, sy, ex, ey;      /* macro dimensions */
        int rsx, rsy, rex, rey;  /* real dimensions */
        int mark;
    } rooms[MAXROOMS];

    char connect [MAXROOMS][MAXROOMS];
    int maxcorridorlength = 3;


    mMapType = kBunkerRooms;

//    I->debug ("building bunker rooms");
    n = 7 + RNG (3) + RNG (3);

//    n = 5;

    for (x = 0; x < MSX; x++) for (y = 0; y < MSY; y++)
        macrosq[x][y] = -1;
    for (x = 0; x < MAXROOMS; x++) for (y = 0; y < MAXROOMS; y++)
        connect[x][y] = 0;
    firstroom = 0;

    /* first step: determine the macro dimensions of the rooms */
reloop:
    retry = 0;
    for (i = firstroom; i < n; ++i) {
        rooms[i].mark = 0;
    retry:
        if (++retry > 100) {
            n = i;
            break;
        }

        x = RNG (MSX);

        if (RNG (2)) {
            /* a wide room */
            y = RNG (MSY);
            if (-1 == macrosq[x][y] and x < MSX - 1 and -1 == macrosq[x+1][y]) {
                if (RNG(3) and y > 0 and
                    (-1 != macrosq[x][y-1] or -1 != macrosq[x+1][y-1]))
                {
                    goto retry;
                }
                if (RNG(2) and y < MSY - 1 and
                    (-1 != macrosq[x][y+1] or -1 != macrosq[x+1][y+1]))
                {
                    goto retry;
                }

                macrosq[x][y] = i;
                macrosq[x+1][y] = i;
                rooms[i].sx = x;
                rooms[i].sy = y;
                if (RNG(4) and x < MSX - 2 and -1 == macrosq[x+2][y]) {
                    macrosq[x+2][y] = i;
                    rooms[i].ex = x+2;
                } else {
                    rooms[i].ex = x+1;
                    if (y < MSY - 1 and
                        -1 == macrosq[x][y+1] and -1 == macrosq[x+1][y+1])
                    {
                        macrosq[x][y+1] = i;
                        macrosq[x+1][y+1] = i;
                        rooms[i].ey = y + 1;
                    }
                }
                rooms[i].ey = y;
            } else {
                goto retry;
            }
        } else {
            /* a tall room */
            y = RNG (MSY-1);
            if (-1 == macrosq[x][y] and -1 == macrosq[x][y+1]) {
                macrosq[x][y] = i;
                macrosq[x][y+1] = i;
                rooms[i].sx = x;
                rooms[i].sy = y;
                rooms[i].ey = y+1;
                rooms[i].ex = x;
                if (RNG (4) and x < MSX - 1 and
                    -1 == macrosq[x+1][y] and -1 == macrosq[x+1][y+1])
                {
                    macrosq[x+1][y] = i;
                    macrosq[x+1][y+1] = i;
                    rooms[i].ex = x+1;
                }
                if (RNG (4) and x > 0 and
                    -1 == macrosq[x-1][y] and -1 == macrosq[x-1][y+1])
                {
                    macrosq[x-1][y] = i;
                    macrosq[x-1][y+1] = i;
                    rooms[i].sx = x-1;
                }
            } else {
                goto retry;
            }
        }
    }

    /* deterimine real dimensions, and actually make the rooms: */

    for (i = firstroom; i < n; i++) {
        int w, h;

        w = 6 * (rooms[i].ex - rooms[i].sx);
        if (w > 0) w -= 2;
        w = 5 + RNG (2) + RNG (w / 2, w);
        x = (RNG (1 + 6 * (1 + rooms[i].ex - rooms[i].sx) - w) +
             RNG (1 + 6 * (1 + rooms[i].ex - rooms[i].sx) - w)) / 2;

        if (rooms[i].sy != rooms[i].ey) {
            h = 5 + RNG (5);
            y = RNG (1 + 5 * (1 + rooms[i].ey - rooms[i].sy) - h);
        } else {
            h = RNG (5) ? 5 : 4;
            y = RNG (1 + 5 - h);
        }

        rooms[i].rsx = rooms[i].sx * 6 + x;
        rooms[i].rex = rooms[i].rsx + w - 1;
        rooms[i].rsy = rooms[i].sy * 5 + y;
        rooms[i].rey = rooms[i].rsy + h - 1;
        layRoom (rooms[i].rsx, rooms[i].rsy,
                 rooms[i].rex, rooms[i].rey);

    }

    /* connect the rooms with corridors */

corridor:

    for (i = 0; i < n; i++) {
        int j;
        int cnt;
        int dir[4];

        dir[0] = kNorth;
        dir[1] = kEast;
        dir[2] = kWest;
        dir[3] = kSouth;

        shuffle (dir, 4, sizeof (int));
        for (j = 0; j < 4; j++) {
            int sx, sy, ex, ey;
            x = RNG (rooms[i].sx, rooms[i].ex);
            y = RNG (rooms[i].sy, rooms[i].ey);
            sx = x;
            sy = y;

            assert (x >= 0 and y >= 0);

            for (cnt = maxcorridorlength; cnt; --cnt) {
                int room;
                moveForward ((shDirection) dir[j], &x, &y);
                if (x < 0 or x >= MSX or y < 0 or y >= MSY) {
                    break;
                }
                room = macrosq[x][y];

                if (i == room) {
                    sx = x; sy = y;
                    continue;
                }
                else if (-2 == room) {
                    /* we hit another corridor */
                    break;
                }
                else if (-1 != room) {
                    /* we hit another room */

                    if (connect[i][room]) {
                        /* but there's already a connecting corridor*/
                        break;
                    }

                    if (isHorizontal ((shDirection) dir[j])) {
                        int lo, hi;

                        lo = maxi (y * 5,
                                   1 + maxi(rooms[room].rsy, rooms[i].rsy));
                        hi = mini (y * 5 + 4,
                                   mini(rooms[room].rey, rooms[i].rey) - 1);
                        if (lo > hi) {
                            break;
                        }

                        ex = maxi (x, sx);
                        sx = mini (x, sx);
                        for (x = sx + 1; x < ex; x++) {
                            macrosq[x][y] = -2;
                        }
                        sx = mini (rooms[room].rex, rooms[i].rex);
                        ex = maxi (rooms[room].rsx, rooms[i].rsx);
/*
                        I->debug ("y %d  (%d, %d) (%d, %d)",
                                  y,
                                  rooms[room].rsy, rooms[room].rey,
                                  rooms[i].rsy, rooms[i].rey);
*/
                        y = RNG (lo, hi);

                        /* Test for neighboring corridors or rooms. */
                        for (x = sx + 1; x <= ex - 1; ++x) {
                            if (GETSQ (x, y) != kStone) break;
                        }
                        if (x != ex) break;

                        int dark = RNG (mDLevel + 6) > 7;

                        for (x = sx; x <= ex; x++) {
                            SETSQ (x, y, kFloor);
                            SETSQFLAG (x, y, kHallway);
                            if (kStone == GETSQ (x, y - 1))
                              SETSQ (x, y - 1, kHWall);
                            if (kStone == GETSQ (x, y + 1))
                              SETSQ (x, y + 1, kHWall);

                            if (dark) { //NW NE SW SE
                                setLit (x, y, x > sx ? -1 : 0,
                                              x < ex ? -1 : 0,
                                              x > sx ? -1 : 0,
                                              x < ex ? -1 : 0);
                                setLit (x, y - 1, 0,  0,
                                                  x > sx ? -1 : 0,
                                                  x < ex ? -1 : 0);
                                setLit (x, y + 1, x > sx ? -1 : 0,
                                                  x < ex ? -1 : 0,
                                                  0, 0);
                            }
                        }
                        addDoor (sx, y, 0);
                        addDoor (ex, y, 0);
                    } else { /* vertical */
                        if (rooms[room].rsx + 1 > rooms[i].rex - 1 or
                            rooms[room].rex - 1 < rooms[i].rsx + 1)
                        {
                            break;
                        }
                        ey = maxi (y, sy);
                        sy = mini (y, sy);
                        for (y = sy + 1; y < ey; y++) {
                            macrosq[x][y] = -2;
                        }

                        sy = mini (rooms[room].rey, rooms[i].rey);
                        ey = maxi (rooms[room].rsy, rooms[i].rsy);
                        x = RNG (1 + maxi(rooms[room].rsx, rooms[i].rsx),
                                 mini(rooms[room].rex, rooms[i].rex) - 1);

                        /* Test for neighboring corridors or rooms. */
                        for (y = sy + 1; y <= ey - 1; ++y) {
                            if (GETSQ (x, y) != kStone) break;
                        }
                        if (y != ey) break;

                        int dark = RNG (mDLevel + 6) > 7;

                        for (y = sy; y <= ey; y++) {
                            SETSQ (x, y, kFloor);
                            SETSQFLAG (x, y, kHallway);
                            if (kStone == GETSQ (x - 1, y))
                              SETSQ (x - 1, y, kVWall);
                            if (kStone == GETSQ (x + 1, y))
                              SETSQ (x + 1, y, kVWall);

                            if (dark) {
                                setLit (x, y, y > sy ? -1 : 0,
                                              y > sy ? -1 : 0,
                                              y < ey ? -1 : 0,
                                              y < ey ? -1 : 0);
                                setLit (x - 1, y, 0, y > sy ? -1 : 0,
                                                  0, y < ey ? -1 : 0);
                                setLit (x + 1, y, y > sy ? -1 : 0, 0,
                                                  y < ey ? -1 : 0, 0);
                            }

                        }
                        addDoor (x, sy, 1);
                        addDoor (x, ey, 1);
                    }
                    j += RNG (3);
                    connect[i][room] = 1;
                    connect[room][i] = 1;
                    break;
                }
            }
        }
    }

    /* The final check: are the rooms connected?
       Do a breadth first search, marking all the rooms reachable from room 0
    */

    {
        int j;
        int k;
        int bfs[MAXROOMS];

        for (i = 0; i < n; ++i) {
            bfs[i] = -1;
            rooms[i].mark = 0;
        }
        k = 1;
        bfs[0] = 0;
        rooms[0].mark = 1;

        for (i = 0; i < k; i++) {
            int room = bfs[i];
            for (j = 0; j < n; j++) {
                if (room != j and
                    (connect[j][room] or connect[room][j]) and
                    0 == rooms[j].mark)
                {
                    /* mark this room, add it to the search queue: */
                    rooms[j].mark = 1;
                    bfs[k++] = j;
                }
            }
        }

        if (k != n) {
//          I->debug ("not connected!!! %d %d", k, n);

            /* add a new room? */
            if (RNG (3) and n < MAXROOMS) {
                firstroom = n;
                n++;
                //I->debug ("desperately adding a room!");
                goto reloop;
            }

            /* try again, with longer corridors */
            ++maxcorridorlength;
            if (maxcorridorlength < MSX) {
                goto corridor;
            } else { /* miserable failure! */
                //I->debug ("giving up :-(");
                for (i = 0; i < mFeatures.count (); i++) {
                    delete mFeatures.get(i);
                }
                mFeatures.reset ();
                return 0;
            }
        } else {
            //I->debug ("connected! %d", n);
        }
    }

    for (i = 0; i < n; i++) {
        decorateRoom (rooms[i].rsx, rooms[i].rsy,
                      rooms[i].rex, rooms[i].rey);
    }

    /* All newly dug tunnels will be dark. */
    for (int y = 0; y < mRows ; ++y) {
        for (int x = 0; x < mColumns; ++x) {
            if (kStone == GETSQ (x, y))
                setLit (x, y, -1, -1, -1, -1);
        }
    }
    /*
    for (y = 0; y < MSY; y++) {
        char buff[MSX+1];
        for (x = 0; x < MSX; x++) {
            macrosq[x][y];
            buff[x] = c >= 0 ? c + 'A' : c == -1 ? ' ' : '0' - c;
        }
        buff[MSX] = 0;
        I->debug ("%s", buff);
    }
    */

    return 1;

#undef MSX
#undef MSY
}
