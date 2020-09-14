#include "Monster.h"

/* implementation of A* Algorithm */
static int
estimateDistance (int ox, int oy, int dx, int dy)
{
    return FULLTURN * distance (ox, oy, dx, dy) / 5;
}


//REQUIRES: (ox, oy) adjacent to (dx, dy)
//RETURNS: cost in ms to move between the squares, 9999999 if impossible
static int
moveCost (shMonId id, int ox, int oy, int dx, int dy)
{
    int cost = 0;
    shFeature *f;

    f = Level->getFeature (dx, dy);
    if (f and shFeature::kDoorClosed == f->mType) {
        if (f->isLockedDoor () and (id != kMonDalek or f->isLockBrokenDoor ())) {
            return 9999999; /* Only Daleks can open locked doors */
        }                   /* unless the lock is broken. */
        if ((!(f->mDoor.mFlags & shFeature::kAutomatic) or id == kMonFuelBarrel)
            and 0 == MonIlks[id].mNumHands)
        {   /* No hands - cannot open doors. */
            return 9999999;
        } else {
            cost += FULLTURN;
        }
    } else if (Level->isObstacle (dx, dy)) {
        return 9999999;
    }
    if (ox == dx or oy == dy) {
        cost += FULLTURN;
    } else {
        cost += DIAGTURN;
    }
    return cost;
}


struct PathNode
{
    struct PathNode *prev;
    int g;
    int h;
    int f;
    char isopen;
    char isclosed;
};


inline static int
pathNodeCompareFunc (PathNode *a, PathNode *b)
{
    if (a->f < b->f) {
        return -1;
    } else if (a->f == b->f) {
        return 0;
    } else {
        return 1;
    }
}


static PathNode data[MAPMAXCOLUMNS*MAPMAXROWS];

int
calcShortestPath (int ox, int oy, int dx, int dy,
                  shMonId id,
                  shDirection *resultbuf, int buflen)
{
    int x, y;
    int nx, ny;
    int i;

//    char dirty[MAPMAXCOLUMNS*MAPMAXROWS];
    shHeap <PathNode *> Open (pathNodeCompareFunc);
    shVector <PathNode *> Closed;

    int c1 = 0;
    int c2 = 0;

//    memset (&data, 0, sizeof (data));

#define getnode(_x, _y) (&data[_y * MAPMAXCOLUMNS + _x])
#define getnodex(_n) ((((char *) _n - (char *) data) / sizeof (PathNode)) % MAPMAXCOLUMNS)
#define getnodey(_n) ((((char *) _n - (char *) data) / sizeof (PathNode)) / MAPMAXCOLUMNS)

    PathNode *node;
    PathNode *newnode;

    node = getnode (ox, oy);
    node->g = 0;
    node->h = estimateDistance (ox, oy, dx, dy);
    node->f = node->g + node->h;
    node->prev = NULL;
    Open.add (node);
    node->isopen = 1;

    while (Open.count ()) {
        node = Open.extract ();
        node->isopen = 0;
        ++c1;

        ny = getnodey (node);
        nx = getnodex (node);

//      debug.log ("popping open node %d %d, f=%d", nx, ny, node->f);

        if (nx == dx and ny == dy) {

            /* we found it! */

            int i, j, n;

            n = 0;
            while ((node = node->prev) and n < buflen) {
                x = getnodex (node);
                y = getnodey (node);
                resultbuf[n++] = vectorDirection (x, y, nx, ny);
                nx = x;
                ny = y;
            }
            /* reverse */
            for (j = 0, i = n - 1; j < i; ++j, --i) {
                shDirection tmp = resultbuf[i];
                resultbuf[i] = resultbuf[j];
                resultbuf[j] = tmp;
            }

            {
                char pathbuf[100];
                int l = snprintf (pathbuf, 80, "result: ");
                for (i = 0; i < n and i < 15; i++) {
                    l += snprintf (&pathbuf[l], 99 - l, "%s ",
                                   stringDirection (resultbuf[i]));
                }
                debug.log ("  path %d,%d -> %d,%d; c1=%d c2=%d; %s",
                          ox, oy, dx, dy, c1, c2, pathbuf);
            }

            for (i = Open.count () - 1; i >= 0; i--) {
                Open.get (i) -> isopen = 0;
            }

            for (i = Closed.count () - 1; i >= 0; i--) {
                Closed.get (i) -> isclosed = 0;
            }

            return n;
        }


        for (x = nx - 1; x <= nx + 1; x++) {
            for (y = ny - 1; y <= ny + 1; y++) {
                int cost;
                if ((x == nx and y == ny) or !Level->isInBounds (x, y)) {
                    continue;
                }
                ++c2;
                cost = node->g + moveCost (id, nx, ny, x, y);
                if (cost > 200000) {
                    //debug.log ("too expensive to reach %d %d (%d)", x, y, cost);
                    continue;
                }
                newnode = getnode (x, y);
//              if ((-1 != Open.find (newnode) or -1 != Closed.find (newnode))
                if ((newnode->isopen or newnode->isclosed)
                    and newnode->g <= cost)
                {
                    //debug.log ("already a cheaper way to reach %d %d", x, y);
                    continue;
                }
                newnode->g = cost;
                newnode->h = estimateDistance (x, y, dx, dy);
                newnode->f = newnode->g + newnode->h;
                newnode->prev = node;

                //debug.log ("planning node %d %d, f=%d", x, y, newnode->f);

                if (1 == newnode->isclosed) {
                    Closed.remove (newnode);
                    newnode->isclosed = 0;
                }
                if (0 == newnode->isopen) {
                    Open.insert (newnode);
                    newnode->isopen = 1;
                }
            }
        }
        Closed.add (node);
        node->isclosed = 1;
    }
    /* no path found! */
    debug.log ("  path %d,%d -> %d,%d; c1=%d c2=%d; none found",
              ox, oy, dx, dy, c1, c2);

    for (i = Open.count () - 1; i >= 0; i--) {
        Open.get (i) -> isopen = 0;
    }
    for (i = Closed.count () - 1; i >= 0; i--) {
        Closed.get (i) -> isclosed = 0;
    }

    return 0;
}
