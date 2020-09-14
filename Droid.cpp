#include "Global.h"
#include "Util.h"
#include "Monster.h"
#include "Hero.h"

/* returns ms elapsed */
int
shHero::displace (shCreature *c)
{
    int cx = c->mX;
    int cy = c->mY;
    int hx = cr ()->mX;
    int hy = cr ()->mY;
    int elapsed;

    if (Level->isObstacle (cx, cy)) {
        return HALFTURN;
    }

    Level->removeCreature (c);
    Level->moveCreature (cr (), cx, cy);

    elapsed = FULLTURN;

    if (Level->putCreature (c, hx, hy) < 0) {
        if (Level->findNearbyUnoccupiedSquare (&hx, &hy) < 0  or
            Level->putCreature (c, hx, hy) < 0)
        {
            //FIX: memory leak!
            abort ();
            return elapsed;
        }
    }
    Level->checkTraps (hx, hy);
    return elapsed;
}


/* relocate the creature to a square near the hero */
void
shCreature::followHeroToNewLevel ()
{
    int x = Hero.cr ()->mX;
    int y = Hero.cr ()->mY;

    mLevel->removeCreature (this);
    if (Level->findNearbyUnoccupiedSquare (&x, &y) < 0) {
        abort ();
        return;
    }
    Level->putCreature (this, x, y);
    Level->checkTraps (x, y);
}


/* try to drink canister of beer on the ground here.
   returns ms elapsed, or -1 if nothing happened.
*/
int
shCreature::drinkBeer ()
{
    shObject *obj = mLevel->removeObject (mX, mY, kObjBeer);
    if (obj) {
        mHP = mini (mMaxHP+5, mHP + RNG (2, 8));
        if (mHP > mMaxHP) {
            mMaxHP = mHP;
        }
        if (Hero.cr ()->canSee (this)) {
            obj->set (obj::known_appearance);
            I->p ("%s drinks %s.", your (), obj->an ());
            I->p ("%s looks refreshed.", your ());
            obj->maybeName ();
            delete obj;
            return 1000;
        }
    }
    return -1;
}



/* Called after the hero has travelled from the specified level and
   coordinates to see if adjacent pets follow her. */
void
shHero::checkForFollowers (shMapLevel *lvl, int sx, int sy)
{
    for (int x = sx - 1; x <= sx + 1; ++x) {
        for (int y = sy - 1; y <= sy + 1; ++y) {
            shCreature *c = lvl->getCreature (x, y);
            if (c and c->isPet ()) {
                c->followHeroToNewLevel ();
            }
        }
    }
}


void
shCreature::makePet ()
{
    if (isA (kMonClerkbot)) {
        /* nice try */
        return;
    }

    mTame = 1;
    mDisposition = kHelpful;
    mTactic = kReady;
    mStrategy = kPet;
    if (isA (kMonCylonCenturion)) { /* The iconic phrase must be there. */
        I->p ("\"By your command.\"");
    }
    Hero.mPets.add (this);
}


void
shCreature::findPetGoal ()
{
    int x, y, n, dist;
    shCreature *c;
    shObject *obj;
    int bestdist = 99;

    for (n = 5; n > 0; n--) {
        for (x = mX - n; x <= mX + n; x++) {
            for (y = mY - n; y <= mY + n; y++) {
                if (!mLevel->isInBounds (x, y)) {
                    continue;
                }
                c = mLevel->getCreature (x, y);
                if (c and !c->isHero () and !c->isPet () and c->isHostile () and
                    /* canSee (c) and */
                    (c->mCLevel <= mCLevel + 1 or feat (kExplodes)))
                {
                    dist = distance (mX, mY, x, y);
                    if (dist < bestdist) {
                        bestdist = dist;
                        mDestX = x;
                        mDestY = y;
                    }
                }
                obj = mLevel->findObject (x, y, kObjBeer);
                if (obj) {
                    dist = distance (mX, mY, x, y);
                    if (dist < bestdist) {
                        bestdist = dist;
                        mDestX = x;
                        mDestY = y;
                    }
                }
            }
        }
    }

    /* Nothing interesting, let's wander. */
    if (99 == bestdist and !RNG(6)) {
        shCreature *m = Hero.cr (); /* Master. */
        if (RNG (3)) {
            mDestX = m->mX + RNG (13) - 6;
            mDestY = m->mY + RNG (9) - 4;
            mLevel->findNearbyUnoccupiedSquare (&mDestX, &mDestY);
        } else {
            mDestX = m->mX;
            mDestY = m->mY;
        }
    }
}


//returns ms elapsed, -2 if the monster dies
int
shCreature::doPet ()
{
    int i, n;
    shCoord coord[9];
    int info[9];
    int best = -1;
    int score, bestscore = -9999;
    int flags = kLinedUp | kTrap;
    int res = -1;
    int dist;
    int scrubbot = isA (kMonScrubbot);

    int val_linedup;
    int val_adjacent;
    int val_dist;
    int val_track;
    int val_monst;

    shCreature *c;
    int elapsed;
    int heroX = Hero.cr ()->mX, heroY = Hero.cr ()->mY;

    /* first priority is always to drink any beer in our square! */
    res = drinkBeer ();
    if (-1 != res) return res;

    if (mHP > mMaxHP / 4) {
        flags |= kMonster;
    }

    if (canSee (Hero.cr ())) {
        findPetGoal ();

        val_linedup = -10;
        val_dist = -2;
        val_track = -5;
        val_adjacent = RNG (3) ? -5 : 10;
        val_monst = 10;
    } else {
        mDestX = Hero.cr ()->mX;
        mDestY = Hero.cr ()->mY;
        val_linedup = 0;
        val_dist = -1;
        val_track = -10;

        val_adjacent = 5;
        val_monst = 5;
    }

    if (feat (kSessile))
        val_monst += 50;

    if (scrubbot)  flags |= kFreeWreck;
    n = findSquares (flags, coord, info);
    for (i = 0; i < n; i++) {
        score = 0;
        if (info[i] & kTrap
            and RNG (30)) /* small chance that a pet will walk on a trap*/
        {
            score -= 30;
        }
        if (info[i] & kFreeWreck)  score += 50;
        if (info[i] & kLinedUp)  score += val_linedup;
        if (info[i] & kMonster) {
            c = mLevel->getCreature (coord[i].mX, coord[i].mY);
            if (c and !c->isHero () and !c->isPet () and c->isHostile () and
            /* Smart bombs and missiles should be ready to sacrifice
               themselves on any hostile target. isExplosive checks this. */
                (c->mCLevel <= mCLevel + 1 or feat (kExplodes)))
            {
                score += c->isHostile () ? val_monst : val_monst / 2;
            } else if (c != this) {
                score -= 30;
            }
        }
        if (coord[i].mX == mX and coord[i].mY == mY) {
            score -= 10;
        }
        if (areAdjacent (coord[i].mX, coord[i].mY, heroX, heroY))
            score += val_adjacent;
        dist = distance (coord[i].mX, coord[i].mY, mDestX, mDestY);
        if (dist > 50 and val_dist > 0) { dist = 50; }
        score += val_dist * dist;

        int ti;
        for (ti = 0 ; ti < TRACKLEN; ti++) {
            if (mTrack[ti].mX == coord[i].mX and mTrack[ti].mY == coord[i].mY) {
                score += val_track;
            }
        }

        if (score > bestscore) {
            bestscore = score;
            best = i;
        }
    }

    if (-1 == best) {
        /* nothing to do but stay where we are for now */
        return 500;
    }
    if (coord[best].mX == mX and coord[best].mY == mY) {
        if (info[best] & kFreeWreck) {
            shObjectVector *v = mLevel->getObjects (mX, mY);
            shObject *obj;
            for (int i = 0; i < v->count (); ++i) {
                obj = v->get (i);
                if (obj->isA (kObjWreck)) {
                    v->remove (obj);
                    if (0 == v->count ()) {
                        delete v;
                        mLevel->setObjects (mX, mY, NULL);
                    }
                    return utilizeWreck (obj);
                }
            }
        }
        return 250;
    } else {
        if (info[best] & kMonster) {
            c = mLevel->getCreature (coord[best].mX, coord[best].mY);
            doAttack (c, &elapsed);
            return elapsed;
        }
        if (!feat (kSessile))
            return doComputerMove (
                vectorDirection (mX, mY, coord[best].mX, coord[best].mY));
    }
    return 250;
}

/* returns ms elapsed */
int
shCreature::utilizeWreck (shObject *obj)
{
    see (fmt ("%s utilizes %s.", the (), AN (obj)));
    shObject *cells = new shObject (kObjEnergyCell);
    if (isPet ()) {
        mLevel->putObject (cells, mX, mY);
    } else {
        addObjectToInventory (cells);
    }
    return FULLTURN;
}
