#include <stdlib.h>
#include <string.h>
#include "Util.h"
#include "Map.h"
#include "Monster.h"
#include "Object.h"
#include "Hero.h"
#include "MapBuilder.h"
#include "Services.h"

bool
shRoom::is_shop ()
{
    return mType >= kGeneralStore and mType <= kCanisterStore;
}

int
shMapLevel::testSquares (int x1, int y1, int x2, int y2, shTerrainType what)
{
    int x, y;

    for (x = x1; x <= x2; x++) {
        for (y = y1; y <= y2; y++) {
            if (!TESTSQ (x, y, what)) {
                return 0;
            }
        }
    }
    return 1;
}


/* this routine determines if this room is some kind of special room,
   and fills it with objects and monsters and stuff.
 */

void
shMapLevel::decorateRoom (int sx, int sy, int ex, int ey)
{
    if ((ex-sx) * (ey-sy) <= 20 + RNG (1, 20) and
        mDLevel > 1 and
        !RNG (1 + mDLevel) and
        !(kHasShop & mFlags) and
        makeShop (sx, sy, ex, ey))
    {
        return;
    }

    while (!RNG (8)) {  /* secret treasure niche */
        int x, y, dx, dy, horiz;
    failedniche:
        if (!RNG (222)) break;
        if (RNG (2)) {
            x = RNG (sx + 1, ex - 1);
            dx = x;
            horiz = 1;
            if (RNG (2)) {
                if (!testSquares (x - 1, sy - 2, x + 1, sy - 1, kStone))
                    goto failedniche;
                y = sy - 1;
                dy = sy;
            } else {
                if (!testSquares (x - 1, ey + 1, x + 1, ey + 2, kStone))
                    goto failedniche;
                y = ey + 1;
                dy = ey;
            }
        } else {
            y = RNG (sy + 1, ey - 1);
            dy = y;
            horiz = 0;
            if (RNG (2)) {
                if (!testSquares (sx - 2, y - 1, sx - 1, y + 1, kStone))
                    goto failedniche;
                x = sx - 1;
                dx = sx;
            } else {
                if (!testSquares (ex + 1, y - 1, ex + 2, y + 1, kStone))
                    goto failedniche;
                x = ex + 1;
                dx = ex;
            }
        }
        SETSQ (x-1, y-1, kHWall);
        SETSQ (x,   y-1, kHWall);
        SETSQ (x+1, y-1, kHWall);
        SETSQ (x-1, y,   kVWall);
        SETSQ (x,   y,   kFloor);
        SETSQ (x+1, y,   kVWall);
        SETSQ (x-1, y+1, kHWall);
        SETSQ (x,   y+1, kHWall);
        SETSQ (x+1, y+1, kHWall);

        SETSQ (dx, dy, kFloor);
        addDoor (dx, dy, horiz, 0, RNG (6), 1, !RNG(3), !RNG(10));
        SETSQFLAG (x, y, kHallway);
        if (RNG (6)) putObject (generateObject (), x, y);
        if (RNG (2)) putObject (generateObject (), x, y);

    }

    if (mDLevel > 7 and !RNG (15)) {
        /* monolith room */
        shCreature *monl = shCreature::monster (kMonMonolith);
        putCreature (monl, (sx + ex) / 2, (sy + ey) / 2);
        return;
    }

    if ((mDLevel > 6) and !RNG (22) and
        makeNest (sx, sy, ex, ey))
    {
        return;
    }

    if (mDLevel > 1 and mDLevel < 10 and
        !(kHasMelnorme & mFlags) and !RNG (30))
    {   /* Melnorme room. */
        shCreature *trader = shCreature::monster (kMonMelnorme);
        int dx = (sx + ex + RNG (0, 1)) / 2;
        int dy = (sy + ey + RNG (0, 1)) / 2;
        putCreature (trader, dx, dy);
        debug.log ("placed Melnorme on level %d", mDLevel);
        mFlags |= kHasMelnorme;
    }   /* No return statement.  Trade post is a mundane room. */

    mundaneRoom (sx, sy, ex, ey);
}

void
shMapLevel::darkRoom (int sx, int sy, int ex, int ey)
{
    for (int x = sx; x <= ex; ++x) {
        for (int y = sy; y <= ey; ++y) {
            setLit (x, y,
                    x > sx and y > sy ? -1 : 0,  //NW
                    x < ex and y > sy ? -1 : 0,  //NE
                    x > sx and y < ey ? -1 : 0,  //SW
                    x < ex and y < ey ? -1 : 0); //SE
        }
    } /* Chances are someone broke/stole/ate the lightbulb. -- MB */
    if (!RNG (4)) {
        mSquares[(sx+ex)/2][(sy+ey)/2].mTerr = kBrokenLightAbove;
    }
}

void
shMapLevel::mundaneRoom (int sx, int sy, int ex, int ey)
{
    int i, npiles, n;
    int x, y;

    if (!isTownLevel ()) {
        if (RNG (mDLevel + 6) > 9) {
            darkRoom (sx, sy, ex, ey);
        }

        if (!RNG (30)) {
            /* Radioactive room. */
            for (x = sx + 1; x < ex; x++) {
                for (y = sy + 1; y < ey; y++) {
                    mSquares[x][y].mFlags |= shSquare::kRadioactive;
                }
            }
        }
    }

    /* treasure expectation per room:
       version 0.3:
         0.375 piles x 1.1875 obj per pile +
         (~) 0.125 niches x 0.8 obj per niche
         ~ .5453125 objs per room

       version 0.2.9:
         0.375 piles x 1.375 obj per pile
         = .515625 obj per room
   */

    npiles = RNG (4) ? 0 : RNG (2) + RNG (2) + RNG (2);

    while (npiles--) {
        x = RNG (sx + 1, ex - 1);
        y = RNG (sy + 1, ey - 1);
        if (TESTSQ (x, y, kFloor)  and !isObstacle (x, y)) {
            n = RNG (10) ? 1 : RNG (2, 3);
            for (i = 0; i < n; i++) {
                putObject (generateObject (), x, y);
            }
        }
    }

    if (0 == RNG (9)) {
        x = RNG (sx + 1, ex - 1);
        y = RNG (sy + 1, ey - 1);
        if (TESTSQ (x, y, kFloor) and !isObstacle (x, y) and
            0 == countObjects (x, y))
        {
            addVat (x, y);
        }
    }

    /* Reticulans of PRIME are weak enough to get instakilled in a pit.
       Do not place traps at all on first level. */
    while (!RNG (12) and mDLevel > 1) {
        x = RNG (sx + 1, ex - 1);
        y = RNG (sy + 1, ey - 1);
        if (TESTSQ (x, y, kFloor) and !isObstacle (x, y) and
            !getFeature (x, y))
        {
            shFeature::Type ttype;
            bool accepted = false;

            do {
                ttype = (shFeature::Type) RNG (shFeature::kPit,
                                              shFeature::kPortal);
                switch (ttype) {
                case shFeature::kPit:
                case shFeature::kTrapDoor:
                    accepted = true;
                    break;
                case shFeature::kFloorGrating:
                case shFeature::kACMESign:
                    if (isSpaceBase () or isTownLevel ())  accepted = true;
                    break;
                case shFeature::kAcidPit:
                    if (mDLevel >= 7)  accepted = true;
                    break;
                case shFeature::kHole:
                    if (!RNG (5))  accepted = true;
                    break;
                case shFeature::kRadTrap:
                    if (mDLevel >= 5)  accepted = true;
                    break;
                case shFeature::kWeb:
                case shFeature::kPortal:
                default: /* these traps are unimplemented */
                    break;
                }
            } while (!accepted);

            addTrap (x, y, ttype);
        }
    }
}


int
shMapLevel::makeGarbageCompactor (int sx, int sy, int ex, int ey)
{
    int x;
    int my;
    int roomid = mSquares[sx][sy].mRoomId;

    mRooms[roomid].mType = shRoom::kGarbageCompactor;

    if ((sy + ey) % 2) {
        return 0;
    }
    my = (sy + ey) / 2;

    for (x = sx + 1; x <= ex -1; x++) {
        addMovingHWall (x, sy+1, sy+1, my);     //north wall
        addMovingHWall (x, ey-1, ey-1, my+1);   //south wall
    }

    mTimeOuts.mCompactor = -1;
    mCompactorState = 0;
    mCompactorHacked = 0;

    return 1;
}


shFeature *
shMapLevel::addMovingHWall (int x, int y, int beginy, int endy)
{
    shFeature *wall = new shFeature ();
    wall->mType = shFeature::kMovingHWall;
    wall->mX = x;
    wall->mY = y;
    wall->mMovingHWall.mBeginY = beginy;
    wall->mMovingHWall.mEndY = endy;
//    wall->mMovingHWall.mRoomId = roomid;

    mFeatures.add (wall);
    return wall;
}


shFeature *
shMapLevel::addMachinery (int x, int y)
{
    shFeature *f = new shFeature ();
    f->mType = shFeature::kMachinery;
    f->mX = x;
    f->mY = y;
    mFeatures.add (f);
    return f;
}


void
shMapLevel::moveWalls (int action)
{
    shFeature *f;

    int north = (1 == action or -1 == action) ? 1 : 0;

    int squares[20];
    int n = 0;
    int x, y;
    int i, j;
    int seen = 0;
    int interrupt = 0;
    int heard = 0;

    // move the wall
    y = -1;
    for (i = 0; i < mFeatures.count (); i++) {
        f = mFeatures.get (i);
        if (shFeature::kMovingHWall != f->mType) {
            continue;
        }
        if (action > 0) {
            /* close in */
            if (north and f->mMovingHWall.mBeginY < f->mMovingHWall.mEndY and
                f->mY < f->mMovingHWall.mEndY)
            {
                //if (Hero.canSee (f->mX, f->mY))
                //    setMemory (f->mX, f->mY, ' ');
                mVisibility[f->mX][f->mY] = 0;
                y = f->mY + 1;
                squares[n++] = f->mX;
                f->mY++;
                addMachinery (f->mX, f->mY-1);
            } else if (!north and f->mMovingHWall.mBeginY > f->mMovingHWall.mEndY and
                f->mY > f->mMovingHWall.mEndY)
            {
                //if (Hero.canSee (f->mX, f->mY))
                //    setMemory (f->mX, f->mY, ' ');
                mVisibility[f->mX][f->mY] = 0;
                y = f->mY - 1;
                squares[n++] = f->mX;
                f->mY--;
                addMachinery (f->mX, f->mY+1);
            } else {
                continue;
            }
            if (Hero.cr ()->canSee (f->mX, f->mY)) {
                interrupt++;
                seen++;
            } else if (distance (Hero.cr (), f->mX, f->mY) < 5 * 20) {
                heard++;
            }
        } else if (action < 0) {
            /* reset */
            int oldy = f->mY;
            if (north and f->mMovingHWall.mBeginY < f->mMovingHWall.mEndY and
                f->mY > f->mMovingHWall.mBeginY)
            {
                y = f->mY - 1;
                shFeature *machinery = getFeature (f->mX, y);
                if (machinery)
                    removeFeature (machinery);
                f->mY--;
            } else if (!north and
                       f->mMovingHWall.mBeginY > f->mMovingHWall.mEndY and
                       f->mY < f->mMovingHWall.mBeginY)
            {
                y = f->mY + 1;
                shFeature *machinery = getFeature (f->mX, y);
                if (machinery)
                    removeFeature (machinery);
                f->mY++;
            } else {
                continue;
            }
            if (Hero.cr ()->canSee (f->mX, oldy))
                interrupt++;
        }
    }

    if (!Hero.getStoryFlag ("walls moving")) {
        if (seen) {
            I->p ("The walls are moving!");
            Hero.setStoryFlag ("walls moving", 1);
        } else if (heard and !Hero.getStoryFlag ("walls heard")) {
            I->p ("You hear a loud rumbling!");
            Hero.setStoryFlag ("walls heard", 1);
        }
    }

    // displace objects and creatures

    if (n) {
        shuffle (squares, n, sizeof(int));

        for (i = 0; i < n; i++) {
            x = squares[i];

            shObjectVector *v = getObjects (x, y);
            if (v) {
                int y2 = north ? y + 1 : y - 1;
                int safe = !isObstacle (x, y2);
                for (j = 0; j < v->count (); j++) {
                    shObject *obj = v->get (j);
                    if (safe) {
                        putObject (obj, x, y2);
                    } else {
                        delete obj;
                    }
                }
                if (Hero.cr ()->mX == x and Hero.cr ()->mY == y2) {
                    I->p ("%s pushed into your vicinity.",
                          v->count () > 1 ? "Some objects are"
                                          : "An object is");
                }
                delete v;
                setObjects (x, y, NULL);
            }

            shCreature *c = getCreature (x, y);
            if (c) {
                if (c->mZ < 0) {
                    if (c->isHero ()) {
                        I->p ("You are sealed below the moving wall!");
                        I->p ("That's not supposed to be possible!");
                        I->p ("Please send me a bug report! -cyrus");
                    }
                } else {
                    pushCreature (c, north ? kSouth : kNorth);
                }
            }
        }
    }
    if (interrupt)
        Hero.interrupt ();
}


//RETURNS: 0 on success, -1 o/w
int
shMapLevel::pushCreature (shCreature *c, shDirection dir)
{
    shDirection dirlist[3];
    int r = RNG (1, 2);
    int i;

    dirlist[0] = dir;
    dirlist[r] = (shDirection) ((dir + 1) % 8);
    dirlist[3-r] = (shDirection) ((dir + 7) % 8);

    for (i = 0; i < 3; i++) {
        int x = c->mX;
        int y = c->mY;
        shDirection dir = dirlist[i];

        moveForward (dir, &x, &y);

        if (isObstacle (x, y) or isOccupied (x, y))
            continue;

        return moveCreature (c, x, y);
    }

    c->msg ("You are crushed between the walls!") or
        c->appear (fmt ("%s is crushed!", THE (c)));
    c->die (kMisc, "Crushed by a garbage compactor");
    return -1;
}





void
shMapLevel::magDoors (int action)  /* 1 lock, -1 unlock */
{
    shFeature *f;
    int i;
    int heard = 0;

    for (i = 0; i < mFeatures.count (); i++) {
        f = mFeatures.get (i);
        if (f->isMagneticallySealed ()) {
            if (action > 0 and !f->isOpenDoor () and !f->isLockedDoor ()) {
                // TODO: if the door is open, slam it shut
                f->lockDoor ();
            } else if (action < 0 and !f->isOpenDoor () and f->isLockedDoor ()) {
                f->unlockDoor ();
            } else {
                continue;
            }
            if (distance (Hero.cr (), f->mX, f->mY) < 5 * 10) {
                heard++;
            }
        }
    }

    if (heard) {
        I->p ("You hear %s.", heard > 1 ? "some clicks" : "something click");
    }
}


int
shMapLevel::makeHospital (int sx, int sy, int ex, int ey)
{
    int dx;
    int dy;
    int roomid = mSquares[sx][sy].mRoomId;

    dx = (sx + ex + RNG (0, 1)) / 2;
    dy = (sy + ey + RNG (0, 1)) / 2;

    shCreature *docbot = shCreature::monster (kMonDocbot);
    putCreature (docbot, dx, dy);
    docbot->mDoctor.mHomeX = dx;
    docbot->mDoctor.mHomeY = dy;
    docbot->mDoctor.mRoomID = roomid;
    for (int i = 0; i < kMedMaxService; ++i)
        docbot->mDoctor.mPermute[i] = i;
    shuffle (docbot->mDoctor.mPermute, kMedMaxService, sizeof (int));

    mRooms[roomid].mType = shRoom::kHospital;

    debug.log ("made hospital on level %d", mDLevel);

    return 1;
}


/* returns 1 if shop successfully created, 0 o/w */
int
shMapLevel::makeShop (int sx, int sy, int ex, int ey, int kind)
{
    int dx = -1, gx = -1;
    int dy = -1, gy = -1;
    int roomid = mSquares[sx][sy].mRoomId;

    /* Find the door (we only make shops in rooms with exactly one door). */
    shFeature *door = NULL;
    shFeature *f;
    for (int x = sx + 1; x < ex; x++) {
        f = getFeature (x, sy);
        if (f and f->isDoor ()) {
            if (door) return 0;
            door = f;
            dx = x;
            dy = sy;
        }
        f = getFeature (x, ey);
        if (f and f->isDoor ()) {
            if (door) return 0;
            door = f;
            dx = x;
            dy = ey;
        }
    }
    for (int y = sy + 1; y < ey; y++) {
        f = getFeature (sx, y);
        if (f and f->isDoor ()) {
            if (door) return 0;
            door = f;
            dx = sx;
            dy = y;
        }
        f = getFeature (ex, y);
        if (f and f->isDoor ()) {
            if (door) return 0;
            door = f;
            dx = ex;
            dy = y;
        }
    }

    if (!door)  return 0;  /* What?! There are no doors to this room? */

    /* Make sure door is a normal automatic door. */
    door->mType = shFeature::kDoorClosed;
    door->mDoor.mFlags = shFeature::kAutomatic;

    /* First row or column from the door shall be empty. */
    if (dx == sx) {
        sx++; dx++; gx = sx;
    } else if (dx == ex) {
        ex--; dx--; gx = ex;
    }
    if (dy == sy) {
        sy++; dy++; gy = sy;
    } else if (dy == ey) {
        ey--; dy--; gy = ey;
    }

    if (-1 == kind) { /* Randomly determine kind of shop. */
        switch (RNG (20)) {
        case 0: case 1: case 2: case 3: case 4:
        case 5: case 6: case 7:
            kind = shRoom::kGeneralStore; break;
        case 8: case 9: case 10:
            kind = shRoom::kHardwareStore; break;
        case 11: case 12: case 13:
            kind = shRoom::kSoftwareStore; break;
        case 14: case 15:
            kind = shRoom::kArmorStore; break;
        case 16: case 17:
            kind = shRoom::kWeaponStore; break;
        case 18:
            kind = shRoom::kImplantStore; break;
        case 19:
            kind = shRoom::kCanisterStore; break;
        }
    }

    bool infested = !RNG (25); /* Full of cockroaches. */

    /* Get us some stuff to sell. */
    for (int x = sx + 1; x < ex; x++) {
        for (int y = sy + 1; y < ey; y++) {
            shObject *obj = NULL;

            /* Stuff for sale depends on shop type. */
            switch (kind) {
            case shRoom::kGeneralStore:
                /* Anything except cash goes. */
                obj = gen_obj_type (prob::NotMoney); break;
            case shRoom::kHardwareStore:
                obj = gen_obj_type (prob::Tool); break;
            case shRoom::kSoftwareStore:
                if (RNG (8)) {
                    obj = gen_obj_type (prob::FloppyDisk);
                } else {
                    obj = gen_obj_type (prob::Computer);
                } break;
            case shRoom::kArmorStore:
                obj = gen_obj_type (prob::Armor); break;
            case shRoom::kWeaponStore:
                if (RNG (20)) {
                    /* Also generates ammo. */
                    obj = gen_obj_type (prob::Weapon);
                } else {
                    obj = gen_obj_type (prob::RayGun);
                } break;
            case shRoom::kImplantStore:
                obj = gen_obj_type (prob::Implant); break;
            case shRoom::kCanisterStore:
                obj = gen_obj_type (prob::Canister); break;
            }
            obj->set (obj::unpaid);
            putObject (obj, x, y);

            if (infested and /* Put one under every big enough item. */
                obj->myIlk ()->mSize > MonIlks[kMonGiantCockroach].mSize)
            {
                shCreature *cockroach = shCreature::monster (kMonGiantCockroach);
                putCreature (cockroach, x, y);
            }
        }
    }

    /* The shopkeeper droid. */
    shCreature *clerk = shCreature::monster (kMonClerkbot);
    putCreature (clerk, dx, dy);
    clerk->mShopKeeper.mHomeX = dx;
    clerk->mShopKeeper.mHomeY = dy;
    clerk->mShopKeeper.mShopId = roomid;
    clerk->mShopKeeper.mBill = 0;

    /* Shops may have a guard. */
    bool guarded = false;
    if (mDLevel > 4 and (!RNG (3) or mDLevel > 10) and !isTownLevel ()) {
        shCreature *guard = NULL;
        shMonId guardtype[3] = {kMonSecuritron, kMonWarbot, kMonGuardbot};
        guard = shCreature::monster (guardtype [RNG (3)]);
        if (guard) { /* Pick a nice warm spot in a corner. */
            if (gx == -1) {
                gx = (sx+1 == dx) ? ex-1 :
                     (ex-1 == dx) ? sx+1 :
                          RNG (2) ? sx+1 : ex-1;
            } else {
                gy = (sy+1 == dy) ? ey-1 :
                     (ey-1 == dy) ? sy+1 :
                          RNG (2) ? sy+1 : ey-1;
            }
            if (!putCreature (guard, gx, gy)) {
                guard->mStrategy = shCreature::kGuard;
                guard->mDisposition = shCreature::kIndifferent;
                guard->mGuard.mToll = -1;
                guarded = true;
            }
        }
    }

    mRooms[roomid].mType = (shRoom::Type) kind;
    mFlags |= kHasShop;
    debug.log ("made %s%sshop on level %d.",
        guarded ? "guarded " : "", infested ? "infested " : "", mDLevel);
    return 1;
}


int
shMapLevel::makeNest (int sx, int sy, int ex, int ey)
{
    int x, y;
    int roomid = mSquares[sx][sy].mRoomId;

    if (mDLevel > 12) {
        shCreature *queen = shCreature::monster (kMonAlienQueen);
        queen->mStrategy = shCreature::kLurk;
        putCreature (queen, RNG (sx + 1, ex - 1), RNG (sy + 1, ey -1));
    }

    debug.log ("made nest on level %d", mDLevel);

    for (x = sx + 1; x < ex ; x++) {
        for (y = sy + 1; y < ey; y++) {
            if (!isOccupied (x, y)) {
                shCreature *alien = NULL;
                if (x%4 and y%3) {
                    alien = shCreature::monster (kMonAlienEgg);
                } else if (!RNG (8) and mDLevel >= 8) {
                    alien = shCreature::monster (kMonAlienWarrior);
                    alien->mStrategy = shCreature::kLurk;
                }
                if (alien) {
                    putCreature (alien, x, y);
                }
            } /* No stairs in a nest! */
            //mSquares[x][y].mFlags &= ~shSquare::kStairsOK;
        }
    }

    mRooms[roomid].mType = shRoom::kNest;
    return 1;
}


shFeature *
shMapLevel::addTrap (int x, int y, shFeature::Type type)
{
    shFeature *trap = new shFeature ();
    trap->mX = x;
    trap->mY = y;
    if (type != shFeature::kHole and type != shFeature::kFloorGrating) {
        trap->mTrapUnknown = 1;
        trap->mTrapMonUnknown = 1;
    }
    bool retry;
    do {
        retry = false;
        switch (type) {
        case shFeature::kSewagePit:
            if (kSewage != getSquare (x, y) ->mTerr) {
                type = shFeature::kPit;
            }
            break;
        case shFeature::kPit:
        case shFeature::kAcidPit:
            switch (getSquare (x, y) ->mTerr) {
            case kSewage:
                type = shFeature::kSewagePit;
                break;
            default:
                break;
            }
            break;
        case shFeature::kFloorGrating:
        case shFeature::kHole:
        case shFeature::kTrapDoor:
            if (noDig () or isBottomLevel ()) {
                type = shFeature::kPit;
                retry = true;
            } else {
                trap->mDest.mLevel = 0;
                trap->mDest.mX = -1;
                trap->mDest.mY = -1;
            }
            break;
        default:
            break;
        }
    } while (retry);
    trap->mType = type;
    mFeatures.add (trap);
    return trap;
}


void
shMapLevel::addFeature (shFeature *f)
{
    mFeatures.add (f);
    /* Stairs have emergency lights that illuminate 3x3 area around them. */
    if (f->isStairs ())
        for (int x = maxi (f->mX - 1, 0);
             x <= mini (f->mX + 1, MAPMAXCOLUMNS);
             ++x)
        {
            for (int y = maxi (f->mY - 1, 0);
                 y <= mini (f->mY + 1, MAPMAXROWS);
                 ++y)
            {
                mSquares[x][y].mFlags |= shSquare::kLit;
            }
        }
}


shObjectIlk *
shFeature::keyNeededForDoor ()
{
    if (!isDoor ()) return NULL;
    if ((mDoor.mFlags & kLock1) == kLock1)
        return &AllIlks[kObjKeycard1];
    if ((mDoor.mFlags & kLock2) == kLock2)
        return &AllIlks[kObjKeycard2];
    if ((mDoor.mFlags & kLock3) == kLock3)
        return &AllIlks[kObjKeycard3];
    if ((mDoor.mFlags & kLock4) == kLock4)
        return &AllIlks[kObjKeycard4];
    if ((mDoor.mFlags & kLockMaster) == kLockMaster)
        return &AllIlks[kObjMasterKeycard];
    return NULL;
}
