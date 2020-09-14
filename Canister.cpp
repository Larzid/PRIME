#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"
#include "Transport.h"
#include "Scenario.h"
#include <ctype.h>

static void
wasteCannedEmbryo (shCreature *actor)
{   /* Give exp for killing alien. */
    actor->beatChallenge (MonIlks[kMonFacehugger].mBaseLevel);
    /* Count wasted embryos too. */
    ++MonIlks[kMonAlienEmbryo].mKills;
}

/* Pouring a canister from a stack into a vat should always */
static void   /* ask the hero to name only a singular item. */
pourMaybeName (shObject *cans)
{
    int cnt = cans->mCount;
    cans->mCount = 1;
    cans->maybeName ();
    cans->mCount = cnt;
}

/* Splashes objects on floor with canister contents. */
static void
pourCanisterOnObjects (shCreature *c, shObject *can, int x, int y)
{
    int seen = c->canSee (x, y);
    shObjectVector *v = Level->getObjects (x, y);
    if (v and can->myIlk ()->mMissileAttack) { /* Is explosive. */
        for (int i = v->count () - 1; i >= 0; --i) {
            shObject *obj = v->get (i);
            if (obj->sufferDamage (can->myIlk ()->mMissileAttack, c, x, y)) {
                if (seen) {
                    can->set (obj::known_type);
                } else {
                    pourMaybeName (can);
                }
                v->remove (obj);
                delete obj;
            }
        }
        if (!v->count ()) {
            delete v;
            v = NULL;
            Level->setObjects (x, y, NULL);
        }
        return;
    } /* Try for special interaction. */
    if (v) for (int i = v->count () - 1; i >= 0; --i) {
        shObject *obj = v->get (i);
        switch (can->mIlkId) {
        case kObjBrain:
            if (obj->isA (kObjBioComputer)) {
                if (seen) {
                    I->p ("%s devours the brain!", THE (obj));
                    can->set (obj::known_type);
                } else {
                    I->p ("You hear content munching sounds.");
                    pourMaybeName (can);
                }
                obj->mEnhancement = mini(+5, obj->mEnhancement + can->mBugginess + 1);
                return;
            } else if (obj->isA (kObjBioArmor)) {
                if (c->canSee (x, y)) {
                    I->p ("%s envelops the brain!", THE (obj));
                    can->set (obj::known_type);
                } else {
                    I->p ("You hear a slurping sound.");
                    pourMaybeName (can);
                }
                obj->mIlkId = kObjBioComputer;
                obj->mEnhancement = RNG (-3, +3) + can->mBugginess;
                return;
            }
            break;
        case kObjMutagenCanister:
            if (obj->isA (kObjBioArmor)) {
                int die = RNG (2);
                if (seen) {
                    I->p ("%s soaks up the liquid!", THE (obj));
                    if (die) {
                        I->p ("It develops a fatal mutation and dies.");
                    } else {
                        I->p ("It develops a harmful mutation.");
                    }
                    can->set (obj::known_type);
                }
                if (die) {
                    v->remove (obj);
                    delete obj;
                    if (!v->count ()) {
                        delete v;
                        v = NULL;
                        Level->setObjects (x, y, NULL);
                    }
                } else {
                    obj->mIlkId = kObjMBioArmor0;
                    obj->mEnhancement = maxi (-(obj->myIlk ()->mMaxEnhancement),
                        obj->mEnhancement - 1);
                }
                return;
            }
            break;
        case kObjGainAbilityCanister:
            if (!obj->isA (kObjBioComputer) and !obj->isA (kObjBioArmor)) {
                break;
            }
            if (seen) {
                I->p ("%s soaks up the liquid!", THE (obj));
            }
            if (obj->isA (kObjBioComputer)) {
                if (can->isBuggy ()) {
                    obj->mEnhancement = -5;
                    if (seen) {
                        I->p ("It loses its ability to process input.");
                        can->set (obj::known_type);
                    }
                } else {
                    if (seen and (obj->mEnhancement == 5 or
                        (obj->mEnhancement == 4 and !can->isOptimized ())))
                    {
                        I->p("Nothing else seems to happen.");
                        pourMaybeName (can);
                    } else {
                        if (seen) {
                            I->p ("Its abilities improve!");
                            can->set (obj::known_type);
                        }
                        ++obj->mEnhancement;
                    }
                }
            } else if (can->isBuggy ()) {
                if (obj->mIlkId == kObjBioArmor or
                    obj->isA (kObjMBioArmor0))
                {
                    if (obj->is (obj::known_enhancement) and seen) {
                        I->p ("It suffers.");
                        can->set (obj::known_type);
                    }
                    obj->mEnhancement = maxi (-(obj->myIlk ()->mMaxEnhancement),
                        obj->mEnhancement - 1);
                } else {
                    if (seen) {
                        I->p ("It regresses in development!");
                        can->set (obj::known_type);
                    }
                    obj->mIlkId = obj->myIlk ()->mParent;
                }
            } else {
                if (obj->mIlkId == kObjBioArmor) {
                    if (seen) {
                        I->p ("It gains a new ability!");
                        can->set (obj::known_type);
                    }
                    switch (RNG (3)) {
                    case 0: obj->mIlkId = kObjEBioArmor1; break;
                    case 1: obj->mIlkId = kObjEBioArmor2; break;
                    case 2: obj->mIlkId = kObjEBioArmor3; break;
                    }
                } else if (obj->mIlkId == kObjMBioArmor0) {
                    if (seen) {
                        I->p ("Its mutation develops into somewhat beneficial one.");
                        can->set (obj::known_type);
                    }
                    if (RNG (2)) {
                        obj->mIlkId = kObjMBioArmor1;
                    } else {
                        obj->mIlkId = kObjMBioArmor2;
                    }
                } else if (can->isOptimized () and
                       (obj->mIlkId == kObjEBioArmor1 or
                        obj->mIlkId == kObjEBioArmor2 or
                        obj->mIlkId == kObjEBioArmor3))
                {
                    if (seen) {
                        I->p ("It is further enhanced!");
                        can->set (obj::known_type);
                    }
                    switch (obj->mIlkId) {
                    case kObjEBioArmor1:
                        if (RNG (2)) {
                            obj->mIlkId = kObjSBioArmor1;
                        } else {
                            obj->mIlkId = kObjSBioArmor2;
                        }
                        break;
                    case kObjEBioArmor2:
                        obj->mIlkId = kObjSBioArmor3; break;
                    case kObjEBioArmor3:
                        obj->mIlkId = kObjSBioArmor4; break;
                    default: break;
                    }
                } else {
                    if (obj->is (obj::known_enhancement) and seen) {
                        I->p ("Looks like %s enjoyed it.", THE (obj));
                        can->set (obj::known_type);
                    }
                    obj->mEnhancement = mini (obj->myIlk ()->mMaxEnhancement,
                        obj->mEnhancement + 1);
                }
            }
            return;
        default: /* Has no special interaction. No point in iterating. */
            return;
        }
    } /* If it survived until this point it means the floor is contaminated. */
    if (can->isA (kObjMutagenCanister)) {
        Level->getSquare(x, y)->mFlags |= shSquare::kRadioactive;
    }
}

static int
pourCanisterAtFeature (shCreature *c, shObject *can, shFeature *f, int x, int y)
{
    f->mTrapUnknown = 0; /* Better reveal it. */
    switch (f->mType) {
    case shFeature::kAcidPit:
        switch (can->mIlkId) {
        case kObjAntimatter:
            if (Level->getLevelBelow ()) {
                if (!c->intr (kBlind)) {
                    I->p ("The acid is annihilated.");
                }
                f->mType = shFeature::kHole;
                Level->checkTraps (x, y);
            } else {
                f->mType = shFeature::kPit;
            }
            return FULLTURN;
        case kObjWater:
            f->mType = shFeature::kPit;
            if (!c->intr (kBlind)) {
                I->p ("The acid boils violently as it is neutralized.");
                can->set (obj::known_type);
            } else {
                I->p ("You hear boiling.");
                pourMaybeName (can);
            }
            return FULLTURN;
        default:
            if (!c->intr (kBlind)) {
                I->p ("The acid boils.");
            } else {
                I->p ("You hear boiling.");
            }
            pourMaybeName (can);
            return FULLTURN;
        }
    case shFeature::kPit:
        switch (can->mIlkId) {
        case kObjAntimatter:
            if (Level->getLevelBelow ()) {
                f->mType = shFeature::kHole;
                Level->checkTraps (x, y);
            }
            return FULLTURN;
        case kObjUniversalSolvent:
            f->mType = shFeature::kAcidPit;
            if (!c->intr (kBlind)) {
                I->p ("The pit is filled with acid!");
            } else {
                I->p ("You smell acidic odor.");
            }
            can->set (obj::known_type);
            return FULLTURN;
        default:
            I->p ("You pour out the contents into %s.", THE (f));
            pourCanisterOnObjects (c, can, x, y);
            pourMaybeName (can);
            return FULLTURN;
        }
    case shFeature::kHole:
        I->p ("You pour out the contents into %s.", THE (f));
        return FULLTURN;
    case shFeature::kRadTrap:
        switch (can->mIlkId) {
        case kObjAntimatter:
            if (!c->intr (kBlind)) {
                I->p ("The rad trap is annihilated!");
            }
            if (Level->getLevelBelow ()) {
                f->mType = shFeature::kHole;
            } else {
                f->mType = shFeature::kPit;
            }
            Level->checkTraps (x, y, 100);
            break;
        case kObjUniversalSolvent:
            if (!c->intr (kBlind)) {
                I->p ("The rad trap dissolves!");
            } else {
                I->p ("You smell acidic odor.");
            }
            Level->removeFeature (f);
            can->set (obj::known_type);
            break;
        default:
            I->p ("You pour out the contents into %s.", THE (f));
            pourMaybeName (can);
            break;
        }
        return FULLTURN;
    case shFeature::kVat:
        switch (can->mIlkId) {
        case kObjAntimatter:
            if (c->canSee (x, y)) {
                I->p ("The vat is annihilated!");
            }
            Level->removeFeature (f);
            can->set (obj::known_type);
            Level->addTrap (x, y, shFeature::kHole);
            if (c->isInShop ()) {
                c->damagedShop (x, y);
            }
            Level->checkTraps (x, y, 100);
            return FULLTURN;
        case kObjLNO:
            I->p ("The sludge freezes!");
            can->set (obj::known_type);
            return FULLTURN;
        case kObjNapalm: case kObjPlasmaCanister:
            if (!c->intr (kBlind)) {
                I->p ("The sludge boils!");
            } else {
                I->p ("You hear boiling.");
            }
            pourMaybeName (can);
            return FULLTURN;
        case kObjBeer: case kObjNanoCola: case kObjNukaCola: case kObjB3:
        case kObjCoffee:
            if (can->mIlkId == Scen.vat_bubbly_beverage) {
                const char *how_many;
                how_many = f->mVat.mHealthy >= +2 ? "many large"
                         : f->mVat.mHealthy == +1 ? "some large"
                         : f->mVat.mHealthy ==  0 ? "a few"
                         : f->mVat.mHealthy == -1 ? "some small"
                         :                          "many small";
                I->p ("%s bubbles rise from the vat.", how_many);
                f->mVat.mAnalyzed = 1;
            } else {
                I->p ("The sludge fizzes.");
            }
            pourMaybeName (can);
            return FULLTURN;
        case kObjSuperGlue:
            I->p ("The sludge thickens.");
            f->mVat.mHealthy -= 1;
            can->set (obj::known_type);
            return FULLTURN;
        case kObjUniversalSolvent: case kObjWater:
            I->p ("The sludge thins.");
            pourMaybeName (can);
            return FULLTURN;
        case kObjMutagenCanister:
            I->p ("The sludge changes color.");
            f->mVat.mHealthy -= 1;
            f->mVat.mRadioactive++;
            can->set (obj::known_type);
            return FULLTURN;
        case kObjPoisonCanister:
            I->p ("The sludge seems less nutritous now.");
            f->mVat.mHealthy -= 2;
            can->set (obj::known_type);
            return FULLTURN;
        case kObjFullHealingCanister:
            f->mVat.mHealthy += 2; /* Fall through. */
        case kObjHealingCanister:
            I->p ("The sludge seems more nutritious now.");
            f->mVat.mHealthy += 1;
            pourMaybeName (can);
            return FULLTURN;
        case kObjGainAbilityCanister:
            f->mVat.mHealthy += 2; /* Fall through. */
        case kObjRestorationCanister:
            I->p ("You've improved the sludge recipe.");
            f->mVat.mHealthy += 2;
            pourMaybeName (can);
            return FULLTURN;
        case kObjBrain:
            I->p ("You drain a brain.");
            f->mVat.mHealthy += 2;
            can->set (obj::known_type);
            return FULLTURN;
        case kObjRadAway:
            I->p ("The sludge seems purified.");
            f->mVat.mHealthy += 1;
            f->mVat.mRadioactive = 0;
            can->set (obj::known_type);
            return FULLTURN;
        case kObjSpeedCanister:
            I->p ("The sludge is churning more rapidly now.");
            can->set (obj::known_type);
            return FULLTURN;
        case kObjSpiceMelange:
        {
            int x, y;
            if (can->isBuggy ()) {
                trn::coord_in_stripe (x, y, 1, 4, &shMapLevel::is_free_sq);
            } else {
                Level->findOpenSquare (&x, &y);
            }
            shFeature::teleportVat (f, c, x, y);
            f->mVat.mHealthy += 1;
            can->set (obj::known_type);
            return FULLTURN;
        }
        case kObjCannedEmbryo: {
            if (!c->intr (kBlind)) {
                I->p ("A larva falls out and drowns in the sludge!");
                can->set (obj::known_type);
            }
            f->mVat.mHealthy = -4; /* Very bad thing to do. */
            wasteCannedEmbryo (c);
            if (c->intr (kBlind)) {
                I->p ("You feel more experienced.");
                pourMaybeName (can);
            }
            return FULLTURN;
        }
        default:
            can->set (obj::known_type); /* This might help nail down any bugs. */
            I->p ("%s does something beyond description!", YOUR (can));
            I->p ("You almost breach space-time continuity by triggering unimplemented effect.");
            I->p ("You'd better report this bug and how you caused it.");
            return FULLTURN;
        }
    case shFeature::kPortableHole:
        switch (can->mIlkId) {
        case kObjAntimatter:
            if (!c->intr (kBlind)) {
                I->p ("The portable hole is annihilated along with %s!",
                    THE (Level->getSquare (x, y)));
            }
            Level->removeFeature (f);
            Level->dig (x, y);
            break;
        case kObjUniversalSolvent:
            if (!c->intr (kBlind)) {
                I->p ("The portable hole dissolves!");
            } else {
                I->p ("You smell acidic odor.");
            }
            Level->removeFeature (f);
            can->set (obj::known_type);
            break;
        default:
            I->p ("You pour out the contents onto %s.", THE (Level->getSquare (x, y)));
        }
        return FULLTURN;
    case shFeature::kDoorClosed:
        if (f->isMagneticallySealed ()) {
            if (can->mIlkId == kObjAntimatter) {
                I->p ("Boom!");
                Level->attackEffect (kAttConcussionGrenade, NULL, x, y, kOrigin, c);
                can->set (obj::known_type);
            } else if (can->mIlkId == kObjCannedEmbryo) {
                if (!c->intr (kBlind))
                    I->p ("Something falls out and is fried by the force field!");
                else
                    I->p ("Kzapp!  You smell fried meat.");
                wasteCannedEmbryo (c);
                can->set (obj::known_type);
            } else {
                if (!c->intr (kBlind))
                    I->p ("The force field neutralizes the liquid.");
            }
            return FULLTURN;
        }
        /* Fall through. */
    case shFeature::kDoorOpen:
        switch (can->mIlkId) {
        case kObjAntimatter:
            if (!c->intr (kBlind))
                I->p ("%s is annihilated!", THE (f));
            Level->removeFeature (f);
            return FULLTURN;
        case kObjUniversalSolvent:
            if (!c->intr (kBlind)) {
                I->p ("%s dissolves!", THE (f));
            } else {
                I->p ("You smell acidic odor.");
            }
            Level->removeFeature (f);
            can->set (obj::known_type);
            return FULLTURN;
        case kObjNapalm:
            if (!c->intr (kBlind)) {
                I->p ("%s burns up!", THE (f));
            } else {
                I->p ("You smell smoke.");
            }
            Level->removeFeature (f);
            can->set (obj::known_type);
            return FULLTURN;
        default:
            break; /* Fall through to default. */
        }
    default:
        I->p ("You pour out the contents onto %s.", THE (f));
        if (can->myIlk ()->mMissileAttack) {
            Level->areaEffectFeature (&Attacks[can->myIlk ()->mMissileAttack],
                NULL, x, y, kDown, c);
        }
        pourCanisterOnObjects (c, can, x, y);
        pourMaybeName (can);
        return FULLTURN;
    }

}

static int useUniversalSolvent (shCreature *c, shObject *can);

static int
useCommonCanister (shCreature *user, shObject *can)
{
    shDirection where = I->getDirection ();
    if (kNoDirection == where) {
        return 0;
    }
    switch (where) {
    case kDown: {
        shFeature *f = Level->getFeature (user->mX, user->mY);
        if (f)
            return pourCanisterAtFeature (user, can, f, user->mX, user->mY);
        user->msg (fmt ("You pour out the contents onto %s.",
                THE (Level->getSquare (user->mX, user->mY))));
        pourCanisterOnObjects (user, can, user->mX, user->mY);
        pourMaybeName (can);
        return FULLTURN;
    }
    case kOrigin:
        if (can->isA (kObjUniversalSolvent))
            return useUniversalSolvent (user, can);
        /* Otherwise fall through. */
    case kUp:
        if (can->myIlk ()->mMissileAttack) {
            can->set (obj::known_type);
            switch (can->mIlkId) {
            case kObjAntimatter: I->p ("You spill antimatter."); break;
            case kObjNapalm: I->p ("It ignites!"); break;
            case kObjLNO: I->p ("It freezes!"); break;
            case kObjPlasmaCanister: I->p ("It shocks you!"); break;
            case kObjUniversalSolvent: I->p ("It is acidic!"); break;
            case kObjSuperGlue: I->p ("This is sticky."); break;
            default: I->p ("You soak yourself."); break;
            }
            if (user->sufferDamage (can->myIlk ()->mMissileAttack)) {
                char *buf = GetBuf ();
                sprintf (buf, "spilling %s %s", can->an (1),
                    where == kUp ? "upwards" : "at self");
                user->die (kKilled, buf);
            }
        }
        return FULLTURN;
    default: {
        int x = user->mX, y = user->mY;
        if (!Level->moveForward (where, &x, &y)) {
            I->p ("You stick the canister outside the map and pour it out.");
            I->p ("Its contents are harmlessly deallocated.");
            return FULLTURN;
        }
        shCreature *c = Level->getCreature (x, y);
        if (!c) {
            shFeature *f = Level->getFeature (x, y);
            if (f) {
                return pourCanisterAtFeature (user, can, f, x, y);
            } else {
                I->p ("You pour out the contents onto %s.",
                    THE (Level->getSquare (x, y)));
            }
            if (can->isA (kObjAntimatter) and !Level->isFloor (x, y)) {
                I->p ("%s is annihilated!", THE (Level->getSquare (x, y)));
                Level->dig (x, y);
                return FULLTURN;
            }
            if (can->myIlk ()->mMissileAttack) {
                user->mLevel->areaEffectFeature (
                    &Attacks[can->myIlk ()->mMissileAttack],
                    NULL, x, y, kDown, user);
            }
            pourCanisterOnObjects (user, can, x, y);
            return FULLTURN;
        }
        I->p ("You splash the contents of canister at %s.", THE (c));
        /* Instakill because player expends whole canister. Yes, even Shodan. */
        if (can->isA (kObjAntimatter)) {
            I->p ("%s is annihilated!", THE (c));
            c->die (kAnnihilated);
            return FULLTURN;
        }
        if (can->myIlk ()->mMissileAttack) {
            switch (can->mIlkId) { /* TODO: Change to 3rd person. */
            case kObjNapalm: I->p ("You burn %s!", THE (c)); break;
            case kObjLNO: I->p ("You freeze %s!", THE (c)); break;
            case kObjPlasmaCanister: I->p ("You shock %s!", THE (c)); break;
            case kObjUniversalSolvent: I->p ("You dissolve %s!", THE (c)); break;
            case kObjSuperGlue: I->p ("You coat %s with glue.", THE (c)); break;
            default:
                I->p ("You do something evil and buggy to %s.", THE (c)); break;
            }
            if (c->sufferDamage (can->myIlk ()->mMissileAttack)) {
                c->die (kSlain);
            }
            can->set (obj::known_type);
        } else {
            I->p ("You soak %s.", THE (c));
            pourMaybeName (can);
        }
        return FULLTURN;
    }
    }
    return FULLTURN; /* Not reached. */
}

static int
quaffAntimatter (shCreature *c, shObject *can)
{
    c->msg ("You drink antimatter.");
    can->set (obj::known_type);
    c->die (kAnnihilated, "drinking a canister of antimatter");
    return FULLTURN;
}

static int
quaffBeer (shCreature *c, shObject *can)
{
    can->set (obj::known_type);
    if (c->is (kFrightened)) {
        c->cure (kFrightened);
        c->msg ("Liquid courage!");
    } else {
        c->msg ("Mmmmm... beer!");
    }
    c->inflict (kConfused, NDX (2, 50) * FULLTURN);
    c->mAbil.temp_mod (abil::Int, -RNG (2, 3));
    return FULLTURN;
}

static int
quaffNanoCola (shCreature *c, shObject *can)
{
    can->set (obj::known_type);
    c->mAbil.temp_mod (abil::Psi, RNG (2+can->mBugginess, 6));
    c->msg ("You feel invigorated!");
    return FULLTURN;
}

static int
quaffNukaCola (shCreature *c, shObject *can)
{
    can->set (obj::known_type);
    c->mRad += RNG (1, 10);
    c->mAbil.temp_mod (abil::Psi, RNG (2+can->mBugginess, 6));
    c->msg ("You feel refreshed!");
    return FULLTURN;
}

static int
quaffB3 (shCreature *c, shObject *can)
{
    can->set (obj::known_type);
    c->msg ("You feel happier!");
    c->mRad += RNG (1, 10);
    if (RNG (2) and c->mHP >= 2) {
        --c->mHP;
        if (c->isHero ())  I->drawSideWin (c);
    }
    if (!RNG (3))  c->abortion ();

    if (!RNG (40 + can->mBugginess * 10)) {
        c->msg ("Your health suffers.");
        if (c->sufferAbilityDamage (abil::Con, 1)) {
            c->die (kKilled, "drinking especially foul BBB canister");
        }
        if (c->isHero ())  I->drawSideWin (c);
    }
    if (!RNG (100))  c->getMutantPower ();

    c->mAbil.temp_mod (abil::Psi, RNG (1, 5 + can->mBugginess));
    return FULLTURN;
}

static int
quaffCoffee (shCreature *c, shObject *can)
{
    if (!can->is (obj::known_type)) {
        c->msg ("As you open the canister it heats up and you smell ... coffee?");
    } else {
        c->msg (fmt ("You drink %s coffee.", can->isBuggy () ? "lukewarm" : "hot"));
    }
    if (!c->getTimeOut (CAFFEINE)) {
        c->mInnateResistances[kMesmerizing] += 25 + 5 * can->mBugginess;
    } /* Values for timeouts are those used by disk of hypnosis in reverse. */
    if (can->isBuggy ()) {
        c->setTimeOut (CAFFEINE, RNG (5, 25) * FULLTURN, 0);
    } else if (can->isDebugged ()) {
        c->setTimeOut (CAFFEINE, RNG (6, 40) * FULLTURN, 0);
    } else {
        c->setTimeOut (CAFFEINE, RNG (12, 80) * FULLTURN, 0);
    }
    can->set (obj::known_type);
    return FULLTURN;
}

static int
quaffSuperGlue (shCreature *c, shObject *can)
{
    can->set (obj::known_type);
    c->msg ("The canister sticks to your tongue!");
    c->msg ("You look ridiculous!");
    Hero.setStoryFlag ("superglued tongue", 1);
    return FULLTURN;
}

static int
quaffUniversalSolvent (shCreature *c, shObject *can)
{
    can->set (obj::known_type);
    c->msg ("This burns!");
    if (c->sufferDamage (kAttUniversalSolvent))
        c->die (kKilled, "drinking a canister of universal solvent");

    c->abortion ();
    return FULLTURN;
}

static int
useUniversalSolvent (shCreature *c, shObject *can)
{
    int solved = 0;
    can->set (obj::known_type);
    if (c->isHero () and Hero.getStoryFlag ("superglued tongue")) {
        c->msg ("You dissolve the super glue.");
        Hero.resetStoryFlag ("superglued tongue");
        ++solved;
    }
    if (c->mWeapon and c->mWeapon->isBuggy () and
        c->mWeapon->is (obj::known_bugginess))
    {
        c->msg (fmt ("You unweld %s.", YOUR (c->mWeapon)));
        c->mWeapon->setDebugged ();
        if (c->mWeapon->sufferDamage (kAttUniversalSolvent, c, c->mX, c->mY)) {
            c->usedUpItem (c->mWeapon, 1, "dissolve");
            delete c->removeOneObjectFromInventory (c->mWeapon);
        }
        ++solved;
    }
    if (!solved) {
        c->msg ("You dissolve some of your flesh.  Ouch!");
        if (c->sufferDamage (kAttUniversalSolvent))
            c->die (kKilled, "bathing in universal solvent");
    }
    return FULLTURN;
}

static int
quaffMutagen (shCreature *c, shObject *can)
{
    can->set (obj::known_type);
    c->msg ("Ick!  That must have been toxic!");
    c->getMutantPower ();
    c->mRad += 125 + RNG (101);
    return FULLTURN;
}

static int
quaffWater (shCreature *c, shObject *can)
{
    can->set (obj::known_type);
    c->msg ("This tastes like water.");
    return FULLTURN;
}

/* This routine and fullHealing return number of problems helped with. */
int
shCreature::healing (int hp, int hpmaxdice)
{
    if (hp == 0)  return 0;
    int id = 0;
    if (isHero () and mHP < mMaxHP) {
        int heal = mini (hp, mMaxHP - mHP);
        I->p ("Your wounds are rapidly healing!  (%+d HP)", heal);
        id++;
    }
    if (isHero () and Hero.getStoryFlag ("brain incision")) {
        Hero.resetStoryFlag ("brain incision");
        I->p ("Your head wound is closed.");
        id++;
    }
    mHP += hp;
    if (mHP > mMaxHP and hpmaxdice) {
        int bonus = NDX (hpmaxdice, 3);
        mMaxHP += bonus;
        mHP = mMaxHP;
        if (isHero ()) {
            I->p ("You feel much healthier.  (%+d max HP)", bonus);
            id++;
        }
    }
    if (isHero () and is (kPlagued)) {
        I->p ("You have a feeling this will delay the plague somewhat.");
        /* Seemingly toggle it off. */
        mConditions &= ~kPlagued;
        /* Push next attack into future. */
        setTimeOut (PLAGUED, 40 * FULLTURN, 0);
        ++id;
    }
    mHP = mini (mHP, mMaxHP);
    if (is (kSickened) and !sewerSmells ()) {
        cure (kSickened);
        ++id;
    }
    static const shCondition remedy[] = {kViolated, kConfused, kStunned};
    const int size = sizeof (remedy) / sizeof (shCondition);
    for (int i = 0; i < size; ++i) {
        if (is (remedy[i])) {
            cure (remedy[i]);
            ++id;
        }
    }
    checkTimeOuts ();
    return id;
}

int
shCreature::fullHealing (int hp, int hpmaxdice)
{
    if (hp == 0)  return 0;
    int id = 0;
    if (isHero () and mHP < mMaxHP) {
        int heal = mini (hp, mMaxHP - mHP);
        msg (fmt ("Your wounds are fully healed!  (%+d HP)", heal));
        id++;
    }
    /* TODO: FIXME!  Monsters also need this. */
    if (isHero () and Hero.getStoryFlag ("brain incision")) {
        Hero.resetStoryFlag ("brain incision");
        msg ("Your head wound is closed.");
        id++;
    }
    mHP += hp;
    if (mHP > mMaxHP and hpmaxdice) {
        int bonus = NDX (hpmaxdice, 6);
        mMaxHP += bonus;
        mHP = mMaxHP;
        msg (fmt ("You feel much healthier.  (%+d max HP)", bonus));
        id++;
    }
    mHP = mini (mHP, mMaxHP);
    static const shCondition remedy[] = {kPlagued, kViolated, kConfused, kStunned, kSickened};
    const int size = sizeof (remedy) / sizeof (shCondition);
    for (int i = 0; i < size; ++i) {
        if (is (remedy[i])) {
            cure (remedy[i]);
            ++id;
        }
    }
    if (is (kPlagued))  ++id;
    cure (kPlagued); /* Plague might be stalled by weak healing. */
    checkTimeOuts ();
    return id;
}

static int
quaffHealing (shCreature *c, shObject *can)
{
    if (c->healing (NDX (3 + can->mBugginess, 6), 1))
        can->set (obj::known_type);
    return FULLTURN;
}

static int
quaffFullHealing (shCreature *c, shObject *can)
{
    if (c->fullHealing (NDX (4 + 2 * can->mBugginess, 8), 2))
        can->set (obj::known_type);
    return FULLTURN;
}

static int
quaffRestoration (shCreature *c, shObject *can)
{
    if (c->restoration (NDX (2, 2) + can->mBugginess)) {
        can->set (obj::known_type);
    } else {
        can->maybeName ();
    }
    return FULLTURN;
}

static int
quaffBrain (shCreature *c, shObject *can)
{
    can->set (obj::known_type | obj::known_bugginess);

    if (!can->isBuggy ()) {
        c->msg ("Brain food!");
        if (c->mAbil.base (abil::Int) < ABILITY_MAX)
            ++c->mAbil._max._int;
    } else {
        c->msg ("Yuck!  Only zombies would enjoy such food.");
    }
    if (c->mAbil.harm (abil::Int)) {
        int bonus = 1;
        ++c->mAbil._totl._int;
        if (can->isOptimized () and c->mAbil.harm (abil::Int)) {
            ++c->mAbil._totl._int;
            ++bonus;
        }
        if (c->isHero ()) {
            I->nonlOnce ();
            I->setColor (kLime);
            I->p ("  (%+d %s)", bonus,
                  abil::shortn (c->myIlk ()->mType, abil::Int));
            I->setColor (kGray);
        }
    }
    /* Temporary boost. */
    c->mAbil.temp_mod (abil::Int, NDX (2, 2));

    c->computeIntrinsics ();
    return FULLTURN;
}

static abil::Index
chooseAbility (shCreature *c, bool restore, bool gain, bool cancelable = false)
{
    shMenu *menu = I->newMenu (restore ? "You may restore an ability:" :
        "You may increase an ability:", shMenu::kNoHelp);
    menu->attachHelp ("attribute.txt");
    int entries = 0;

    FOR_ALL_ABILITIES (a) { /* Prepare options: s - Strength. */
        if (gain) {
            if (c->mAbil.base (a) >= ABILITY_MAX)
                continue;  /* Skip abilities already at ABILITY_MAX. */
        }
        if (restore) {
            if (!c->mAbil.harm (a))
                continue;  /* Skip undamaged abilities. */
        }
        char *buf = GetBuf ();
        strncpy (buf, abil::name (c->myIlk ()->mType, a), SHBUFLEN);
        buf[0] = toupper (buf[0]); /* Capitalize name. */
        menu->addIntItem (abil::name (c->myIlk ()->mType, a)[0], buf, a);
        ++entries;
    }
    if (cancelable)
        menu->addIntItem ('n', "nothing", abil::No);
    abil::Index choice;

    if (entries == 0) { /* Choice does not matter but still has to be valid. */
        return abil::Index (RNG (1, NUM_ABIL));
    }
    if (!c->isHero () or entries == 1) {
        menu->getRandIntResult ((int *) &choice);
        return choice;
    }

    int menuresult;
    int tries = 4;
    do {
        menuresult = menu->getIntResult ((int *) &choice);
        menu->dropResults ();
    } while (!menuresult and --tries);

    if (!tries) /* Undecided, eh?  We'll help. */
        menu->getRandIntResult ((int *) &choice);
    delete menu;
    return choice;
}

void
shCreature::gainAbility (bool controlled, int num)
{   /* Spice Melange enables hero to control his or her biology
       like a Bene Gesserit reverend mother could. */
    controlled = controlled or usesPower (kBGAwareness) or
                 getTimeOut (XRAYVISION);

    using namespace abil;

    Index permute[NUM_ABIL] = {Str, Con, Agi, Dex, Int, Psi};
    shuffle (permute, NUM_ABIL, sizeof (Index));

    for (int i = 0; i < num; ++i) {
        Index idx = permute[i];
        if (controlled) {
            idx = chooseAbility (this, false, true);
            controlled = false;
        }
        int a = mAbil.base (idx);
        if (a < ABILITY_MAX) {
            if (isHero ()) {
                I->setColor (kLime);
                I->p ("You feel %s!  (+1 %s)",
                      idx == Str ? "strong" :
                      idx == Con ? "tough" :
                      idx == Agi ? "agile" :
                      idx == Dex ? "deft" :
                      idx == Int ? "smart" :
                      idx == Psi ? "weird" : "bugged",
                      shortn (myIlk ()->mType, idx));
            }
            if (idx == Con) {
                if (ABILITY_MODIFIER (a) != ABILITY_MODIFIER (a + 1)) {
                    mHP += mCLevel;
                    mMaxHP += mCLevel;
                }
            }
            mAbil.perm_mod (idx, 1);
            if (isHero ())  I->setColor (kGray);
        } else {
            msg (fmt ("You are already as %s as you can get!",
                      idx == Str ? "strong" :
                      idx == Con ? "tough" :
                      idx == Agi ? "agile" :
                      idx == Dex ? "deft" :
                      idx == Int ? "smart" :
                      idx == Psi ? "weird" : "bugged"));
            if (mAbil.harm (idx)) {
                mAbil.restore (idx, 1);
                msg (fmt ("You feel restored.  (+1 %s)",
                          shortn (myIlk ()->mType, idx)));
            }
        }
    }
    computeIntrinsics ();
}

int
shCreature::needsRestoration ()
{   /* Be sensitive to ilness regardless of suppression. */
    if (getTimeOut (PLAGUED))
        return 1;

    int need = 0;
    FOR_ALL_ABILITIES (i)
        need += mAbil.harm (i);

    return need;
}

/* Returns: 1 if creature health was improved, 0 otherwise. */
int
shCreature::restoration (int howmuch)
{
    bool controlled = getTimeOut (XRAYVISION) /* Spice Melange effect. */
                      or usesPower (kBGAwareness);
    if (is (kPlagued)) {
        cure (kPlagued);
        msg ("You feel relieved!");
        return 1;
    } else {
        cure (kPlagued);
        /* No return.  Suppressed plague restores for free. */
    }

    if (!howmuch)  return 0;

    using namespace abil;
    Index permute[NUM_ABIL] = {Str, Con, Agi, Dex, Int, Psi};
    shuffle (permute, NUM_ABIL, sizeof (Index));
    int helped = 0;

    char statmod[20] = "";
    for (int i = 0; i < NUM_ABIL; ++i) {
        Index idx = permute[i];
        if (controlled and isHero ()) {
            idx = chooseAbility (this, true, false);
            controlled = false;
        }
        int harm = mAbil.harm (idx);
        if (harm) {
            /* Found another damaged stat but one was restored already. */
            if (helped) {
                msg (fmt ("You feel restored.%s", statmod));
                computeIntrinsics ();
                return 1;
            }
            if (howmuch > harm) { /* Is amount to restore too great? */
                howmuch = harm;
                helped = 1;
            }
            /* At this point it is known that 'howmuch' points are restored. */
            snprintf (statmod, 20, "  (%+d %s)",
                howmuch, shortn (myIlk ()->mType, idx));
            if (idx == Con) {
                int curr = mAbil.curr (idx);
                int hpgain = mCLevel *
                    (ABILITY_MODIFIER (curr + howmuch) - ABILITY_MODIFIER (curr));
                mMaxHP += hpgain;
                mHP += hpgain;
            }
            mAbil.restore (idx, howmuch);
            if (!mAbil.harm (idx)) {
                /* Restored ability to full potential.  Continue checking. */
                helped = 1;
                continue;
            } else {
                /* Ability is better but still damaged. */
                msg (fmt ("You feel restored.%s", statmod));
                computeIntrinsics ();
                return 1;
            }
            break;
        }
    }
    /* A stat was fixed and no other damaged ability exists.  Great! */
    if (helped) {
        msg (fmt ("You feel fully restored.%s", statmod));
        computeIntrinsics ();
        return 1;
    } else { /* No ability was damaged. */
        msg ("You feel great!");
        return 0;
    }
}

static int
quaffGainAbility (shCreature *c, shObject *can)
{
    can->set (obj::known_type);
    c->gainAbility (false, can->isOptimized () ? NUM_ABIL : 1);
    return FULLTURN;
}

static int
quaffRadAway (shCreature *c, shObject *can)
{
    if (c->mRad > 0) {
        c->mRad -= 100 + NDX (3 + can->mBugginess, 50);
        c->mRad = maxi (0, c->mRad);
    }
    if (!c->mRad)
        c->msg ("You feel purified.");
    else
        c->msg ("You feel less contaminated.");

    can->set (obj::known_type);
    return FULLTURN;
}

static int
quaffSpeed (shCreature *c, shObject *can)
{
    if (c->is (kHosed)) {
        c->cure (kHosed);
        c->checkTimeOuts ();
    }
    int numdice = 12 + 8 * can->mBugginess;
    c->inflict (kSpeedy, FULLTURN * NDX (numdice, 20));
    can->set (obj::known_type);
    return FULLTURN;
}

static int
quaffPlasma (shCreature *c, shObject *can)
{
    can->set (obj::known_type);
    c->msg ("It shocks you!");
    if (c->sufferDamage (kAttPlasma))
        c->die (kKilled, "drinking a canister of plasma");

    c->abortion ();
    return FULLTURN;
}

static int
quaffNapalm (shCreature *c, shObject *can)
{
    can->set (obj::known_type);
    c->msg ("It ignites!");
    if (c->sufferDamage (kAttNapalm))
        c->die (kKilled, "drinking a canister of napalm");

    c->abortion ();
    return FULLTURN;
}

static int
quaffLNO (shCreature *c, shObject *can)
{
    can->set (obj::known_type);
    c->msg ("It freezes!");
    if (c->sufferDamage (kAttLNO))
        c->die (kKilled, "drinking a canister of liquid nitrogen");

    c->abortion ();
    return FULLTURN;
}

static int
quaffFuel (shCreature *c, shObject *)
{   /* Poison resistance does not help against this. */
    int sdam = RNG (1, 3), cdam = RNG (1, 3);
    c->msg (fmt ("Bleargh!  (-%d str, -%d con)", sdam, cdam));
    bool kill1 = c->sufferAbilityDamage (abil::Str, sdam);
    bool kill2 = c->sufferAbilityDamage (abil::Con, cdam);
    c->abortion ();
    if (kill1 or kill2)
        c->die (kKilled, "drinking flamethrower fuel");

    return FULLTURN;
}

static int
quaffPoison (shCreature *c, shObject *can)
{
    if (c->mBodyArmor and c->mBodyArmor->isA (kObjGreenCPA)) {
        I->p ("This nourishing drink has delicious taste!");
        c->healing (NDX (3 - can->mBugginess, 6), 1);
        int dmg = c->mAbil.harm (abil::Con);
        if (dmg) {
            if (can->isBuggy ()) { /* Full restore. */
                c->mAbil.restore (abil::Con, dmg);
                dmg = 0;
            } else { /* One point only. */
                c->mAbil.restore (abil::Con, 1);
                --dmg;
            }
            if (!dmg) {
                I->p ("You feel healthy.");
            } else {
                I->p ("You feel healthier.");
            }
        }
        can->maybeName ();
    } else {
        can->set (obj::known_type);
        I->p ("This stuff must be poisonous.");
        Attacks[kAttPoisonCanister].mDamage[0].mHigh = 3 - can->mBugginess;
        if (c->sufferDamage (kAttPoisonCanister))
            c->die (kKilled, "drinking a canister of poison");
    }
    c->abortion ();
    return FULLTURN;
}

static int
quaffSpiceMelange (shCreature *c, shObject *can)
{
    can->set (obj::known_type);
    c->msg ("You fold space.");
    c->transport (-1, -1, 50 + 25 * can->mBugginess);

    c->msg ("Your senses open to new dimensions.");
    c->setTimeOut (TELEPATHY, FULLTURN * NDX (10, 30));
    c->setTimeOut (XRAYVISION, FULLTURN * NDX (10, 30));
    c->inflict (kSpeedy, FULLTURN * NDX (5, 2));
    c->mInnateIntrinsics.mod (kTelepathy, +15);
    c->mInnateIntrinsics.mod (kXRayVision, +10);
    c->computeIntrinsics ();
    /* Add +20% chances for each Voice effect in power over you. */
    int chance = (c->is (kUnableCompute) > 0) * 20 +
        (c->is (kUnableUsePsi) > 0) * 20 + (c->is (kUnableAttack) > 0) * 20;
    if (RNG (100) < chance)
        c->getMutantPower (kTheVoice);

    return FULLTURN;
}

static int
quaffEmbryo (shCreature *c, shObject *can)
{
    can->set (obj::known_type);
    c->msg ("You drink something writhing and slimy.");
    if (!c->mImpregnation)  ++c->mImpregnation;
    return FULLTURN;
}

static int
quaffBuffout (shCreature *c, shObject *can)
{
    if (c->mAbil.temp (abil::Str) > 0) {
        c->msg ("You overdose Buffout.");
        if (c->sufferAbilityDamage (abil::Str, 2 + can->mBugginess, true))
            c->die (kMisc, "Died of Buffout overdose");
        return FULLTURN;
    }

    int boost = RNG (3, 7) + 2 * can->mBugginess;
    boost = maxi (2, mini (8, boost));
    c->mAbil.temp_mod (abil::Str, boost);
    return FULLTURN;
}

/* TODO: incomplete */
static int
quaffGravy (shCreature *c, shObject *can)
{
    bool death = false;
    can->set (obj::known_type);

    c->msg ("You hear a dark, enticing voice in the back of your head.");
    c->msg ("\"Is there something you hold dear?\"");
    if (c->isHero ())  I->doMorePrompt ();
    abil::Index idx = chooseAbility (c, false, false, true);
    if (idx != abil::No) {
        c->msg ("You are altered!");
        death = c->sufferAbilityDamage (idx, 5, true);

        FOR_ALL_ABILITIES (a)
            if (a != idx)  c->mAbil.perm_mod (a, 1);

        if (death)
            c->die (kKilled, "treachery of the Sith");
        return LONGTURN;
    }

    c->msg ("The voice becomes more seducing and somehow closer.");
    c->msg ("\"Is there something you despise?\"");
    if (c->isHero ())  I->doMorePrompt ();
    idx = chooseAbility (c, false, false, true);
    if (idx != abil::No) {
        c->msg ("You are altered!");
        FOR_ALL_ABILITIES (a)
            if (a != idx)
                death = c->sufferAbilityDamage (a, 1, true) or death;

        c->mAbil.perm_mod (idx, 5);
        if (death)
            c->die (kKilled, "treachery of the Sith");
        return LONGTURN;
    }

    c->msg ("The voice takes on angry tone, booming all around you.");
    c->msg ("\"Very well, you are worthy, proud and untrusting.\"");
    c->msg ("\"TASTE THE POWER!\"");
    if (c->isHero ())  I->doMorePrompt ();
    /* TODO: since max hero power is constant, use array instead of vector. */
    shVector<shMutantPower> powers;
    for (int i = kNoMutantPower; i < kMaxHeroPower; ++i)
        if (c->mMutantPowers[i] != 0)
            powers.add (shMutantPower (i));



    c->msg ("Dark energies course through your cells mutating you!");
    int power_lvl = 0;
    if (powers.count () == 0) {
        power_lvl = RNG (3, 5);
        FOR_ALL_ABILITIES (a)
            if (a != abil::Psi)
                death = c->sufferAbilityDamage (a, 1, true) or death;

        c->mAbil.perm_mod (abil::Psi, 5);
        if (death)
            c->die (kKilled, "treachery of the Sith");

    } else if (powers.count () == 1) {
        power_lvl = 0; /* TODO: stopped here */
        c->lose_mutant_power (powers.get (1));
    }

    powers.reset ();
    for (int i = kNoMutantPower; i < kMaxHeroPower; ++i)
        if (MutantPowers[i].mLevel == power_lvl)
            powers.add (shMutantPower (i));
    c->getMutantPower (powers.get (RNG (powers.count ())));

    return LONGTURN;
}

static int
useCanister (shCreature *user, shObject *can)
{
    int t = useCommonCanister (user, can);
    if (t) {
        if (user->isInShop ()) {
            user->usedUpItem (can, 1, "use");
        }
        user->useUpOneObjectFromInventory (can);
    }
    return t;
}


void
initializeCanisters ()
{
    for (int i = 0; i < kObjNumIlks; ++i)
        if (AllIlks[i].mReal.mType == kCanister)
            AllIlks[i].mUseFunc = useCanister;

    AllIlks[kObjBeer].mQuaffFunc = quaffBeer;
    AllIlks[kObjSuperGlue].mQuaffFunc = quaffSuperGlue;
    AllIlks[kObjNanoCola].mQuaffFunc = quaffNanoCola;
    AllIlks[kObjNukaCola].mQuaffFunc = quaffNukaCola;
    AllIlks[kObjB3].mQuaffFunc = quaffB3;
    AllIlks[kObjCoffee].mQuaffFunc = quaffCoffee;
    AllIlks[kObjWater].mQuaffFunc = quaffWater;
    AllIlks[kObjRadAway].mQuaffFunc = quaffRadAway;
    AllIlks[kObjRestorationCanister].mQuaffFunc = quaffRestoration;
    AllIlks[kObjHealingCanister].mQuaffFunc = quaffHealing;
    AllIlks[kObjLNO].mQuaffFunc = quaffLNO;
    AllIlks[kObjNapalm].mQuaffFunc = quaffNapalm;
    AllIlks[kObjUniversalSolvent].mQuaffFunc = quaffUniversalSolvent;
    AllIlks[kObjSpeedCanister].mQuaffFunc = quaffSpeed;
    AllIlks[kObjPoisonCanister].mQuaffFunc = quaffPoison;
    AllIlks[kObjPlasmaCanister].mQuaffFunc = quaffPlasma;
    AllIlks[kObjMutagenCanister].mQuaffFunc = quaffMutagen;
    AllIlks[kObjFullHealingCanister].mQuaffFunc = quaffFullHealing;
    AllIlks[kObjGainAbilityCanister].mQuaffFunc = quaffGainAbility;
    AllIlks[kObjSpiceMelange].mQuaffFunc = quaffSpiceMelange;
    AllIlks[kObjAntimatter].mQuaffFunc = quaffAntimatter;
    AllIlks[kObjBrain].mQuaffFunc = quaffBrain;
    AllIlks[kObjCannedEmbryo].mQuaffFunc = quaffEmbryo;
    AllIlks[kObjBuffout].mQuaffFunc = quaffBuffout;
    AllIlks[kObjSithGravy].mQuaffFunc = quaffGravy;

    AllIlks[kObjFlamerFuel].mQuaffFunc = quaffFuel;
}


int
shCreature::quaffCanister (shObject *can)
{
    if (isHero () and can->is (obj::known_type) and can->isA (kObjAntimatter)) {
        if (!I->yn ("Dude, really drink ANTIMATTER?!"))
            return 0;
    }

    if (isInShop ()) {
        usedUpItem (can, 1, "drink");
    }
    int elapsed = (can->myIlk ()->mQuaffFunc) (this, can);

    /* Can happen.  In this case 'can' pointer is often invalid. */
    if (mState == kDead)
        return elapsed;

    if (can->is (obj::known_type)) { /* Identify rest of stack. */
        /* TODO: Find out why is it there and what it really does? */
        can->set (obj::known_appearance);
    }
    if (can->isChargeable ()) {
        --can->mCharges;
        /* Emptied one container - throw it away. */
        if ((can->mCharges % can->myIlk ()->mMaxCharges) == 0)
            useUpOneObjectFromInventory (can);
    } else {
        useUpOneObjectFromInventory (can);
    }
    return elapsed;
}

/* Also drinking, but not from canister. */
void
shCreature::quaffFromAcidPit ()
{
    if (sufferDamage (kAttAcidPitSip))
        die (kKilled, "drinking from an acid pit");
    abortion ();
}
