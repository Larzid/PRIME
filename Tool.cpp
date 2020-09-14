#include <ctype.h>
#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"

int
shCreature::attemptRestraining (shObject *bolt)
{
    if (isA (kMonClerkbot) or isA (kMonDocbot)) { /* nice try */
        I->p ("The restraining bolt is vaporized by %s's "
              "anti-shoplifting circuits!", the ());
        newEnemy (Hero.cr ());
        delete bolt;
        return 0;
    }
    addObjectToInventory (bolt);
    if (!isPet ())  makePet ();
    return 1;
}

static int
useRestrainingBolt (shCreature *user, shObject *bolt)
{
    assert (NOT0 (user,->isHero ()));
    if (!user->isHero ())  return FULLTURN;
    int x = user->mX;
    int y = user->mY;

    shCreature *c;
    shDirection dir = I->getDirection ();
    switch (dir) {
    case kNoDirection:
    case kUp:
    case kDown:
        return 0;
    default:
        if (!Level->moveForward (dir, &x, &y)) {
            return 0;
        }
        c = Level->getCreature (x, y);
        if (!c or !c->isRobot ()) {
            user->msg ("Restraining bolts only work on bots and droids.");
            return 0;
        }
        if (c->isPet ()) {
            bool bolt = false;
            for (int i = 0; i < c->mInventory->count (); ++i)
                if (c->mInventory->get (i)->isA (kObjRestrainingBolt)) {
                    bolt = true;
                    break;
                }
            if (bolt) {
                user->msg (fmt ("You've already affixed a restraining bolt to %s.", THE (c)));
            } else {
                user->msg (fmt ("There is no need to restrain %s.", THE (c)));
            }
            return 0;
        }

        int power = bolt->mBugginess + 4;

        bool success = false;
        if (c->mCLevel <= power) {
            success = true;
        } else {
            success = RNG (c->mCLevel) < power;
        }

        if (!success) {
            user->msg (fmt ("You miss %s.", THE (c)));
            c->newEnemy (user);
        } else {
            user->usedUpItem (bolt, 1, "attach");
            bolt = user->removeOneObjectFromInventory (bolt);
            user->msg (fmt ("You attach the restraining bolt to %s.", THE (c)));
            c->attemptRestraining (bolt);
        }
        return FULLTURN;
    }
}


static void
flail (shObject *md, const char *what)
{
    I->p ("You flail %s with your limp %s.", what, md->getShortDescription ());
    I->p ("For some reason it doesn't appear to accomplish anything.");
}


static int
repairObject (shCreature *user, shObject *tool)
{
    assert (NOT0 (user,->isHero ()));
    shObjectVector v;
    selectObjectsByFunction (&v, user->mInventory, &shObject::isDamaged);
    if (!v.count ()) {
        I->p ("You have nothing to repair.");
        return 0;
    }
    shObject *obj = user->quickPickItem (&v, "repair", 0);
    shObject *material;
    if (!obj) return 0;
    if (obj->myIlk ()->mMaterial == kFleshy) {
        I->p ("Repair a piece of meat?  How?");
        return 0;
    }
    if (!obj->is (obj::known_type) or !obj->is (obj::known_appearance)) {
        I->p ("But you don't surely know what it really is.");
        return 0;
    }
    if (obj->is (obj::worn)) {
        I->p ("Take it off first.");
        return 0;
    }
    if (tool->isA (kObjMechaDendrites) and !tool->is (obj::worn)) {
        flail (tool, obj->theQuick ());
        return FULLTURN;
    }
    material = user->quickPickItem (user->mInventory, "get material from", 0);
    if (!material) {
        I->p ("You cannot repair without spare parts.");
        return 0;
    }
    if (obj == material) {
        I->p ("You cannot repair %s with itself.", THE (obj));
        return 0;
    } else if (material->is (obj::worn)) {
        I->p ("Take it off first.");
        return 0;
    } else if (!material->is (obj::known_appearance)) {
        I->p ("But you don't surely know what the material is made of.");
        return 0;
    } else if (material->myIlk ()->mMaterial == kFleshy) {
        I->p ("Piece of meat as material? What are you thinking?");
        return 0;
    } /* Now the complex rules begin. */
    if (obj->isThrownWeapon ()) {
        if (!material->isThrownWeapon ()) {
            I->p ("%s is too dissimilar.", THE (material));
            return 0;
        }
    } else if (obj->isMeleeWeapon ()) {
        if (obj->myIlk ()->mMaterial != material->myIlk ()->mMaterial) {
            I->p ("To repair a melee weapon you need more of the material its made of.");
            return 0;
        }
    } else if (obj->isAimedWeapon ()) {
        if (!(material->isAimedWeapon ())) {
            I->p ("You need to repair it with another ranged weapon.");
            return 0;
        }
        if ((obj->isA (kObjLightFlamer) or obj->isA (kObjFlamer)) and
            (!material->isA (kObjLightFlamer) and !material->isA (kObjFlamer)))
        {
            I->p ("To repair a flamethrower you need another one.");
            return 0;
        } else {
            if (obj->myIlk ()->mParent != material->myIlk ()->mParent) {
                I->p ("These weapons belong to different class.");
                return 0;
            }
        }
    } else if (obj->isA (kArmor)) {
        if (obj->has_subtype (jumpsuit)) {
            if (!material->has_subtype (jumpsuit)) {
                I->p ("You need another jumpsuit.");
                return 0;
            }
            if (obj->isA (kObjChameleonSuit) and
                !material->isA (kObjChameleonSuit))
            {
                I->p ("Chameleon suits must be repaired using"
                      " patches from another one.");
                I->p ("Otherwise its camouflaging properties"
                      " would be diminished.");
                return 0;
            } else if (obj->isA (kObjRadiationSuit) and
                       !material->isA (kObjRadiationSuit))
            {
                I->p ("Radiation suits must be repaired using"
                      " patches from another one.");
                I->p ("Otherwise its protective properties"
                      " would be diminished.");
                return 0;
            } /* Otherwise ok. */
        } else if (obj->has_subtype (body_armor) or
                   obj->has_subtype (helmet))
        {   /* Compatibility rules:
                - type match: helmet <-> helmet or body armor <-> body armor
                - powered match: both powered or none
                - material type match */
            if (obj->myIlk ()->mParent != material->myIlk ()->mParent) {
                I->p ("Armor types do not match.");
                return 0;
            }
            int powered = obj->isPoweredArmor () + material->isPoweredArmor ();
            if (powered % 2) {
                I->p ("One of those armor pieces is powered.");
                I->p ("Either both of those should be powered or none of them.");
                return 0;
            }
            if (obj->myIlk ()->mMaterial != material->myIlk ()->mMaterial) {
                I->p ("The material types do not match.");
                return 0;
            }
        } else {
            I->p ("This armor cannot be repaired.");
            return 0;
        }
    } else {
        I->p ("This object does not have repair handle.  Please report this bug.");
        return 0;
    }
    int rad = obj->isRadioactive () + material->isRadioactive ();
    /* Finally, repair. */
    user->removeObjectFromInventory (material);
    delete material;
    if (rad) {
        shObjectIlk *rad_gr = &AllIlks[kObjRadGrenade];
        for (int i = 0; i < rad; ++i) {
            user->sufferDamage (rad_gr->mMissileAttack);
        }
    }
    if (sportingD20 () + user->getSkillModifier (kRepair) > 15) {
        obj->mDamage = 0;
        I->p ("Now %s looks as good as new!", THE (obj));
    } else {
        I->p ("Your repair attempt is a failure.");
    }
    return 1;
}


static int
makeRepair (shCreature *user, shObject *tool)
{
    int x = user->mX, y = user->mY;
    shFeature *f;
    shCreature *c;
    shObjectVector *v;
    int flailing = 0;
    bool ducttape = false;
    int basescore = user->getSkillModifier (kRepair) + 2 * tool->mBugginess;

    if (tool->isA (kObjDuctTape)) {
        basescore += 8;
        ducttape = true;
    } else if (tool->isA (kObjMonkeyWrench)) {
        basescore += tool->mEnhancement;
    }
    if (tool->isA (kObjMechaDendrites) and !tool->is (obj::worn)) {
        flailing = 1;
    }
    int score = RNG (1, 20) + basescore;
    I->diag ("Make a repair score: base %d, with roll %d.", basescore, score);

    I->p ("Select yourself to repair items in inventory.");
    shDirection dir = I->getDirection ();
    switch (dir) {
    case kNoDirection:
        return 0;
    case kUp:
        if (Level->mSquares[x][y].mTerr == kBrokenLightAbove) {
            if (flailing) {
                flail (tool, "the broken light");
                return FULLTURN;
            } else if (Hero.mProfession == Janitor) {
                int room = Level->getRoomID (x, y);
                int sx = x, sy = y, ex = x, ey = y;
                Level->getRoomDimensions (room, &sx, &sy, &ex, &ey);
                for (int ty = sy; ty <= ey; ++ty)
                    for (int tx = sx; tx <= ex; ++tx)
                        Level->setLit (tx, ty);
                Level->mSquares[x][y].mTerr = kFloor;
                I->p ("You replace the missing lightbulb.");
                if (tool->isA (kObjMechaDendrites)) {
                    I->p ("Screwing it with %s was fun.", YOUR (tool));
                }
                user->beatChallenge (Level->mDLevel);
                break;
            } else {
                I->p ("The lightbulb is missing.  Call janitor to bring a spare.");
                return 0;
            }
        }
        I->p ("There's nothing to repair on the ceiling.");
        return 0;
    case kOrigin:
        if (!ducttape) {
            if (!repairObject (user, tool)) {
                return 0;
            } else {
                break;
            }
        }
    default:
        if (!Level->moveForward (dir, &x, &y)) {
            I->p ("There's nothing there to repair.");
            return 0;
        }
        c = Level->getCreature (x, y);
        if (c and dir != kDown and ducttape) {
            I->p ("You tape %s.", c->isHero () ? "yourself" : THE (c));
            c->mTrapped.mTaped += NDX (2, 2) + tool->mBugginess;
            break;
        }
        if (c and dir != kDown) {
            /* repair a pet droid */
            const char *who = THE (c);
            if (c->isHero ()) {
                I->p ("You can't make repairs to yourself!");
                return 0;
            } else if (!c->isRobot ()) {
                I->p ("You can't make repairs to %s!", who);
                return 0;
            } else if (c->mHP == c->mMaxHP) {
                I->p ("%s doesn't appear to be damaged.", who);
                return 0;
            } else if (!c->isPet ()) { /* Replace with mDisposition check? */
                I->p ("%s won't cooperate with your attempt to repair it.",
                      who);
                return FULLTURN;
            } else if (flailing) {
                flail (tool, who);
                return FULLTURN;
            } else if (score < 15) {
                I->p ("Your repair attempt is a failure.");
            } else {
                c->mHP += score - 15 + RNG (1, 3);
                if (c->mHP >= c->mMaxHP) {
                    c->mHP = c->mMaxHP;
                    I->p ("Now %s looks as good as new!", who);
                } else {
                    I->p ("You repair some of the damage on %s.", who);
                }
            }
            break;
        }
        if ((v = Level->getObjects (x, y))) {
            int i;
            shObject *obj;
            int nx = x;
            int ny = y;
            int repaired = 0;

            for (i = 0; i < v->count (); i++) {
                obj = v->get (i);
                if (obj->isA (kObjWreck)) {
                    if (nx == user->mX and ny == user->mY and
                        -1 == Level->findNearbyUnoccupiedSquare (&nx, &ny))
                    {
                        I->p ("There is %s here, but you need more room "
                              "to repair it.", AN (obj));
                        break;
                    }
                    if (I->yn ("Repair %s?", THE (obj))) {
                        if (flailing) {
                            flail (tool, THE (obj));
                            return FULLTURN;
                        } else if (score < 20) {
                            I->p ("Your repair attempt is a failure.");
                        } else {
                            shCreature *bot = shCreature::monster (obj->mWreckIlk);
                            /* Prevent XP farming. */
                            bot->mConditions |= kNoExp;
                            /* Prevent drop farming. */
                            for (int i = bot->mInventory->count () -1; i >= 0; --i) {
                                shObject *obj = bot->mInventory->get (i);
                                bot->removeObjectFromInventory (obj);
                                delete obj;
                            }
                            Level->putCreature (bot, nx, ny);
                            I->p ("You bring %s back on-line!", THE (obj));
                            v->remove (obj);
                            delete obj;
                        }
                        repaired = 1;
                        break;
                    }
                }
            }
            if (repaired) break;
        }

        if ((f = Level->getFeature (x, y))) {
            if (f->isAutomaticDoor () and f->isBerserkDoor () and
                !f->mTrapUnknown)
            {
                if (flailing) {
                    flail (tool, "the door");
                } else if (score >= 12) {
                    f->mDoor.mFlags &= ~shFeature::kBerserk;
                    I->p ("You repair the malfunctioning door.");
                    if (Hero.mProfession == Janitor) {
                        /* A shout out to ADOM. :) */
                        I->p ("You feel like behaving properly.");
                        /* For doing what you should be. */
                        user->beatChallenge (Level->mDLevel);
                    }
                    Level->remember (x, y, f); /* For blind chars. */
                } else {
                    I->p ("You repair attempt is a failure.");
                }
                break;
            } else if (f->isLockBrokenDoor ()) {
                if (flailing) {
                    flail (tool, "the broken lock");
                } else if (score >= 15) {
                    f->mDoor.mFlags &= ~shFeature::kLockBroken;
                    I->p ("You repair the broken lock.");
                    /* No Janitor bonus.  This could be abused. */
                } else {
                    I->p ("You repair attempt is a failure.");
                }
                break;
            } else if (f->isAutomaticDoor () and f->isInvertedDoor ()) {
                if (flailing) {
                    flail (tool, "the inverted logic circuits");
                } else if (score >= 15) {
                    f->mDoor.mFlags &= ~shFeature::kInverted;
                    I->p ("You rewire the inverted logic circuits.");
                    if (Hero.mProfession == Janitor) {
                        user->beatChallenge (Level->mDLevel);
                    }
                } else {
                    I->p ("You repair attempt is a failure.");
                }
                break;
            } else if (shFeature::kRadTrap == f->mType and !f->mTrapUnknown) {
                const int raddiff = 24;
                if (dir != kDown) {
                    I->p ("You need to stand closer.");
                    return 0;
                } else if (flailing) {
                    flail (tool, "the poor rad trap");
                } else if (score >= raddiff) {
                    I->p ("You dismantle the trap.");
                    Level->removeFeature (f);
                } else {
                    int chance = (basescore - raddiff + 20) * 5;
                    I->p ("Whoops!  This task is %s",
                          chance <= 0 ? "impossible!" :
                          chance <= 25 ? "difficult." :
                          chance <= 50 ? "supposed to be doable." :
                          chance <= 75 ? "supposed to be easy." :
                                         "supposed to be a trivial matter!");
                    Level->checkTraps (x, y, 100);
                }
                break;
            } else if ((shFeature::kPit == f->mType or
                        shFeature::kAcidPit == f->mType or
                        shFeature::kHole == f->mType) and ducttape)
            {
                if (user->mZ == -1) {
                    I->p ("Get out first.");
                    return 0;
                }
                I->p ("You tape %s opening.  Looks almost like a solid ground.",
                    f->mType == shFeature::kHole ? "hole" : "pit");
                /* Make sure no intelligent enemy saw you at work. */
                int observers = 0;
                for (int i = 0; i < Level->mCrList.count (); ++i) {
                    shCreature *c = Level->mCrList.get (i);
                    if (c and c->mAbil.Int () > 6 and c->canSee (user) and
                        !c->isHero () and !c->isPet ())
                    {
                        ++observers;
                        if (user->canSee (c))
                            user->msg (fmt ("%s watches you closely.", THE (c)));
                    }
                } /* It shall be conveniently assumed monsters had plenty
                     of time to forget the trap existed in the first place. */
                if (!observers)  f->mTrapMonUnknown = 2;
                break;
            } else {
                I->p ("It ain't broke!");
                return 0;
            }
        }
        I->p ("There's nothing there to repair.");
        return 0;
    }
    if (tool->isA (kObjMonkeyWrench) or
        (tool->isA (kObjMechaDendrites) and !flailing)) {
        /* This is intentionally a ripoff in case of wrench. */
        user->employedItem (tool, 10);
    }
    if (flailing) {
        user->employedItem (tool, 0); /* This leads to a YAFM. */
    }
    /* Superglue will get used up in the useCanister () function. */
    if (ducttape)
        user->useUpOneObjectFromInventory (tool);

    /* Yeah, realistically this should take longer, but that wouldn't
       be very fun for the player I think. -- CADV */
    return FULLTURN;
}


static int
useDuctTape (shCreature *user, shObject *tape)
{
    if (user->mWeapon and user->mWeapon->isWeldedWeapon ()) {
        user->msg ("You cannot free both your hands.");
        if (user->isHero () and !user->mWeapon->is (obj::known_bugginess)) {
            user->mWeapon->set (obj::known_bugginess);
            user->mWeapon->announce ();
        }
        return 0;
    }
    return makeRepair (user, tape);
}

static int
useMechaDendrites (shCreature *user, shObject *dendrites)
{
    return makeRepair (user, dendrites);
}

static int
useMonkeyWrench (shCreature *user, shObject *wrench)
{
    if (NOT0 (user->mWeapon, != wrench) and
        user->mWeapon->isWeldedWeapon ())
    {
        I->p ("You cannot free both your hands.");
        if (!user->mWeapon->is (obj::known_bugginess)) {
            user->mWeapon->set (obj::known_bugginess);
            user->mWeapon->announce ();
        }
        if (user->mWeapon->isA (kObjMonkeyWrench))
            I->p ("But why not use the wrench you are holding instead?");
        return 0;
    }
    int elapsed = makeRepair (user, wrench);
    if (elapsed and user->mWeapon != wrench and wrench->isBuggy () and
        !user->isOrc ())
    {
        if (user->mWeapon)  user->unwield (user->mWeapon);
        user->wield (wrench, 1);
        bool wasknown = wrench->is (obj::known_bugginess);
        wrench->set (obj::known_bugginess);
        I->p ("You cannot put away %s%c", YOUR (wrench), wasknown ? "." : "!");
    }
    return elapsed;
}


int
shCreature::useKey (shObject *key, shFeature *door)
{
    int elapsed = 0;
    int locked = door->isLockedDoor ();
    const char *does_what = locked ? "unlocks" : "locks";
    shObjectIlk *cardneeded = door->keyNeededForDoor ();

    if (shFeature::kDoorHiddenHoriz == door->mType or
        shFeature::kDoorHiddenVert == door->mType)
    {
        I->p ("There's no lock there.");
        return 0;
    }
    if (door->isLockBrokenDoor ()) {
        I->p ("The lock on this door is broken.");
        return QUICKTURN;
    }
    if (!cardneeded and !door->isRetinaDoor ()) {
        I->p ("There is no lock on this door.");
        return QUICKTURN;
    }
    if (door->isOpenDoor ()) {
        I->p ("You have to close it first.");
        return 0;
    }

    if (cardneeded and key->has_subtype (keycard)) {
        const char *key_d = key->getShortDescription ();
        if (intr (kBlind)) {
            if (!key->isBuggy () and
                (key->isA (kObjMasterKeycard) or
                 key->myIlk () == cardneeded or
                 key->is (obj::cracked)))
            {
                I->p ("You swipe your %s and the door lock accepts it.", key_d);
                if (key->is (obj::known_appearance) and /* You know keycard color. */
                /* The keycard is not cracked or you are unaware of the fact. */
                    (!key->is (obj::cracked | obj::known_cracked)) and
                /* This is not master keycard or you are unaware of the fact. */
                    (!key->isA (kObjMasterKeycard) or !key->is (obj::known_type)))
                { /* You infer the door lock must match color of keycard. */
                    int codeflag = door->mDoor.mFlags & shFeature::kColorCode;
                    Level->mRemembered[door->mX][door->mY].mDoor &= ~shFeature::kColorCode;
                    Level->mRemembered[door->mX][door->mY].mDoor |= codeflag;
                }
            } else {
                I->p ("You swipe your %s but it seems not to work.", key_d);
                return QUICKTURN;
            }
        } else if (key->isBuggy () and key->is (obj::known_bugginess)) {
            I->p ("But your %s is buggy.", key_d);
            return 0;
        } else if (key->myIlk () == cardneeded) {
            if (key->isBuggy ()) {
                I->p ("You swipe %s but the door does not react.", key_d);
                I->p ("It must be buggy.");
                key->set (obj::known_bugginess);
                return QUICKTURN;
            }
            I->p ("You swipe your %s and the door %s.", key_d, does_what);
        } else if (key->isBuggy ()) {
            I->p ("You swipe your %s but nothing happens.", key_d);
            if ((key->isA (kObjMasterKeycard) and key->is (obj::known_type)) or
                (key->is (obj::cracked | obj::known_cracked)))
            {
                I->p ("Damn it!  This keycard must be buggy.");
                key->set (obj::known_bugginess);
            } else {
                I->p ("Obviously the keycard and lock colors do not match.");
            }
            return QUICKTURN;
        } else if (key->isA (kObjMasterKeycard)) {
            I->p ("You swipe your %s and the door %s.", key_d, does_what);
            if (!key->is (obj::known_type) and key->myIlk () != cardneeded) {
                I->p ("This must be the master keycard.");
                key->set (obj::known_type);
            }
        } else if (key->is (obj::cracked)) {
            if (key->is (obj::known_cracked)) {
                I->p ("You swipe your %s and the door %s.", key_d, does_what);
            } else {
                I->p ("You swipe your %s.  To your surprise the door %s.",
                    key_d, does_what);
                I->p ("This keycard must be cracked.");
                key->set (obj::known_cracked);
            }
        } else {
            I->p ("You swipe your %s but nothing happens.", key_d);
            I->p ("Obviously the keycard and lock colors do not match.");
            if (key->is (obj::known_bugginess))
                key->set (obj::known_cracked);
            return QUICKTURN;
        }
        elapsed = QUICKTURN;
    } else if (key->isA (kObjLockPick) or key->isA (kObjMechaDendrites)) {
        int score = sportingD20 () + getSkillModifier (kOpenLock);
        if (door->isRetinaDoor ())
            score -= 10;
        if (!locked)
            score += 4;
        if (score >= 20) {
            if (locked) {
                I->p ("You run a bypass on the locking mechanism.");
                if (door->isAlarmedDoor () and Hero.mProfession == Ninja) {
                    I->p ("You also disable the door's alarm system.  Ninja'd!");
                    door->mDoor.mFlags &= ~shFeature::kAlarmed;
                    beatChallenge (Level->mDLevel);
                }
                if (Hero.mProfession == Ninja and !key->is (obj::known_bugginess)) {
                    key->set (obj::known_bugginess);
                    key->announce ();
                }
            } else {
                I->p ("You lock the door.");
            }
            elapsed = FULLTURN;
        } else {
            if (door->isRetinaDoor ())
                I->p ("It's very difficult to bypass "
                      "the retina scanner.");
            if (door->isAlarmedDoor () and score <= 10) {
                I->p ("You set off an alarm!");
                Level->doorAlarm (door);
            } else {
                I->p ("You fail to defeat the lock.");
            }
            return FULLTURN;
        }
    } else if (door->isRetinaDoor ()) {
        I->p ("You need the proper retina to unlock this door.");
    } else {
        if (intr (kBlind)) {
            I->p ("Nothing happens.");
        } else {
            char *buf = GetBuf ();
            strcpy (buf, door->getDoorLockName ());
            /* Only orange key needs to be preceded by 'an'. */
            const char *an = buf[0] == 'o' ? "an" : "a";
            I->p ("You need %s %s keycard to %s this door.", an, buf,
                  locked ? "unlock" : "lock");
        }
        return QUICKTURN;
    }
    if (locked) {
        door->unlockDoor ();
    } else {
        door->lockDoor ();
    }
    return elapsed;
}


static int
useKeyTool (shCreature *user, shObject *key)
{
    assert (NOT0 (user,->isHero ()));
    int x = user->mX, y = user->mY;
    shFeature *f;

    shDirection dir = I->getDirection ();
    switch (dir) {
    case kNoDirection:
        return 0;
    case kUp:
        I->p ("There's no lock on the ceiling.");
        return 0;
    default:
        if (!Level->moveForward (dir, &x, &y)) {
            I->p ("There's no lock there.");
            return 0;
        }
        if ((f = Level->getFeature (x, y)) and f->isDoor ()) {
            return user->useKey (key, f);
        }
        I->p ("There's no lock there.");
        return 0;
    }
}


static int
useComputer (shCreature *user, shObject *computer)
{
    if (user->is (kUnableCompute)) {
        I->p ("You dare to read the Nam-Shub?!  Defiler.");
        return 0;
    }

    shObjectVector v;
    selectObjects (&v, user->mInventory, kFloppyDisk);
    shObject *obj = user->quickPickItem (&v, "execute", 0);
    if (!obj)  return 0;

    return computer->executeProgram (obj);
}


static int
useMedicomp (shCreature *user, shObject *medicomp)
{
    const int PLAGUE_COST = 10;
    const int RESTORE_COST = 4;
    if (!medicomp->mCharges) {
        I->p ("The medicomp is out of charges.");
        int known = medicomp->is (obj::known_charges);
        medicomp->set (obj::known_charges);
        return known ? 0 : HALFTURN;
    }
    if (Hero.mProfession == Yautja)  medicomp->set (obj::known_charges);
    if (user->is (kPlagued)) {
        if (medicomp->mCharges >= PLAGUE_COST) {
            medicomp->mCharges -= PLAGUE_COST;
            user->restoration (1);
            return LONGTURN;
        } else {
            I->p ("Not enough charges to cure plague.");
            int known = medicomp->is (obj::known_charges);
            medicomp->set (obj::known_charges);
            return known ? 0 : HALFTURN;
        }
    }
    if (user->mHP < user->mMaxHP or user->is (kViolated) or
        user->is (kStunned) or user->is (kConfused) or
        (user->isHero () and Hero.getStoryFlag ("brain incision")))
    {
        --medicomp->mCharges;
        user->healing (NDX (5 + medicomp->mBugginess, 4), 0);
        return LONGTURN;
    }
    if (user->needsRestoration ()) {
        if (medicomp->mCharges >= RESTORE_COST) {
            medicomp->mCharges -= RESTORE_COST;
            user->restoration (2 + medicomp->mBugginess);
            return LONGTURN;
        } else {
            I->p ("Not enough charges to restore abilities.");
            int known = medicomp->is (obj::known_charges);
            medicomp->set (obj::known_charges);
            return known ? 0 : HALFTURN;
        }
    }
    I->p ("Nothing to cure.");
    return 0;
}

static int
scanVat (shCreature *c, shFeature *f, shObject *tricorder)
{
    const int vatScanCost = 5;
    if (f->mVat.mAnalyzed) {
        I->p ("You already know this is %s.", f->getDescription ());
        return 0;
    }
    if (c->countEnergy () < vatScanCost) {
        I->p ("You need %d energy to scan this vat.", vatScanCost);
        return 0;
    }
    c->loseEnergy (vatScanCost);
    /* Intentionally says nothing about radioactiveness.
       Use Geiger counter for this purpose. */
    f->mVat.mAnalyzed = 1;
    I->p ("According to %s this is %s.",
        YOUR (tricorder), f->getDescription ());
    I->drawSideWin (c);
    return LONGTURN;
}

static int
useTricorder (shCreature *user, shObject *tricorder)
{   /* If you change values below remember to update Tricorder lore. */
    const int quickScanCost = 1;
    const int brainScanCost = 2;
    const int diagnosisCost = 9;
    const char *nothing = "There is nothing to analyze.";
    static const char *creatureTypeName[kMaxCreatureType] =
    {
        "unliving",
        "unliving",
        "unliving",
        "unliving",
        "egg",
        "ooze",
        "cyborg",
        "aberration",
        "animal",
        "alien",
        "beast",
        "humanoid",
        "mutant humanoid",
        "insect",
        "unknown lifeform", /* outsider */
        "vermin",
        "zerg"
    };

    if (!tricorder->is (obj::known_type)) {
        I->p ("It is a tricorder.");
        tricorder->set (obj::known_type);
        I->pause ();
    }
    int x = user->mX, y = user->mY;
    shCreature *c;
    shFeature *f;
    shObjectVector *v;

    if (tricorder->isBuggy ()) {
        I->p ("It appears to be broken.");
        tricorder->set (obj::known_bugginess);
        return 0;
    }
    shDirection dir = I->getDirection ();
    switch (dir) {
    case kNoDirection:
        return 0;
    case kUp:
        I->p (nothing);
        return 0;
    case kDown:
        f = Level->getFeature (x, y);
        if (f and f->mType == shFeature::kVat) {
            int ret = scanVat (user, f, tricorder);
            if (ret)  return ret;
        }
        v = Level->getObjects (x, y);
        if (v and user->countEnergy () < 1) {
            I->p ("You need power for %s to scan this pile.", YOUR (tricorder));
            return 0;
        }
        if (v) {  /* Check stuff on ground. */
            int scans = 0;
            for (int i = 0; i < v->count (); ++i) {
                shObject *obj = v->get (i);
                if (obj->is (obj::known_type) and
                    (!(obj->isA (kObjBrain) or obj->isA (kObjBioArmor) or
                       obj->isA (kObjBioComputer)) or
                     obj->is (obj::known_bugginess)))
                {  /* Fully scanned; skip. */
                    continue;
                }
                if (user->countEnergy () < quickScanCost) {
                    I->p ("You lack %d energy to continue this scan.",
                          quickScanCost);
                    break;
                }
                user->loseEnergy (quickScanCost);
                ++scans;
                if (obj->isA (kObjBrain) or obj->isA (kObjBioArmor) or
                    obj->isA (kObjBioComputer))
                {
                    if (!obj->is (obj::known_type)) {
                        obj->set (obj::known_type);
                        I->p ("You discover %s!", AN (obj));
                    }
                    if (user->countEnergy () >= brainScanCost and
                        !obj->is (obj::known_bugginess))
                    {
                        if (I->yn ("Scan %s closely?  (%d energy)",
                                   THE (obj), brainScanCost))
                        {
                            user->loseEnergy (brainScanCost);
                            obj->set (obj::known_bugginess);
                            I->p ("%s", THE (obj));
                        }
                    } else if (!obj->is (obj::known_bugginess)) {
                        I->p ("You lack %d energy to scan %s closely.",
                              brainScanCost, THE (obj));
                    }
                    /* Continue scanning. */
                }
            }
            if (scans)
                I->p ("%d object%s scanned.", scans, scans > 1 ? "s" : "");
            if (scans == 1)  return HALFTURN;
            if (scans > 1)   return FULLTURN;
            I->drawSideWin (user);  /* Refresh energy used. */
        }
        I->p ("You detect nothing of interest.");
        return 0;
    default:
        if (!Level->moveForward (dir, &x, &y)) {
            return 0;
        }
        if (user->countEnergy () < quickScanCost) {
            I->p ("You need %d energy for %s.", quickScanCost, YOUR (tricorder));
            return 0;
        }
        f = Level->getFeature (x, y);
        c = Level->getCreature (x, y);
        if (!c and user->intr (kBlind)) {
            user->loseEnergy (quickScanCost);
            I->p ("You detect no life signs there.");
            I->drawSideWin (user);
            return 0;
        } else if (c) {
            const char *desc;
            if (user->intr (kBlind)) {
                desc = creatureTypeName[c->myIlk ()->mType];
            } else {
                desc = c->myIlk ()->mName;
            }
            c->mHidden = 0;

            user->loseEnergy (quickScanCost);
            I->p ("%s HP:%d/%d AC: %d", desc, c->mHP, c->mMaxHP, c->getAC ());
            I->drawSideWin (user);
            /* Creature diagnosed must stay still. */
            if (c->isHero () or c->isPet () or c->feat (kSessile) or
                c->isHelpless () or c->isTrapped ())
            {
                bool wasbugknown = tricorder->is (obj::known_bugginess);
                tricorder->set (obj::known_bugginess);
                /* Tell player if most other conditions are fulfilled. */
                if (tricorder->isDebugged ()) {
                    if (!wasbugknown) {
                        I->p ("Optimize your tricorder to perform full diangostic scans.");
                    }
                } else if (user->countEnergy () < diagnosisCost) {
                    I->p ("You need %d energy to perform full scan.", diagnosisCost);
                } else if (I->yn ("Perform full diagnostic scan?")) {
                    user->loseEnergy (diagnosisCost);
                    /* Shopkeepers charge only for using diagnosis.  On the other
                       hand it is cheaper than docbot service so may be worth it. */
                    user->employedItem (tricorder, 80);
                    I->drawSideWin (user);
                    /* Diagnosis screen would obscure this information. */
                    I->pause ();
                    c->doDiagnostics (tricorder->mBugginess);
                    return LONGTURN;
                }
            }
        } else if (f and f->mType == shFeature::kVat) {
            return scanVat (user, f, tricorder);
        } else {
            I->p (nothing);
        }
    } /* Takes no time to use to be useful in combat. */
    return 0;
}


static int
useDroidCaller (shCreature *user, shObject *obj)
{
    shVector <shCreature *> clist;

    if (Level->isInGarbageCompactor (user->mX, user->mY) and
        !obj->isBuggy () and Level->mCompactorState < 10 and
        Level->mCompactorState > 0)
    {
        for (int i = 0; i < Hero.mPets.count (); ++i) {
            shCreature *pet = Hero.mPets.get (i);
            /* R2D2!  Turn off the garbage compactor at the sewer plant! */
            if (pet->isA (kMonAstromechDroid)) {
                I->p ("You call upon your astromech droid for help.");
                if (user->tryToTranslate (pet)) {
                    if (Level->mCompactorHacked == 0) {
                        I->p ("The R2 unit on %s %d acknowledges your request.",
                              pet->mLevel->mName, pet->mLevel->mDLevel);
                    } else {
                        I->p ("The R2 unit on %s %d is working on it.",
                              pet->mLevel->mName, pet->mLevel->mDLevel);
                    }
                } else {
                    I->p ("You receive a torrent of beeps and chirps"
                          " in response.");
                }
                Level->mCompactorHacked = 1; /* Hero is safe. */
                return HALFTURN;
            }
        }
    }
    I->p ("%s produces a strange whistling sound.", THE (obj));

    for (int i = 0; i < Level->mCrList.count (); ++i) {
        shCreature *c = Level->mCrList.get (i);
        if (   (obj->isBuggy () and c->isRobot ())
            or (!obj->isBuggy () and c->isPet ()))
        {
            int x = user->mX, y = user->mY;
            if (!obj->isOptimized ()) {
                x = x + RNG(7) - 3;
                y = y + RNG(5) - 2;
            }

            if (!Level->findNearbyUnoccupiedSquare (&x, &y)) {
                c->transport (x, y);
                if (user->canSee (c) and !c->isPet ()) {
                    obj->set (obj::known_bugginess);
                }
                if (c->isA (kMonClerkbot) or
                    c->isA (kMonGuardbot) or
                    c->isA (kMonSecuritron) or
                    c->isA (kMonDocbot) or
                    c->isA (kMonWarbot))
                {
                    clist.add (c);
                }
            }
        }
    }
    if (!obj->is (obj::known_type) or !obj->is (obj::known_appearance)) {
        obj->set (obj::known_type);
        obj->announce ();
    }
    I->drawScreen ();
    for (int i = 0; i < clist.count (); ++i)
        clist.get (i)->newEnemy (user);

    return HALFTURN;
}


static int
useRemote (shCreature *user, shObject *remote)
{
    const int elapsed = FULLTURN;
    if (!user->isHero ())  return elapsed;

    if (!remote->is (obj::known_type)) {
        I->p ("It appears to be a remote control.");
        remote->set (obj::known_type);
    }

    /* Remote control's range is dependent on its state. */
    const int range_by_bugginess[] = {3, 7, 9, 40};
    int x = -1, y = -1;
    int actual_range = range_by_bugginess[remote->mBugginess + 1];
    int range = remote->is (obj::known_bugginess) ? actual_range : 99;

    /* Pick target. */
    if (!I->getSquare ("Send signal to what?  (select a location)",
            &x, &y, range))
    {
        I->nevermind ();
        return 0;
    }

    shCreature *c = user->mLevel->getCreature (x, y);
    if (!c) {
        I->p ("Nothing happens.");
        return elapsed;
    }

    /* Is target eligible? */
    static const shMonId triggerable[] = {
        kMonKamikazeGoblin,
        kMonSmartBomb,
        kMonSmartMissile,
        kMonSpiderMine
    };
    const int num_trig_mon = sizeof (triggerable) / sizeof (shMonId);

    bool will_respond = false;
    for (int i = 0; i < num_trig_mon; ++i) {
        if (c->isA (triggerable[i])) {
            will_respond = true;
            break;
        }
    }

    bool is_in_range = distance (user->mX, user->mY, x, y) <= actual_range * 5;
    if (!will_respond or !is_in_range) {
        I->p ("%s reports no feedback.", YOUR (remote));
        return elapsed;
    }

    /* Is the bot owned or the remote overridden? */
    will_respond = c->isPet () or remote->is (obj::cracked);
    if (!will_respond) {
        I->p ("%s reports signal was rejected.");
        return elapsed;
    }

    bool see = user->canSee (c);
    /* Issue go-boom order! */
    for (int i = 0; i < MAXATTACKS; ++i) {
        shAttack &atk = Attacks[c->myIlk ()->mAttacks[i].mAttId];
        if (atk.mType == shAttack::kExplode) {
            c->die (kSlain, user, remote, &atk);
            break;
        }
    }
    if (!see) {
        I->p ("%s reports signal was accepted.", YOUR (remote));
    }

    return elapsed;
}


static int
useGrenade (shCreature *user, shObject *grenade)   /* That means set it off. */
{
    I->p ("You push the button.");
    if (grenade->mIlkId == kObjFlare) {
        if (user->intr (kBlind))
            I->p ("%s becomes very warm.", grenade->your (1));
        /* Else grenade explosion message will handle this. */
        grenade->set (obj::known_type);
    }
    Level->attackEffect (grenade->myIlk ()->mMissileAttack, grenade,
            user->mX, user->mY, kOrigin, user, grenade->mEnhancement);
    user->useUpOneObjectFromInventory (grenade);
    return FULLTURN;
}


static int
useProximityMine (shCreature *user, shObject *obj)
{
    int x = user->mX, y = user->mY;

    if (!obj->is (obj::known_type)) {
        I->p ("You push the button.");
        obj->set (obj::known_type);
        obj->announce ();
        I->pause ();
    }
    shDirection dir = I->getDirection ();
    switch (dir) {
    case kNoDirection:
        return 0;
    case kUp:
        I->p ("You can't reach the ceiling.");
        return 0;
    case kDown:
        I->p ("Move away from this spot to place it here.");
        return 0;
    default:
        if (!Level->moveForward (dir, &x, &y)) {
            return 0;
        }
        if (Level->getCreature (x, y)) {
            I->p ("There is something standing there already.");
            return 0;
        }
        if (Level->isObstacle (x, y)) {
            I->p ("There is not enough space to place it.");
            return 0;
        }
        shCreature *mine = shCreature::monster (kMonSmartBomb);
        mine->mConditions |= kNoExp;
        if (!obj->isBuggy ()) {
            mine->makePet ();
            obj->set (obj::known_bugginess);  /* Rest in stack known. */
        }
        I->p ("You prime %s.", obj->your (1));
        user->useUpOneObjectFromInventory (obj);
        Level->putCreature (mine, x, y);
        Level->checkTraps (x, y, 100);
    }
    return LONGTURN;
}

/* returns ms elapsed */
static int
usePortableHole (shCreature *user, shObject *obj)
{
    int x = user->mX, y = user->mY;
    shFeature *f;
    shCreature *c;

    shDirection dir = I->getDirection ();
    switch (dir) {
    case kNoDirection:
        return 0;
    case kUp:
        I->p ("You can't reach the ceiling.");
        return 0;
    case kDown:
    default:
        if (!Level->moveForward (dir, &x, &y)) {
            return 0;
        }
        f = Level->getFeature (x, y);
        if (!Level->isFloor (x, y)) {
            if (f) {
                I->p ("But there is a portable hole already.");
                return 0;
            }
            int instant = 0, nx = x, ny = y;
            shTerrainType middle = kVoid;
            /* For low depth hole activates automatically. */
            /* Patterns .#. .##. .# #. only. Pattern ### is not permitted. */
            for (int i = 0; i < 3; ++i) {
                if (!Level->moveForward (dir, &nx, &ny))
                    break;
                if (i == 0)
                    middle = Level->getSquare (nx, ny)->mTerr;
                if (Level->isFloor (nx, ny)) {
                    instant = (i < 2) or (middle == kStone);
                    break;
                }
            }
            user->useUpOneObjectFromInventory (obj);
            if (instant) { /* Dig immediately. */
                I->p ("As you lay it on %s it immediately activates.",
                    THE (Level->getSquare (x, y)));
                while (!Level->isFloor (x, y)) {
                    Level->dig (x, y);
                    Level->moveForward (dir, &x, &y);
                } /* Check only for hero position. */
                if (user->isInShop ()) {
                    user->damagedShop (user->mX, user->mY);
                }
                break;
            }
            shFeature *f = new shFeature ();
            f->mType = shFeature::kPortableHole;
            f->mX = x;
            f->mY = y;
            Level->addFeature (f);
            I->p ("You lay it on %s.", THE (Level->getSquare (x, y)));
            /* Laying it from outside is okay. Doing it from inside is not. */
            if (user->isInShop ()) {
                user->damagedShop (user->mX, user->mY);
            }
            break;
        }
        if (f) {
            /* It is scummable to detect traps for free otherwise. */
            I->p ("It failed to activate properly %s.  It is wasted.",
                  x == user->mX and y == user->mY ? "here" : "there");
            user->useUpOneObjectFromInventory (obj);
            return 0;
        }
        user->useUpOneObjectFromInventory (obj);
        Level->addTrap (x, y, shFeature::kHole);
        if (Level->isInShop (x, y)) {
            user->damagedShop (x, y);
        }
        c = Level->getCreature (x, y);
        if (c and !c->isHero ()) {
            c->newEnemy (user);
        }
        Level->checkTraps (x, y, 100);
    }
    return FULLTURN;
}

/* returns ms elapsed */
int
useOnOffTool (shCreature *user, shObject *tool)
{
    if (tool->is (obj::active)) {
        I->p ("You turn off %s.", YOUR (tool));
        tool->clear (obj::active);
        if (tool->isA (kObjGeigerCounter) and !tool->isBuggy () and
            Hero.getStoryFlag ("geiger counter message"))
        {
            I->p ("It stops clicking.");
        }
    } else if (user->countEnergy () <= 0 and tool->myIlk ()->mEnergyUse) {
        I->p ("You're out of juice!");
    } else {
        I->p ("You turn on %s.", YOUR (tool));
        tool->set (obj::active);
        if (tool->isA (kObjGeigerCounter) and !tool->isBuggy () and
            Hero.getStoryFlag ("geiger counter message"))
        {
            if (!tool->is (obj::known_type)) {
                I->p ("It starts clicking.  It must be a Geiger counter.");
                tool->set (obj::known_type);
            } else {
                I->p ("It starts clicking.");
            }
        }
    }
    user->computeIntrinsics ();
    return HALFTURN;
}


static int
useEnergyTank (shCreature *user, shObject *tank)
{
    int capacity = tank->myIlk ()->mMaxCharges;
    shObject *cells = NULL;
    if (tank->mCharges == capacity) {
        I->p ("%s is full already.", YOUR (tank));
        return 0;
    }
    for (int i = 0; i < user->mInventory->count (); ++i) {
        cells = user->mInventory->get (i);
        if (cells->mIlkId == kObjEnergyCell) break;
    }
    if (!cells or cells->mIlkId != kObjEnergyCell) {
        I->p ("You have no energy cells to discharge into %s.", tank->theQuick ());
        return 0;
    }
    int eat = mini (cells->mCount, capacity - tank->mCharges);
    user->useUpSomeObjectsFromInventory (cells, eat);
    tank->mCharges += eat;
    bool full = tank->mCharges == capacity;
    I->p ("%s absorbs energy of %d energy cell%s%s.", YOUR (tank),
          eat, eat > 1 ? "s" : "", full ? " and is now full" : "");
    return FULLTURN;
}


static int
useKhaydarinCrystal (shCreature *user, shObject *crystal)
{
    int worked = 0;
    if (!crystal->mCharges) {
        I->p ("Nothing happens.");
        int elapsed = crystal->is (obj::known_charges) ? 0 : 200;
        crystal->set (obj::known_charges);
        return elapsed;
    }
    if (user->mPsionicStormFatigue) {
        --user->mPsionicStormFatigue;
    }
    if (user->mAbil.harm (abil::Psi) or user->mAbil.temp (abil::Psi) < 0) {
        I->p ("You focus your mental powers.");
        int tmp = user->mAbil.temp (abil::Psi);
        int loss = user->mAbil.harm (abil::Psi);
        if (tmp < 0)
            user->mAbil.temp_mod (abil::Psi, -tmp);
        if (loss)
            user->mAbil.restore (abil::Psi, loss);
        ++worked;
    }
    if (user->is (kConfused) or user->is (kStunned)) {
        I->p ("Your thoughts become crystal clear.");
        user->cure (kConfused);
        user->cure (kStunned);
        ++worked;
    }
    if (!worked) {
        I->p ("%s glows faintly.", YOUR (crystal));
    }
    --crystal->mCharges;
    /* Using up all charges takes away telepathy. */
    if (!crystal->mCharges)
        user->computeIntrinsics ();
    user->computeSkills (); /* Charges give bonus to concentration. */
    return HALFTURN;
}

#if 0
static int
useLicense (shCreature *user, shObject *license)
{
    const char *interrogate[] =
    {
        "Orgasmatron?!  What are you talking about?",
        "The Orgasmatron is rumored to be somewhere in the Mainframe.",
        "There is more than one Orgasmatron but only one true Orgasmatron.",
        "I heard someone left an Orgasmatron in a garbage compactor.",
        "Follow the white rabbit.  It will lead you to the Orgasmatron.",
        "Entrance to the Mainframe is protected by a door with retina scanner.",
        "The key to retina scanner is held by the Bastard Operator From Hell.",
        "You can find the Operator at the bottom of Gamma Caves.",
        "You most probably will need to visit Gamma Caves.",
        "Some say that a skilled burglar can bypass retina scanner door.",
        "Need protection against radiation?  Search the waste treatment plant.",
        "I am afraid you may need to confront Shodan about it."
    };
    const int numblurbs = sizeof (interrogate) / sizeof (char *);
    shDirection dir = I->getDirection ();
    switch (dir) {
        case kNoDirection: case kUp: case kDown:
            I->p ("No idea what to do with %s there.", YOUR (license));
            return 0;
        case kOrigin: {
            char *buf = GetBuf ();
            strcpy (buf, Hero.mName);
            buf[0] = toupper (buf[0]);
            if (!Hero.intr (kBlind)) {
                /* What if it is generated randomly? */
                I->p ("It reads: \"Detective %s\".", buf);
            } else {
                I->p ("You cannot read while blind.");
            }
            return 0;
        }
        default: {
            int x = Hero.mX, y = Hero.mY;
            if (!Level->moveForward (dir, &x, &y)) {
                return 0;
            }
            shCreature *c = Level->getCreature (x, y);
            shFeature *f = Level->getFeature (x, y);
            if (c) {
                if (c->intr (kBlind)) {
                    I->p ("%s seems to be blind.", THE (c));
                } else if (c->is (kAsleep)) {
                    I->p ("%s snores loudly.", THE (c));
                } else if (c->isA (kMonGuardbot) and !c->isHostile ()) {
                    I->p ("You present %s to %s.", YOUR (license), THE (c));
                    shMonster *guard = (shMonster *) c;
                    if (guard->mGuard.mToll == 0) {
                        I->p ("%s ignores you.", THE (guard));
                    } else {
                        if (license->isBuggy ()) {
                            if (Hero.tryToTranslate (guard)) {
                                I->p ("\"Your license has expired.  You must pay like everyone else.\"");
                            } else {
                                I->p ("%s whirs disappointingly.", THE (guard));
                            }
                        } else {
                            if (Hero.tryToTranslate (guard)) {
                                I->p ("\"You may pass.\"");
                            } else {
                                I->p ("%s beeps calmly.", THE (guard));
                            }
                            guard->mGuard.mToll = 0;
                        }
                    }
                } else if (c->isA (kMonLawyer) and c->isHostile ()) {
                    int bribe = 200 + RNG (101);
                    if (I->yn ("%s nods with understanding.  "
                               "Pay $%d to buy cooperation?",
                               THE (c), bribe))
                    {
                        if (Hero.countMoney () >= bribe) {
                            I->p ("%s takes your money.", THE (c));
                            Hero.loseMoney (bribe);
                            c->gainMoney (bribe);
                            ((shMonster *) c) ->mDisposition =
                                shMonster::kIndifferent;
                        } else {
                            I->p ("You don't have that much money.  %s disregards your license.", THE (c));
                        }
                    }
                } else if (c->mType != kHumanoid and c->mType != kMutant and
                           c->mType != kOutsider and c->mType != kCyborg)
                {
                    I->p ("%s seems strangely uncooperative.", THE (c));
                } else if (!c->isHostile () or c->is (kParalyzed)) {
                    I->pageLog ();
                    I->p ("You present %s to %s.", YOUR (license), THE (c));
                    I->p ("You interrogate about the Bizarro Orgasmatron.");
                    /* This could be replaced by get random fortune message. */
                    int reply = c->mMaxHP % numblurbs;
                    if (Hero.tryToTranslate (c)) {
                        I->p ("\"%s\"", interrogate[reply]);
                    } else {
                        I->p ("You fail to understand the reply.");
                    }
                } else {
                    I->p ("%s seems more interested in killing you.", THE (c));
                }
            } else if (f and f->isDoor ()) {
                return Hero.useKey (license, f);
            } else {
                I->p ("No idea what to do with %s there.", YOUR (license));
                return 0;
            }
        }
    }
    return FULLTURN;
}
#endif

static int
swallowBluePill (shCreature *user, shObject *pill)
{
    I->p ("You swallow the pill.");
    if (Level->isMainframe ()) return 0;
    user->useUpOneObjectFromInventory (pill);
    for (int i = 1; i < Maze.count (); ++i) {
        shMapLevel *L = Maze.get (i);
        if (L->isMainframe ()) {
            Level->warpCreature (user, L);
            break;
        }
    }
    return 0;
}

static int
swallowRedPill (shCreature *user, shObject *pill)
{
    I->p ("You swallow the pill.");
    if (!Level->isMainframe ()) return 0;
    user->useUpOneObjectFromInventory (pill);
    for (int i = 1; i < Maze.count (); ++i) {
        shMapLevel *L = Maze.get (i);
        if (L->mType == shMapLevel::kRabbit) {
            Level->warpCreature (user, L);
            break;
        }
    }
    return 0;
}

static int
useTransferCable (shCreature *user, shObject *cable)
{
    const int elapsed = FULLTURN;
    if (user->isUnderwater ()) {
        I->p ("This transfer cable is not waterproof.");
        return 0;
    }
    shObjectVector v;
    /* Find all energy containers hero knows of. */
    for (int i = 0; i < user->mInventory->count (); ++i) {
        shObject *obj = user->mInventory->get (i);
        if (obj->has_subtype (energy_tank) or
           (obj->is (obj::known_type) and
            (obj->isA (kObjEnergyBelt) or
             obj->isA (kObjTeslaSuit) or obj->isA (kObjShockCapacitor))))
            v.add (obj);
    }
    /* Pick source and destination. */
    shObject *src = user->quickPickItem (&v, "transfer from", 0);
    if (!src)  return 0;
    v.remove (src);
    /* Suit of tesla armor and shock capacitor can be only discharged. */
    if (src->isA (kObjTeslaSuit) or src->isA (kObjShockCapacitor)) {
        if (!src->mCharges) {
            I->p ("%s is already discharged.", YOUR (src));
            return 0;
        }
        if (I->yn ("Do you want to ground out %s?", YOUR (src))) {
            bool doit = true;
            shFeature *f = user->mLevel->getFeature (user->mX, user->mY);
            if (user->mZ == 1) {
                I->p ("You link %s to the ceiling.", YOUR (src));
            } else if (user->mZ == -1 and f) {
                I->p ("You link %s to %s wall.", YOUR (src), THE (f));
            } else if (user->mZ == 0 and user->intr (kFlying) and f and f->isPit ()) {
                doit = false;
                I->p ("You can neither touch ground nor ceiling at the moment.");
            } else {
                I->p ("You link %s to %s.", YOUR (src),
                      user->mLevel->getSquare (user->mX, user->mY)->the ());
            }
            if (doit) {
                src->mCharges = 0;
                return elapsed;
            }
        }
        return 0;
    }
    shObject *dest = user->quickPickItem (&v, "transfer to", 0);
    if (!dest)  return 0;
    /* As above. */
    if (dest->isA (kObjTeslaSuit) or dest->isA (kObjShockCapacitor)) {
        I->p ("%s cannot be recharged directly.", YOUR (dest));
        return 0;
    }
    /* Move the energy, although not without a cost. */
    int space = dest->myIlk ()->mMaxCharges - dest->mCharges;
    int transfer = 0;
    if (space > src->mCharges) {
        transfer = src->mCharges;
        src->mCharges = 0;
    } else {
        transfer = space;
        src->mCharges -= transfer;
    }
    int loss = cable->isOptimized () ? transfer / 100 : /* 1% */
               cable->isDebugged () ? transfer * 3 / 100 : /* 3% */
               transfer / 10; /* 10% */
    if (loss > src->mCharges) {
        transfer -= (loss - src->mCharges);
        src->mCharges = 0;
    } else {
        src->mCharges -= loss;
    }
    if (!transfer) {
        I->nevermind ();
        return 0;
    }

    dest->mCharges += transfer;
    int action = dest->is (obj::unpaid) - src->is (obj::unpaid);
    const char *verb =
        action <= -1 ? "steal" : action >= +1 ? "donate" : "transfer";
    if (action <= -1 and Hero.mProfession == Ninja)  verb = "ninja'd";
    I->p ("You %s energy of %d cell%s.", verb, transfer + loss,
        transfer + loss > 1 ? "s" : "");
    if (loss)
        I->p ("Energy of %d cell%s was lost in the process.", loss,
            loss > 1 ? "s" : "");
    return elapsed;
}

static int
igniteLightsaber (shCreature *user, shObject *saber)
{
    if (saber->is (obj::known_type)) {
        I->p ("Wield this %s to fight with it.", saber->getShortDescription ());
        return 0;
    }  /* Applying unknown lightsaber activates it. */
    saber->set (obj::known_type);
    user->reorganizeInventory ();
    const char *color = "";
    if (saber->is (obj::known_appearance)) { /* You now see blade color. */
        char *buf = GetBuf ();
        strcpy (buf, saber->getShortDescription ());
        strchr (buf, ' ')[1] = 0; /* Cut description after space. */
        color = buf;
    }
    I->p ("Kzzzcht!  The flashlight produces %senergy blade!", color);
    if (RNG (2)) { /* You held the wrong way. */
        I->p ("Ooowww!!  You happened to hold it the wrong way.");
        if (user->sufferDamage (saber->myIlk ()->mMeleeAttack)) {
            user->die (kKilled, "igniting a lightsaber while holding it the wrong way");
        }
    } else {
        I->p ("Luckily you held it outwards.");
    }
    return FULLTURN;
}

static int
switchMaskMode (shCreature *user, shObject *mask)
{
    if (mask->is (obj::toggled)) {
        I->p ("You switch %s to thermal vision mode.", YOUR (mask));
        mask->clear (obj::toggled);
    } else {
        I->p ("You switch %s to EM field vision mode.", YOUR (mask));
        mask->set (obj::toggled);
    }
    user->computeIntrinsics ();
    return HALFTURN;
}

static int
bribeSomeone (shCreature *user, shObject *)
{
    shDirection dir = I->getDirection ();
    switch (dir) {
        case kNoDirection: case kUp: case kDown:
            return 0;
        case kOrigin:
            I->p ("You count your money.  You got $%d.", user->countMoney ());
            return 0;
        default: {
            int x = user->mX, y = user->mY;
            shCreature *c = NULL;
            if (!Level->moveForward (dir, &x, &y) or
                NULL == (c = Level->getCreature (x, y)))
            {
                I->p ("There is nobody to pay.");
                return 0;
            }
            user->bribe (c);
            return FULLTURN;
        }
    }
}

static int
useBizarroOrgasmatron (shCreature *user, shObject *obj)
{
    if (!user->isHero ())
        return FULLTURN;

    if (Level->isMainframe ()) {
        I->p ("Indeed, this is the Bizarro Orgasmatron!");
        I->p ("However, to make real use of its power you must escape the Mainframe.");
        obj->set (obj::known_type);
        return FULLTURN;
    }
    I->p ("Hot dog!  You finally got your paws on the Bizarro Orgasmatron!");
    I->p ("You use its awesome power to beam yourself off this two-bit space hulk.");
    I->p ("");

    Hero.earnScore (10000);
    user->die (kWonGame);
    return FULLTURN;
}


static int
useBizarreOrgasmatron (shCreature *user, shObject *obj)
{
    if (!user->isHero ())
        return FULLTURN;

    I->p ("Hot dog!  You finally got your paws on the Bizarre Orgasmatron!");
    I->p ("You use its awesome power to beam yourself off this two-bit space hulk.");
    I->p ("");
    I->pause ();

    if (user->countEnergy () > 15) {
        user->loseEnergy (15);
    }
    I->p ("Hrmm... that's bizarre...");
    obj->set (obj::known_type);
    return FULLTURN;
}


static int
useBizaaroOrgasmatron (shCreature *user, shObject *obj)
{
    if (!user->isHero ())
        return FULLTURN;

    I->p ("You activate the Bizaaro Orgasmatron...");
    I->pause ();
    I->p ("An invisible choir sings, and you are bathed in radiance...");
    I->pause ();
    I->p ("The voice of eit_cyrus booms out:");
    I->setColor (kLime);
    I->p ("\"CoGNRATULATOINSS, n00B!!!\"");
    I->pause ();
    I->p ("\"In retrun  for thy sevrice, I grants thee teh gift of a SPELLCHEECKER!!!\"");
    I->p ("\"HAHAHAHAHOHOHOAHAHAHA!!! !!!!!1\"");
    I->setColor (kGray);

    obj->set (obj::known_type);
    return FULLTURN;
}


static int
useBizzaroOrgasmatron (shCreature *user, shObject *obj)
{
    if (!user->isHero ())
        return FULLTURN;

    I->p ("This makes you feel great!");
    obj->set (obj::known_type);
    return FULLTURN;
}


static int
useBazaaroOrgasmatron (shCreature *user, shObject *obj)
{
    if (!user->isHero ())
        return FULLTURN;

    I->p ("Nothing happens.");
    obj->set (obj::known_type);
    return FULLTURN;
}


static int
useBazarroOrgasmatron (shCreature *user, shObject *obj)
{
    if (!user->isHero ())
        return FULLTURN;

    char *capsname = strdup (user->mName);
    for (char *p = capsname; *p; ++p)
        *p = toupper (*p);

    I->p ("THANK YOU %s!", capsname);
    I->p ("BUT OUR PRINCESS IS IN ANOTHER CASTLE!");
    free (capsname);
    obj->set (obj::known_type);
    return FULLTURN;
}

void
initializeTools ()
{
    AllIlks[kObjMoney].mUseFunc = bribeSomeone;
    AllIlks[kObjBioMask].mUseFunc = switchMaskMode;
    AllIlks[kObjDroidCaller].mUseFunc = useDroidCaller;
    AllIlks[kObjTricorder].mUseFunc = useTricorder;
    for (int i = kObjKeycard1; i <= kObjMasterKeycard; ++i)
        AllIlks[i].mUseFunc = useKeyTool;
    AllIlks[kObjLockPick].mUseFunc = useKeyTool;
    /* AllIlks[kObjLicense].mUseFunc = useLicense; */
    AllIlks[kObjRestrainingBolt].mUseFunc = useRestrainingBolt;
    AllIlks[kObjMedicomp].mUseFunc = useMedicomp;
    for (int i = kObjMiniComputer; i <= kObjSatCom; ++i)
        AllIlks[i].mUseFunc = useComputer;
    AllIlks[kObjPortableHole].mUseFunc = usePortableHole;
    AllIlks[kObjKhaydarin].mUseFunc = useKhaydarinCrystal;
    AllIlks[kObjBluePill].mUseFunc = swallowBluePill;
    AllIlks[kObjRedPill].mUseFunc = swallowRedPill;
    AllIlks[kObjTransferCable].mUseFunc = useTransferCable;
    AllIlks[kObjLightSaber].mUseFunc = igniteLightsaber;
    AllIlks[kObjRemoteControl].mUseFunc = useRemote;

    for (int i = kObjConcussionGrenade; i <= kObjFlare; ++i)
        AllIlks[i].mUseFunc = useGrenade;
    AllIlks[kObjProximityMine].mUseFunc = useProximityMine;

    AllIlks[kObjSmallEnergyTank].mUseFunc = useEnergyTank;
    AllIlks[kObjLargeEnergyTank].mUseFunc = useEnergyTank;
    AllIlks[kObjEnergyBelt].mUseFunc = useEnergyTank;

    AllIlks[kObjDuctTape].mUseFunc = useDuctTape;
    AllIlks[kObjMonkeyWrench].mUseFunc = useMonkeyWrench;
    AllIlks[kObjMechaDendrites].mUseFunc = useMechaDendrites;

    AllIlks[kObjFlashlight].mUseFunc = useOnOffTool;
    AllIlks[kObjBrotherhoodPH].mUseFunc = useOnOffTool;
    AllIlks[kObjDeepBluePA].mUseFunc = useOnOffTool;
    AllIlks[kObjGeigerCounter].mUseFunc = useOnOffTool;
    AllIlks[kObjMotionTracker].mUseFunc = useOnOffTool;
    AllIlks[kObjShieldBelt].mUseFunc = useOnOffTool;
    AllIlks[kObjCloakingBelt].mUseFunc = useOnOffTool;

    AllIlks[kObjTheOrgasmatron].mUseFunc = useBizarroOrgasmatron;
    AllIlks[kObjFakeOrgasmatron1].mUseFunc = useBizarreOrgasmatron;
    AllIlks[kObjFakeOrgasmatron2].mUseFunc = useBizaaroOrgasmatron;
    AllIlks[kObjFakeOrgasmatron3].mUseFunc = useBizzaroOrgasmatron;
    AllIlks[kObjFakeOrgasmatron4].mUseFunc = useBazaaroOrgasmatron;
    AllIlks[kObjFakeOrgasmatron5].mUseFunc = useBazarroOrgasmatron;
}
