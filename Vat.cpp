/********************************

      Quaffing from Vats

********************************/

#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"
#include "Interface.h"

/* Produces random vats for matter compiler. */
shFeature *
shFeature::newVat ()
{
    shFeature *vat = new shFeature ();
    vat->mType = shFeature::kVat;
    vat->mVat.mHealthy = RNG (-3, 3);
    vat->mVat.mRadioactive = RNG (10) > 0;
    vat->mVat.mAnalyzed = RNG (2);
    return vat;
}

/* Creates random vats for hero to find on map. */
void
shMapLevel::addVat (int x, int y)
{
    shFeature *vat = shFeature::newVat ();
    vat->mX = x;
    vat->mY = y;
    if (vat->mVat.mRadioactive) {
        /* Ordinary radioactive vats on map usually are unhealthy. */
        if (RNG (7))  vat->mVat.mHealthy = 0;
    } else {
        /* Non-radioactive vats likely are good. */
        vat->mVat.mHealthy = RNG (-1, 3);
    }
    vat->mVat.mAnalyzed = 0;
    mFeatures.add (vat);
}

static shCreature *
summonVatSlime (int x, int y)
{
    shCreature *slime = shCreature::monster (kMonVatSlime);
    if (!Level->findNearbyUnoccupiedSquare (&x, &y) and
        !Level->putCreature (slime, x, y))
    {
        return slime;
    }
    delete slime;
    return NULL;
}

static shObject *
dropIntoAcidPit (shCreature *actor, shFeature *vat, shObject *obj)
{   /* Objects falling into acid pit may corrode. */
    if (obj->sufferDamage (kAttAcidPitBath, actor, vat->mX, vat->mY)) {
        if (actor)
            actor->usedUpItem (obj, obj->mCount, "dissolve");
        /* Give no message.  Would be annoying when dropping multiple
           items and the effect is obvious upon inspection. */
        delete obj;
        return NULL;
    }
    return obj;
}

static shObject *
dropIntoVat (shCreature *actor, shFeature *vat, shObject *obj)
{
    /* FIXME: Right now there is no way to pass hero's creature pointer
       here so it is assumed hero is doing the dropping. */
    if (!actor)  actor = Hero.cr ();

    if (!obj->isA (kImplant))
        return obj;
    if (NOT0 (actor,->isInShop ()))
        actor->dropIntoOwnedVat (obj);

    bool seen = actor and actor->canSee (vat->mX, vat->mY);
    bool present = actor and actor->mX == vat->mX and actor->mY == vat->mY;
    switch (obj->mIlkId) {
    case kObjRadiationProcessor:
        if (vat->mVat.mRadioactive) {
            if (seen or present)
                I->p ("%s overloads and explodes!", THE (obj));
            Level->attackEffect (kAttConcussionGrenade, obj, vat->mX, vat->mY,
                                 kOrigin, actor, 0, -1);
            --vat->mVat.mRadioactive;
            if (seen or present)
                obj->set (obj::known_type);
            delete obj;
            return NULL;
        }
        break;
    case kObjHealthMonitor:
        obj->setBuggy ();
        break;
    case kObjPoisonResistor:
        if (seen or present) {
            I->p ("The sludge boils upon contact with %s!", YOUR (obj));
            obj->set (obj::known_type);
        }
        break;
    case kObjReflexCoordinator:
    case kObjMotoricCoordinator:
        if (seen) {
            I->p ("The sludge squirms frantically.");
        } else if (present) {
            I->p ("The sludge seems to be moving.");
        }
        if (seen or present)
            obj->maybeName ();
        break;
    case kObjCerebralCoprocessor:
        if (obj->mEnhancement > 0) {
            shCreature *slime = summonVatSlime (vat->mX, vat->mY);
            if (!slime) {
                if (present) {
                    I->p ("The sludge gurgles!");
                    obj->set (obj::known_type);
                }
                break;
            }
            slime->addObjectToInventory (obj);
            if (seen or present) {
                I->p ("It's alive!");
                obj->set (obj::known_type);
            }
            if (!RNG (3))
                Level->removeFeature (vat);
            return NULL;
        }
        break;
    case kObjPsiAmp:
        if (present and
            (obj->mEnhancement > 2 or (Hero.isPsyker () and obj->mEnhancement)))
        {
            I->p ("You sense a psychic disturbance.");
            obj->set (obj::known_type);
            if (Hero.isPsyker ())
                obj->set (obj::known_enhancement);
        }
        break;
    case kObjAdrenalineGenerator:
        if (seen and present) {
            I->p ("The sludge looks dangerous.");
            obj->set (obj::known_type);
        }
        break;
    case kObjNarcoleptor:
        if (seen) {
            I->p ("Whoa!  Trippy swirls!");
            obj->set (obj::known_type);
            Level->attackEffect (kAttHypnoDiskO, obj, vat->mX, vat->mY,
                                 kOrigin, actor, 0, -1);
        }
        break;
    case kObjTissueRegenerator:
        if (seen or present) {
            I->p ("The sludge grows %smoss.", seen ? "purple " : "");
            obj->set (obj::known_type);
        }
        break;
    case kObjBabelFish:
        if (present) {
            I->p ("Bye bye Willy, be free.");
            delete obj;
            if (actor)
                actor->beatChallenge (12);
            return NULL;
        }
        break;
    default: break;
    }
    return obj;
}

/* Usually returns the passed object.  Returns NULL if it has been destroyed
   or newly creatred object dropped item was transformed. */
shObject *
shFeature::dropObject (shCreature *actor, shObject *obj)
{
    assert (obj);
    if (mType == kStairsDown) {
        /* Object falls down. */
        shMapLevel *next = Maze.get (mDest.mLevel);
        /* If no level or no connected stairs cancel. */
        if (!next or mDest.mX == -1)  return obj;
        if (Hero.cr ()->canSee (mX, mY))
            I->p ("%s fall%s down.", THE (obj), obj->mCount > 1 ? "" : "s");
        int newx = mDest.mX, newy = mDest.mY;
        /* Return code can be ignored.  If no adjacent square is free
           just put the stuff directly on the stairs.  The displace is
           meant to avoid obscuring important base feature. */
        next->findAdjacentUnoccupiedSquare (&newx, &newy);
        next->putObject (obj, newx, newy);
        return NULL; /* Object is no longer on this level. */
    } else if (mType == kAcidPit) {
        return dropIntoAcidPit (actor, this, obj);
    } else if (mType == kVat) {
        return dropIntoVat (actor, this, obj);
    }
    return obj;
}

/* Returns true if vat dried up. */
bool
shFeature::quaffFromVat (shCreature *actor)
{
    if (actor->isInShop ())  actor->quaffFromOwnedVat (this);

    /* Usually you need to pour some nasty stuff in to achieve this. */
    if (mVat.mHealthy <= -4) {
        I->p ("Arrgghh!!  You vomit.");
        if (actor->sufferDamage (kAttVatPlague))
            actor->die (kKilled, "a filthy sludge vat");
        mVat.mAnalyzed = 1;
        return false;
    }

    enum Outcome {
        kIrradiate,
        kPurify,
        kGainAbility,
        kGainPower,
        kGoodStuff,
        kRandomStuff,
        kBadStuff,
        kVenom
    };

    /* Set up chances for drinking outcome. */
    shVector<int> chances;
    if (mVat.mRadioactive) {
        /* 10% irradiate, 5% purify, 15% mutant power */
        for (int i = 0; i < 2; ++i)
            chances.add (kIrradiate);
        chances.add (kPurify);
        for (int i = 0; i < 3; ++i)
            chances.add (kGainPower);
    } else {
        /* 25% purify, 5% mutant power */
        for (int i = 0; i < 5; ++i)
            chances.add (kPurify);
        chances.add (kGainPower);
    }
    /* Up to 15% gain ability, 30% good effect. */
    for (int i = mini (3, mVat.mHealthy); i > 0; --i) {
        chances.add (kGainAbility);
        chances.add (kGoodStuff);
        chances.add (kGoodStuff);
    }
    /* Up to 15% venom, 30% bad effect. */
    for (int i = maxi (-3, mVat.mHealthy); i < 0; ++i) {
        chances.add (kVenom);
        chances.add (kBadStuff);
        chances.add (kBadStuff);
    }

    /* Fill the rest with random effects. */
    while (chances.count () < 20)
        chances.add (kRandomStuff);

    int dryup = 3; /* One in three chance to dry up afterwards. */

    /* Choose outcome. */
    Outcome outcome = Outcome (chances.get (RNG (20)));
    if (outcome == kRandomStuff)
        outcome = RNG (2) ? kGoodStuff : kBadStuff;

    /* See what it does. */
    switch (outcome) {
    case kRandomStuff: /* Already handled above. */
        break;
    case kIrradiate:
        actor->mRad += NDX (5, 25);
        I->p ("Ick!  That had a toxic taste!");
        break;
    case kPurify:
        actor->mRad -= NDX (4, 50);
        actor->mRad = maxi (0, actor->mRad);
        if (!actor->mRad)
            I->p ("You feel purified!");
        else
            I->p ("You feel less contaminated.");
        break;
    case kGainAbility:
        actor->gainAbility (false, 1);
        mVat.mAnalyzed = 1;
        dryup = 2;
        break;
    case kGainPower:
        actor->getMutantPower ();
        dryup = 2;
        break;
    case kGoodStuff:
        switch (RNG (4)) {
        case 0:
            actor->mHP += NDX (4, 6);
            actor->mHP = mini (actor->mHP, actor->mMaxHP);
            I->p ("You feel better!");
            break;
        case 1:
        {
            actor->mAbil.temp_mod (abil::Psi, RNG (1, 6));
            I->p ("You feel invigorated!");
            break;
        }
        case 2:
            I->p ("Mmmm... bouncy bubbly beverage!");
            if (actor->mImpregnation) {
                actor->abortion (1); /* Here message given slightly differs. */
                I->p ("You feel the alien embryo inside you die.");
            }
            break;
        case 3:
            I->p ("Mmmm... hot fun!");
            if (actor->needsRestoration ())
                actor->restoration (1 + mini (3, mVat.mHealthy));
            break;
        }
        break;
    case kBadStuff:
        switch (RNG (3)) {
        case 0:
            I->p ("Oops!  You fall in!  You are covered in slime!");
            actor->damageEquipment (kAttVatCorrosion, kCorrosive);
            I->p ("You climb out of the vat.");
            break;
        case 1:
        {
            shCreature *slime = summonVatSlime (actor->mX, actor->mY);
            slime
                ? I->p ("It's alive!")
                : I->p ("The sludge gurgles!");
            break;
        }
        case 2:
            I->p ("You are jolted by an electric shock!");
            if (actor->sufferDamage (kAttVatShock))
                actor->die (kKilled, "an improperly grounded sludge vat");
            break;
        }
        break;
    case kVenom:
        switch (RNG (3)) {
        case 0:
            I->p ("This stuff is poisonous!");
            actor->abortion ();
            if (actor->sufferDamage (kAttVatPoison)) {
                actor->die (kKilled, "drinking some unhealthy sludge");
            }
            break;
        case 1:
        case 2:
            I->p ("This stuff is filthy!  (-1 max HP)");
            --actor->mMaxHP;
            if (actor->mHP > actor->mMaxHP)
                --actor->mHP;
            if (actor->mMaxHP == 0) {
                actor->die (kKilled, "drinking some unhealthy sludge");
            }
            break;
        }
        mVat.mAnalyzed = 1;
        break;
    }

    return !RNG (dryup);
}

void
shCreature::quaffFromVat (shFeature *vat)
{
    assert (vat->mX == mX and vat->mY == mY);

    if (vat->quaffFromVat (this) and mState == kActing) {
        I->p ("The vat dries up!");
        Level->forgetFeature (vat->mX, vat->mY);
        Level->removeFeature (vat);
    }
}

/* Returns true if the hero has observed the origin of effect. */
bool
shFeature::teleportVat (shFeature *vat, shCreature *actor, int newX, int newY)
{
    if (!vat or vat->mType != kVat)
        return false;
    shCreature *h = Hero.cr ();
    bool observed = false;
    int oldX = vat->mX, oldY = vat->mY;
    int prevroom = Level->getRoomID (oldX, oldY);
    //Level->findSuitableStairSquare (&newX, &newY);
    if (!Level->getFeature (newX, newY)) {
        if (h->canSee (oldX, oldY)) {
            I->p ("The vat flickers and disappears!");
            observed = true;
        } else if (h->mX == oldX and h->mY == oldY) {
            I->p ("The vat seems to be gone.");
            observed = true;
        }
        Level->addVat (newX, newY);
        shFeature *newvat = Level->getFeature (newX, newY);
        newvat->mVat = vat->mVat; /* Transfer all vat properties. */
        Level->removeFeature (vat);
        if (h->canSee (newX, newY)) {
            I->p ("Suddenly a vat materializes out of nowhere and falls down!");
        } else if (h->mX == newX and h->mY == newY) {
            I->p ("You hear something big closing on you from above ...");
        }
        /* See if the vat falls on someone. */
        shCreature *c = Level->getCreature (newX, newY);
        if (c) {
            if (c->sufferDamage (kAttFallingVat)) {
                if (h->canSee (c) or c == h) {
                    I->p ("It crushes %s into a pulp!", AN (c));
                    I->pauseXY (c->mX, c->mY);
                } else if (h->canHearThoughts (c)) {
                    I->p ("The mind of %s emanates pure terror for an instant.", AN (c));
                    h->checkConcentration (); /* Contact with terrified mind. */
                    I->p ("Then thoughts of %s abruptly vanish.", THE (c));
                    I->pauseXY (c->mX, c->mY);
                }
                c->die (kSlain);
            } else if (h->canSee (c) or c == h) {
                I->p ("It lands right on %s!", AN (c));
                I->pauseXY (c->mX, c->mY);
                if (c->canSee (h)) {
                    c->newEnemy (h);
                }
            } else if (h->canHearThoughts (c)) {
                I->p ("The mind of %s emanates fear for a moment.", AN (c));
                I->pauseXY (c->mX, c->mY);
                if (c->intr (kTelepathy) and actor == h) {
                    I->p("Somehow %s blames you for this.", THE (c));
                    c->newEnemy (h);
                }
            }
        }
        /* Does the vat belong to a clerkbot? */
        if (Level->isInShop (oldX, oldY)) {
            int newroom = Level->getRoomID (newX, newY);
            prevroom == newroom
                ? actor->movedVat ()   /* Harmless furniture rearrange. */
                : actor->stolenVat (); /* Vat landed outside shop. */
        }
    } else {
        if (h->canSee (oldX, oldY)) {
            I->p ("The vat flickers for a moment.");
            observed = true;
        }
    }
    return observed;
}

/* Returns true if the hero has observed the origin of effect. */
bool
shFeature::teleportAcid (shFeature *acidpit, shCreature *actor, int newX, int newY)
{
    if (!acidpit or acidpit->mType != kAcidPit)
        return false;
    shCreature *h = Hero.cr ();
    bool observed = false;
    int oldX = acidpit->mX, oldY = acidpit->mY;
    //Level->findOpenSquare (&newX, &newY);
    if (!Level->getFeature (newX, newY)) {
        if (h->canSee (oldX, oldY) and !acidpit->mTrapUnknown) {
            I->p ("The acid flickers and disappears!");
            observed = true;
        } else if (h->mX == oldX and h->mY == oldY and h->mZ == -1 and
                   !acidpit->mTrapUnknown)
        {
            I->p ("The acid seems to be gone.");
            observed = true;
        }
        acidpit->mType = kPit;

        if (h->canSee (newX, newY)) {
            I->p ("Suddenly an acid waterfall pours out of nowhere!");
            observed = true;
        } else if (h->mX == newX and h->mY == newY) {
            I->p ("You hear rushing water!  Or some other liquid ...");
            observed = true;
        }
        /* See if the acid pours down on someone. */
        shCreature *c = Level->getCreature (newX, newY);
        if (c) {
            if (c->sufferDamage (kAttAcidWaterfall)) {
                if (h->canSee (c) or c == h) {
                    I->p ("%s %s completely dissolved!", THE (c), c->are ());
                    I->pauseXY (c->mX, c->mY);
                } else if (h->canHearThoughts (c)) {
                    I->p ("The mind of %s emanates great pain for an instant.", AN (c));
                    h->checkConcentration (); /* Contact with mind in especially painful agony. */
                    I->p ("Then thoughts of %s abruptly vanish.", THE (c));
                    I->pauseXY (c->mX, c->mY);
                }
                c->die (kSlain);
            } else if (h->canSee (c) or c == h) {
                I->p ("%s %s washed!", THE (c), c->are ());
                I->pauseXY (c->mX, c->mY);
                if (c->canSee (h)) {
                    c->newEnemy (h);
                }
            } else if (h->canHearThoughts (c)) {
                I->p ("The mind of %s emanates pain for a moment.", AN (c));
                I->pauseXY (c->mX, c->mY);
                if (c->intr (kTelepathy) and actor == h) {
                    I->p("Somehow %s blames you for this.", THE (c));
                    c->newEnemy (h);
                }
            }
        }
        /* Pour down on items. */
        Level->areaEffectObjects (&Attacks[kAttAcidWaterfall],
            NULL, newX, newY, actor);
        /* The acid may land in another pit. */
        shFeature *f = Level->getFeature (newX, newY);
        if (f and f->mType == kPit) {
            if (h->canSee (newX, newY))
                I->p ("A pit is filled.");
            f->mType = kAcidPit;
        }
    } else {
        if (h->canSee (oldX, oldY)) {
            I->p ("The acid flickers for a moment.");
            observed = true;
        }
    }
    return observed;
}
