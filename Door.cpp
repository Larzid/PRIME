#include "Map.h"
#include "MonsterIlk.h"
#include "Hero.h"

/* Prompts an automatic door to change its state if necessary.
   RETURNS: -1 - door has shut
             0 - no change
            +1 - door has opened */
static int
doorReaction (shFeature &door, shMapLevel *lvl)
{
    assert (door.isAutomaticDoor ());
    /* Count creatures within sensor zone. */
    int neighbors = 0;
    for (int x = door.mX - 1; x <= door.mX + 1; ++x)
        for (int y = door.mY - 1; y <= door.mY + 1; ++y) {
            if (!lvl->isInBounds (x, y))
                continue;

            if (lvl->isOccupied (x, y))
                ++neighbors;
        }

    bool is_blocked = lvl->getObjects (door.mX, door.mY) or
        lvl->getCreature (door.mX, door.mY);
    bool willOpen = door.isClosedDoor () and !door.isLockedDoor () and
            (neighbors xor door.isInvertedDoor ());
    if (willOpen) {
        door.mDoor.mMoveTime = Clock;
        door.mType = shFeature::kDoorOpen;
        return +1;
    }
    bool willClose = door.isOpenDoor () and !is_blocked and
            (!neighbors xor door.isInvertedDoor ());
    if (willClose) {
        door.mDoor.mMoveTime = Clock;
        door.mType = shFeature::kDoorClosed;
        return -1;
    }
    return 0;
}

/* MODIFIES: Opens or closes any automatic doors that may have been affected
             by a creature leaving or entering the square (x, y). */
void
shMapLevel::checkDoors (int x, int y)
{
    shCreature *hero = Hero.cr ();
    /* Counts number of doors heard moving.
       If it is positive they were opening, if negative they were shutting. */
    int hear = 0;
    for (int tx = x - 1; tx <= x + 1; ++tx)
        for (int ty = y - 1; ty <= y + 1; ++ty) {
            shFeature *f = getFeature (tx, ty);
            if (!f or !f->isAutomaticDoor ())  continue;
            int res = doorReaction (*f, this);
            if (res and hero and hero->intr (kBlind) and hero->isAdjacent (x, y)) {
                hear += res;
                /* Hearing does not give much information about the door. */
                mRemembered[tx][ty].mFeat = f->mType;
            }
        }
    const char *numtostr[] = {
        "zero",
        "one",
        "two",
        "three",
        "four",
        "five",
        "six",
        "seven",
        "eight"
    };
    if (hear) {
        int absnum = hear < 0 ? -hear : hear;
        const char *state = hear < 0 ? "shut" : "open";
        if (absnum == 1) {
            I->p ("You hear a door whoosh %s.", state);
        } else {
            I->p ("You hear %s doors whoosh %s.", numtostr[absnum], state);
        }
    }
}


void
shMapLevel::doorAlarm (shFeature *door)
{
    shFeature *f;
    shVector <shFeature *> flist;

    door->mDoor.mFlags &= ~shFeature::kAlarmed;

    alertMonsters (door->mX, door->mY, 50, Hero.cr ()->mX, Hero.cr ()->mY);

    /* Chance for not triggering the alarm at all. */
    if (!RNG (7))  return;

    for (int i = 0; i < mFeatures.count (); ++i) {
        f = mFeatures.get (i);
        if (f->isStairs ())  flist.add (f);
    }

    if (!flist.count ())
        return; /* Impossible in normal game. */

    f = flist.get (RNG (flist.count ()));
    int x = f->mX, y = f->mY;

    switch (RNG (mDLevel / 8, mDLevel / 3)) {
    case 0:
        if (RNG (2)) {
            I->p ("An away team has been sent to investigate!");
            spawnGroup (x, y, NDX (2, 2), kMonRedshirt);
        } else {
            I->p ("A small cylon patrol is coming to investigate!");
            spawnGroup (x, y, NDX (2, 2), kMonCylonCenturion);
        }
        break;
    case 1:
        if (RNG (2)) {
            I->p ("An imperial security detail has been dispatched to investigate!");
            spawnGroup (x, y, NDX (2, 2), kMonStormtrooper);
            spawnGroup (x, y, 1, kMonStormtrooperCommander);
        } else {
            I->p ("A large cylon patrol is coming to investigate!");
            spawnGroup (x, y, RNG (2, 4), kMonCylonCenturion);
            spawnGroup (x, y, 2, kMonCylonCommandCenturion);
        }
        break;
    case 2:
        if (RNG (2)) {
            I->p ("A team of troubleshooters has been sent in to investigate!");
            spawnGroup (x, y, NDX (2, 3), kMonTroubleshooter);
        } else {
            I->p ("An honor guard is coming to investigate!");
            spawnGroup (x, y, RNG (3, 5), kMonKlingon);
        }
        break;
    case 3:
    default:
        if (RNG (2)) {
            I->p ("A squad of securitrons is coming to investigate!");
            spawnGroup (x, y, 3, kMonSecuritron);
        } else {
            I->p ("A securitron has been sent to investigate!");
            spawnGroup (x, y, 1, kMonSecuritron);
        }
        break;
    }

#if 0
    int tries = 50, x, y;
    bool done = false;
    while (!done and tries--) {
        x = f->mX;
        y = f->mY;
        if (0 == Level->findAdjacentUnoccupiedSquare (&x, &y)) {
            spawnTeam (x, y, &Sent);
            done = true;
            /* TODO: This is not enough.  One should also set right tactics. */
            shCreature *m = shCreature::monster (ilk);
            m->mDestX = door->mX;
            m->mDestY = door->mY;
            if (Level->putCreature (m, x, y)) {
                cnt++;
            }
        }
    }
#endif
}

/* Melting a door to dark room can create strange sight artifacts.
   Clean up non-full darkness flags. */
void
shMapLevel::doorRemoved(shFeature *door)
{
    int x = door->mX, y = door->mY;
    int state = mSquares[x][y].mFlags & shSquare::kDark;
    if (state != shSquare::kDark)
        mSquares[x][y].mFlags &= ~shSquare::kDark;
}

/* When a door is created as a part of dark room ensure it is not lit
   when looking from inside the room. */
void
shMapLevel::doorCreated(shFeature *door)
{
    int x = door->mX, y = door->mY;
    if (isInBounds (x-1, y-1) and (mSquares[x-1][y-1].mFlags & shSquare::kDark) == shSquare::kDark)
        mSquares[x][y].mFlags |= shSquare::kDarkNW;
    if (isInBounds (x+1, y-1) and (mSquares[x+1][y-1].mFlags & shSquare::kDark) == shSquare::kDark)
        mSquares[x][y].mFlags |= shSquare::kDarkNE;
    if (isInBounds (x-1, y+1) and (mSquares[x-1][y+1].mFlags & shSquare::kDark) == shSquare::kDark)
        mSquares[x][y].mFlags |= shSquare::kDarkSE;
    if (isInBounds (x+1, y+1) and (mSquares[x+1][y+1].mFlags & shSquare::kDark) == shSquare::kDark)
        mSquares[x][y].mFlags |= shSquare::kDarkSE;
}

/* Purpose: generate new door for matter compiler to choose. */
shFeature *
shFeature::newDoor (void)
{
    shFeature *door = new shFeature ();
    door->mType = kDoorOpen;
    door->mDoor.mFlags = 0;
    /* -5000 is sufficiently in the past so that on turn 1
       motion trackers do not pick it up as moving. */
    door->mDoor.mMoveTime = -5000;

    if (RNG (2)) {
        if (RNG (25))  switch (RNG (4)) { /* Choose random lock. */
            case 0: door->mDoor.mFlags |= shFeature::kLock1; break;
            case 1: door->mDoor.mFlags |= shFeature::kLock2; break;
            case 2: door->mDoor.mFlags |= shFeature::kLock3; break;
            case 3: door->mDoor.mFlags |= shFeature::kLock4; break;
        } else { /* Rare master lock. */
            door->mDoor.mFlags |= shFeature::kLockMaster;
        }
    }
    if (!RNG (3)) {
        door->mDoor.mFlags |= shFeature::kAutomatic;
        if (!RNG (15)) {
            door->mDoor.mFlags |= shFeature::kBerserk;
        }
        if (!RNG (15)) {
            door->mDoor.mFlags |= shFeature::kInverted;
        }
    }
    if (!RNG (5)) {
        door->mDoor.mFlags |= shFeature::kAlarmed;
    }
    return door;
}

/* Purpose: create and add specified door to map. */
void
shMapLevel::addDoor (int x, int y, int horiz,
                     int open /* = -1 */,
                     int lock /* = -1 */,
                     int secret /* = -1 */,
                     int alarmed /* = -1 */,
                     int magsealed /* = -1*/,
                     int retina /* = 0 */)
{
    shFeature *door = new shFeature ();

    door->mX = x;
    door->mY = y;
    door->mDoor.mFlags = 0;
    door->mDoor.mMoveTime = -5000; /* See newDoor () for explanation. */

    if (!isFloor (x, y))
        mSquares[x][y].mTerr = kFloor;

    if (lock == -1)
        lock = !RNG (13);

    if (alarmed == -1 and lock and mDLevel > 2)
        alarmed = !RNG (mDLevel > 8 ? 3 : 13);
    else
        alarmed = 0;

    /* Never randomly generate secret doors.  They are boring. */
    if (secret == -1)
        secret = 0;

    if (secret)
        open = 0;
    if (open == -1)
        open = !RNG (4);

    if (magsealed == -1)
        magsealed = !RNG (22);

    int autom = !lock and RNG (3);

    if (open) {
        door->mType = shFeature::kDoorOpen;
        if (autom and !RNG (40)) {
            door->mDoor.mFlags |= shFeature::kAutomatic | shFeature::kInverted;
        }
    } else if (!secret) {
        door->mType = shFeature::kDoorClosed;
        if (autom) {
            door->mDoor.mFlags |= shFeature::kAutomatic;
            if (!RNG (50) and mDLevel > 1) {
                door->mDoor.mFlags |= shFeature::kBerserk;
                door->mTrapUnknown = 1;
                door->mTrapMonUnknown = 0;
            }
        }
    } else { /* hidden door */
        door->mType = horiz ? shFeature::kDoorHiddenHoriz
                            : shFeature::kDoorHiddenVert;
        door->mTrapUnknown = 1; /* Required for autowalk to ignore it. */
        door->mTrapMonUnknown = 0;
    }

    if (door->isClosedDoor () and lock) {
        door->mDoor.mFlags |= shFeature::kLocked;
        lock = 1;
    }

    if (retina) {
        door->mDoor.mFlags |= shFeature::kLockRetina;
    } else {
        switch (RNG (lock ? 4 : 5)) { /* what kind of locking mechanism */
        case 0: door->mDoor.mFlags |= shFeature::kLock1; break;
        case 1: door->mDoor.mFlags |= shFeature::kLock2; break;
        case 2: door->mDoor.mFlags |= shFeature::kLock3; break;
        case 3: door->mDoor.mFlags |= shFeature::kLock4; break;
        case 4: break;
        }
    }

    if (alarmed)
        door->mDoor.mFlags |= shFeature::kAlarmed;
    if (magsealed)
        door->mDoor.mFlags |= shFeature::kMagneticallySealed;
    if (horiz)
        door->mDoor.mFlags |= shFeature::kHorizontal;

    addFeature (door);
    doorCreated (door);
}
