#include "Global.h"
#include "Util.h"
#include "Creature.h"
#include "Object.h"
#include "AttackType.h"
#include "Mutant.h"
#include "Hero.h"
#include "Transport.h"
#include <ctype.h>
#include <math.h>

shCreature::shCreature ()
    : mTimeOuts (), mSkills ()
{
    mX = -10;
    mY = -10;
    mZ = 0;
    mLastX = mLastY = 0;
    mName[0] = 0;
    mAP = -1000;
    mCLevel = 0;
    mAC = 0;
    mToHitModifier = 0;
    mDamageModifier = 0;
    mLastRegen = MAXTIME;
    mLastLevel = NULL;
    mReflexSaveBonus = 0;
    mInnateIntrinsics.clear ();
    mFeats = 0;
    mLevel = NULL;
    for (int i = 0; i < kMaxEnergyType; ++i) {
        mInnateResistances[i] = 0;
        mResistances[i] = 0;
    }
    for (int i = 0; i < kMaxAllPower; ++i) {
        mMutantPowers[i] = 0;
    }
    for (int i = 0; i < shObjectIlk::kMaxSite; ++i) {
        mImplants[i] = NULL;
    }
    for (int i = 0; i < TRACKLEN; ++i) {
        mTrack[i].mX = -1;
    }
    mConditions = 0;
    mRad = 0;

    mInventory = new shVector <shObject *> (8);
    mWeight = 0;
    mPsiModifier = 0;

    mSpeed = 100;
    mSpeedBoost = 0;
    mBurdenMod = 0;
    mNaturalArmorBonus = 0;
    mBodyArmor = NULL;
    mJumpsuit = NULL;
    mHelmet = NULL;
    mBoots = NULL;
    mBelt = NULL;
    mGoggles = NULL;
    mCloak = NULL;
    mWeapon = NULL;
    mState = kWaiting;
    mHidden = 0;
    mMimic = kMmNothing;
    mSpotAttempted = 0;
    mTrapped.mInPit = 0;
    mTrapped.mDrowning = 0;
    mTrapped.mWebbed = 0;
    mTrapped.mTaped = 0;
    mImpregnation = 0;
    mDisturbed = false;
    mXP = 0;
    mSkillPoints = 0;

    /* Prevent motion tracker from picking up things it should not. */
    mLastMoveTime = -10000;
}


shCreature::~shCreature ()
{
    delete mInventory;
}

bool
shCreature::isHero ()
{
    return this == Hero.cr ();
}

/* Prejudiced term from WH40k for intelligent non-human races. */
bool
shCreature::isXenos ()
{   /* Elves, Orcs/Orz, VUX, Reticulans, Klingons. */
    return strchr ("EOVgk", mGlyph.mSym);
}

shMonsterIlk *
shCreature::myIlk ()
{
    return &MonIlks[mIlkId];
}

bool
shCreature::isA (shMonId id)
{
    return mIlkId == id;
}

#define RADIATES(_crtype) (_crtype <= kDroid or \
                           (_crtype >= kCyborg and _crtype != kAlien))

bool
shCreature::isAlive ()
{
    return IS_ALIVE (myIlk ()->mType);
}

bool
shCreature::isProgram ()
{
    return myIlk ()->mType == kProgram;
}

bool
shCreature::isRobot ()
{
    return kBot == myIlk ()->mType or kDroid == myIlk ()->mType;
}

bool
shCreature::isInsect ()
{
    return kInsect == myIlk ()->mType;
}

bool
shCreature::isAlien ()
{
    return kAlien == myIlk ()->mType;
}

bool
shCreature::hasMind ()
{
    return HAS_MIND (myIlk ()->mType) or isA (kMonBrainMold);
}

bool
shCreature::radiates ()
{
    return RADIATES (myIlk ()->mType);
}

const char *
shCreature::the ()
{
    if (isHero ())
        return "you";
    shCreature *h = Hero.cr ();
    if (!h->canSee (this) and !h->canHearThoughts (this))
        return hasMind () ? "someone" : "something";
    if (h->is (kXenosHunter) and isXenos ())
        return "the xenos scum";
    if (h->is (kXenosHunter) and isHeretic ())
        return "the heretic";

    char *buf = GetBuf ();
    snprintf (buf, SHBUFLEN, "the %s", myIlk ()->mName);
    return buf;
}

const char *
shCreature::an ()
{
    if (isHero ())
        return "you";
    shCreature *h = Hero.cr ();
    if (!h->canSee (this) and !h->canHearThoughts (this))
        return hasMind () ? "someone" : "something";
    if (feat (kUniqueMonster))
        return the ();
    if (h->is (kXenosHunter) and isXenos ())
        return "a xenos scum";
    if (h->is (kXenosHunter) and isHeretic ())
        return "a heretic";

    char *buf = GetBuf ();
    snprintf (buf, SHBUFLEN,
              isvowel (myIlk ()->mName[0]) ? "an %s" : "a %s",
              myIlk ()->mName);
    return buf;
}

const char *
shCreature::your ()
{
    assert (!isHero ());
    if (Hero.cr ()->intr (kBlind) and !Hero.cr ()->canHearThoughts (this))
        return hasMind () ? "someone" : "something";

    char *buf = GetBuf ();
    snprintf (buf, SHBUFLEN, "your %s", myIlk ()->mName);
    return buf;
}

const char *
shCreature::are ()
{   /* Watch out for all monsters with names in plural. */
    if (isHero () or mIlkId == kMonCreepingCredits)
        return "are";
    return "is";
}

const char *
shCreature::herself ()
{
    if (isHero ())
        return "yourself";
    if (!hasMind ())
        return "itself";
    if (kFemale == mGender)
        return "herself";
    if (kMale == mGender)
        return "himself";

    return "itself";
}

const char *
shCreature::her ()
{
    return isHero () ? "you" :
           !hasMind () ? "its" :
           kFemale == mGender ? "her" :
           kMale   == mGender ? "his" : "its";
}

const char *
shCreature::her (const char *thing)
{
    char *buf = GetBuf ();

    snprintf (buf, SHBUFLEN, "%s %s",
              !hasMind () ? "its" :
              kFemale == mGender ? "her" :
              kMale   == mGender ? "his" : "its", thing);
    return buf;
}

const char *
shCreature::namedher ()
{
    if (isHero ())  return "your";
    char *buf = GetBuf ();
    snprintf (buf, SHBUFLEN, "%s's", myIlk ()->mName);
    return buf;
}

int
shCreature::numHands ()
{
    return myIlk ()->mNumHands;
}


shCreature::TimeOut *
shCreature::getTimeOut (shTimeOutType key)
{
    for (int i = 0; i < mTimeOuts.count (); ++i)
        if (key == mTimeOuts.get (i) -> mKey)
            return mTimeOuts.get (i);
    return NULL;
}


shTime
shCreature::setTimeOut (shTimeOutType key, shTime howlong, int additive)
{
    TimeOut *t = getTimeOut (key);

    if (!t) {
        mTimeOuts.add (new TimeOut (key, howlong + Clock));
        return howlong + Clock;
    }

    if (additive) {
        t->mWhen += howlong;
    } else if (0 == howlong) {
        t->mWhen = 0;
    } else {
        t->mWhen = maxi (howlong + Clock, t->mWhen);
    }
    return t->mWhen;
}

/* Returns true if c has died due to trapping effect. */
static bool
trapEffect (shCreature *c, shFeature *f)
{
    if (f and f->mType == shFeature::kAcidPit) {
        bool resists = c->mResistances[kCorrosive] >
            Attacks[kAttAcidPitBath].mDamage[0].mHigh;
        if (!resists) {
            c->msg ("You are bathing in acid!")
                or !c->feat (kSessile)
                ? c->appear (fmt ("%s splashes in the acid!", THE (c)))
                : c->appear (fmt ("%s is being slowly dissolved by the acid.", THE (c)));
        }
        if (c->sufferDamage (kAttAcidPitBath)) {
            if (!c->isHero () and Hero.cr ()->canSee (c)) {
                c->pDeathMessage (THE (c), kSlain);
            }
            if (c->isHero ()) {
                c->die (kMisc, "Dissolved in acid");
            } else {
                c->die (kSlain, "acid");
            }
            return true;
        }
    }

    if (c->mTrapped.mDrowning) {
        --c->mTrapped.mDrowning;
        if (0 == c->mTrapped.mDrowning) {
            c->appear (fmt ("%s drowns!", THE (c)));
            if (kSewage == c->mLevel->getSquare (c->mX, c->mY)->mTerr) {
                c->die (kDrowned, "in a pool of sewage");
            } else {
                c->die (kDrowned, "in a pool");
            }
            return true;
        } else if (2 == c->mTrapped.mDrowning) {
            c->msg ("You're drowning!");
        } else if (5 == c->mTrapped.mDrowning) {
            c->msg ("You're almost out of air!");
        }
    }
    return false;
}

/* returns non-zero iff the creature dies */
int
shCreature::checkTimeOuts ()
{
    TimeOut *t;
    int donotremove = 0;
    int oldcond = mConditions;

    for (int i = mTimeOuts.count () - 1; i >= 0; --i) {
        t = mTimeOuts.get (i);
        if (t->mWhen <= Clock) {
            /* Keys XRAYVISION and TELEPATHY are only gained through drinking
               canister of spice melange.  Thus values for intrinsic loss must
               reflect those in Canister.cpp:quaffSpiceMelange () routine. */
            if (XRAYVISION == t->mKey) {
                mInnateIntrinsics.mod (kXRayVision, -10);
                computeIntrinsics ();
                if (isHero () and
                    intr (kPerilSensing) and
                    mGoggles->is (obj::toggled))
                {
                    I->p ("You can no longer see through "
                          "your darkened goggles.");
                }
                interrupt ();
            } else if (TELEPATHY == t->mKey) {
                mInnateIntrinsics.mod (kTelepathy, -15);
                computeIntrinsics ();
                interrupt ();
            } else if (STUNNED == t->mKey) {
                mConditions &= ~kStunned;
                if (isHero ()) {
                    I->drawSideWin (this);
                    I->p ("You feel steadier.");
                }
                interrupt ();
            } else if (CAFFEINE == t->mKey) {
                mInnateResistances[kMesmerizing] -= 25;
                if (isHero ()) {
                    I->p ("You feel caffeine effects decline.");
                }
                interrupt ();
            } else if (CONFUSED == t->mKey) {
                mConditions &= ~kConfused;
                if (isHero ()) {
                    I->drawSideWin (this);
                    I->p ("You feel less confused.");
                }
                interrupt ();
            } else if (HOSED == t->mKey) {
                mConditions &= ~kHosed;
                computeIntrinsics ();
                if (isHero ()) {
                    I->drawSideWin (this);
                    I->p ("You are no longer hosed.");
                }
                interrupt ();
            } else if (PARALYZED == t->mKey) {
                mConditions &= ~kParalyzed;
                if (isHero ()) {
                    I->drawSideWin (this);
                    I->p ("You can move again.");
                }
                interrupt ();
            } else if (FLEEING == t->mKey) {
                mConditions &= ~kFleeing;
            } else if (FRIGHTENED == t->mKey) {
                mConditions &= ~kFrightened;
                if (isHero ()) {
                    I->drawSideWin (this);
                    if (oldcond & kFrightened)
                        I->p ("You regain your courage.");
                }
                interrupt ();
            } else if (FROZEN == t->mKey) {
                mConditions &= ~kFrozen;
                if (Hero.cr ()->canSee (this)) {
                    I->p ("Ice block holding %s melts down.", the ());
                }
            } else if (BLINDED == t->mKey) {
                mInnateIntrinsics.set (kBlind, false);
                computeIntrinsics ();
                if (isHero () and !intr (kBlind)) {
                    I->drawSideWin (this);
                    I->p ("You can see again.");
                }
                interrupt ();
            } else if (VIOLATED == t->mKey) {
                mConditions &= ~kViolated;
                if (isHero ()) {
                    I->drawSideWin (this);
                    I->p ("You don't feel so sore anymore.");
                }
                computeIntrinsics ();
                interrupt ();
            } else if (PLAGUED == t->mKey) {
                if (!is (kPlagued)) {
                    /* Hero bought some time. Now it ends. */
                    if (isHero ()) {
                        I->p ("The ilness is coming back.");
                        interrupt ();
                    }
                    mConditions |= kPlagued;
                    t->mWhen += 3000;
                } else {
                    int dmg = 0, statloss = 0;
                    abil::Index stat = abil::random_index ();
                    if (isHero () and stat == abil::Psi) {
                        /* Hurt might lead to loss of power maintaining. */
                        int upkeep = 0;
                        for (int i = kNoMutantPower; i < kMaxHeroPower; ++i) {
                            if (mMutantPowers[i] > 1) {
                                upkeep += MutantPowers[i].maintainCost ();
                            }
                        }
                        if (mAbil.harm (stat) <
                            (mAbil.base (stat) + upkeep) / 2)
                        {
                            sufferAbilityDamage (stat, 1);
                            statloss = 1;
                        }
                    } else { /* Just hurt. */
                        if (mAbil.harm (stat) < mAbil.base (stat) / 2) {
                            sufferAbilityDamage (stat, 1);
                            statloss = 1;
                        }
                    }
                    if (mHP > mMaxHP / 2) {
                        int diff = mHP - mMaxHP / 2;
                        if (diff > 5) diff = 5;
                        dmg = RNG (1, diff);
                        mHP -= dmg;
                    }
                    if (dmg or statloss) { /* Print ilness message. */
                        if (statloss) {
                            I->p ("Plague tortures your %s.",
                                stat >= abil::Int ? "mind" : "body");
                        } else {
                            I->p ("You feel sick.");
                        }
                        if (statloss) {
                            I->nonlOnce ();
                            char *buf = GetBuf ();
                            strcpy (buf, abil::name (myIlk ()->mType, stat));
                            buf[3] = 0;
                            buf[0] = toupper (buf[0]);
                            I->p ("  (-1 %s)", buf);
                        }
                    }
                    /* This must be cured. */
                    t->mWhen += 5000 + RNG (6) * 1000;
                }
                donotremove = 1;
            } else if (SICKENED == t->mKey) {
                if (sewerSmells ()) {
                    /* keep sick */
                    t->mWhen += FULLTURN;
                    donotremove = 1;
                } else {
                    interrupt ();
                    mConditions &= ~kSickened;
                }
            } else if (SPEEDY == t->mKey) {
                mConditions &= ~kSpeedy;
                if (isHero ()) {
                    I->p ("You slow down.");
                }
                computeIntrinsics ();
                interrupt ();
            } else if (SLOWED == t->mKey) {
                mConditions &= ~kSlowed;
                if (isHero ()) {
                    I->p ("You speed up.");
                }
                computeIntrinsics ();
                interrupt ();
            } else if (ASLEEP == t->mKey) {
                if (is (kAsleep)) {
                    mConditions &= ~kAsleep;
                    if (isHero ()) {
                        I->drawSideWin (this);
                        I->p ("You wake up.");
                    }
                }
                interrupt ();
            } else if (VOICE_NAMSHUB == t->mKey) {
                mConditions &= ~kUnableCompute;
                if (isHero ()) {
                    I->p ("The command not to use the Nam-Shub has weakened.");
                }
                computeIntrinsics ();
                interrupt ();
            } else if (VOICE_MUTANT == t->mKey) {
                mConditions &= ~kUnableUsePsi;
                if (isHero ()) {
                    I->p ("You overcome distaste towards your mutant nature.");
                }
                computeIntrinsics ();
                interrupt ();
            } else if (VOICE_VIOLENCE == t->mKey) {
                mConditions &= ~kUnableAttack;
                if (isHero ()) {
                    I->p ("You again will fight when it is necessary.");
                }
                computeIntrinsics ();
                interrupt ();
            } else if (VOICE_GENEROSITY == t->mKey) {
                /* Silently remove this timer.  The effect is permanent. */
            } else if (TRAPPED == t->mKey) {
                if (isTrapped ()) {
                    trapEffect (this, mLevel->getFeature (mX, mY));
                    t->mWhen += FULLTURN;
                    ++i;
                    donotremove = 1;
                }
            }

            if (!donotremove) {
                mTimeOuts.remove (t);
                delete t;
            }
        }
    }
    return 0;
}


/* Relies on an innate intrinsic instead of condition. Not good ... */
void
shCreature::makeBlinded (int howlong)
{
    mInnateIntrinsics.set (kBlind, true);
    computeIntrinsics ();
    setTimeOut (BLINDED, howlong);
}


int
isAdditiveTimeOut (shTimeOutType tm_out)
{
    switch (tm_out) {
    case ASLEEP: case FLEEING: case FRIGHTENED: case PARALYZED: case VIOLATED:
        return 0;
    default:
        return 1;
    }
}

shTimeOutType
conditionToTimeOut (shCondition cond)
{
    switch (cond) {
    case kAsleep:        return ASLEEP;
    case kConfused:      return CONFUSED;
    case kFleeing:       return FLEEING;
    case kFrightened:    return FRIGHTENED;
    case kFrozen:        return FROZEN;
    case kGenerous:      return VOICE_GENEROSITY;
    case kHosed:         return HOSED;
    case kParalyzed:     return PARALYZED;
    case kPlagued:       return PLAGUED;
    case kSickened:      return SICKENED;
    case kSlowed:        return SLOWED;
    case kSpeedy:        return SPEEDY;
    case kStunned:       return STUNNED;
    case kUnableCompute: return VOICE_NAMSHUB;
    case kUnableUsePsi:  return VOICE_MUTANT;
    case kUnableAttack:  return VOICE_VIOLENCE;
    case kViolated:      return VIOLATED;
    default:             return NOTIMEOUT;
    }
}

bool
shCreature::isHelpless (void)
{
    return mConditions & (kAsleep | kFrozen | kParalyzed);
}

int
shCreature::is (shCondition cond)
{
    return mConditions & cond;
}

void
shCreature::inflict (shCondition cond, int howlong)
{
    shTimeOutType tm_out = conditionToTimeOut (cond);
    assert (tm_out != NOTIMEOUT);

    switch (cond) {
    case kSpeedy:
        if (is (kSpeedy)) {
            howlong /= 2;
            msg ("You feel a bit speedier.");
        } else {
            appear (fmt ("%s speeds up!", the ()))
            or msg ("You feel speedy!");
        }
        break;
    case kSlowed:
        if (is (kSlowed)) {
            howlong /= 2;
            msg ("You feel a bit slower.");
        } else {
            appear (fmt ("%s slows down.", the ()))
            or msg ("You feel sluggish...");
        }
        break;
    case kHosed:
        if (is (kHosed))  howlong /= 2;
        msg ("Your connection is hosed!");
        break;
    case kParalyzed:
        if (is (kParalyzed))  return;
        appear (fmt ("%s can't move!", the ()));
        break;
    case kFrozen:
        if (!is (kFrozen)) {
            shFeature *f = mLevel->getFeature (mX, mY);
            bool pitHere = f and (f->isPit () or f->isHole ());
            /* Are you flying high above ground (depression)?
               Monsters are always considered to be flying high. */
            if (Hero.cr ()->canSee (this))
                I->p ("%s %s encased in ice!", the (), are ());
            if (intr (kFlying) and
                (mZ == 1 or (mZ == 0 and pitHere) or !isHero ()))
            {
                appear (fmt ("%s falls down and shatters on impact!", the ()))
                or msg ("Frozen, you plummet to your death.  "
                        "Your body shatters into thousand pieces!");
                die (kKilled, "shattering into pieces");
                return;
            }
        }
        break;
    case kPlagued:
        if (is (kPlagued)) {
            /* Assure imminent stat degrade. */
            clearTimeOut (PLAGUED);
            setTimeOut (PLAGUED, 100, 0);
            return;
        } else {
            msg ("You feel terribly sick.");
        }
        break;
    case kSickened:
        if (is (kSickened)) {
            msg ("You feel worse.");
            howlong /= 2;
        } else {
            msg ("You feel sick.");
        }
        break;
    case kStunned:
        if (is (kStunned)) {
            msg ("You reel...");
            howlong /= 2;
        } else {
            msg ("You stagger...");
        }
        checkConcentration ();

        if (mWeapon and RNG (2) and
            /* No dropping weapons like power fist. */
            mWeapon->myIlk ()->mMeleeSkill != kUnarmedCombat)
        {
            shObject *obj = mWeapon;
            unwield (obj);
            removeObjectFromInventory (obj);
            if (isHero ())  I->drawSideWin (this);

            msg (fmt ("You drop %s!", YOUR (obj)))
            or appear (fmt ("%s drops %s!", the (), obj->her (this)));
            /* Following line is for monsters that took weapon from shop. */
            if (obj->is (obj::unpaid) and !Level->isInShop (mX, mY)) {
                obj->clear (obj::unpaid);
            }
            Level->putObject (obj, mX, mY);
        }
        break;
    case kViolated:
        if (hasInstalled (kObjExcruciator)) {
            msg ("You barely feel it.");
            return;
        }
        if (is (kAsleep)) {
            msg ("You have an unpleasant dream!");
        } else {
            appear (fmt ("%s looks a little uncomfortable.", the ()))
            or msg ("You feel violated!");
            checkConcentration ();
        }
        break;
    default: break;
    }

    mConditions |= cond;
    computeIntrinsics ();
    setTimeOut (tm_out, howlong, isAdditiveTimeOut (tm_out));
}

void
shCreature::cure (shCondition cond)
{
    shTimeOutType tm_out = conditionToTimeOut (cond);
    assert (tm_out != NOTIMEOUT);
    mConditions &= ~cond;
    clearTimeOut (tm_out);

    switch (cond) {
    case kAsleep:
        if (isHero () and mHP > 0) {
            I->drawSideWin (this);
            I->p ("You wake up.");
        }
        break;
    case kPlagued:
        /* Plague timeout must be found and cut out.  Clearing is not enough. */
        for (int i = mTimeOuts.count () - 1; i >= 0; --i) {
            TimeOut *t = mTimeOuts.get (i);
            if (t->mKey == PLAGUED) {
                mTimeOuts.remove (t);
                delete t;
                break;
            }
        }
        break;
    default: break;
    }
}

bool
shCreature::feat (shFeat feat)
{
    return mFeats & feat;
}

void
shCreature::sterilize ()
{
    mIntrinsics.set (kMultiplier, false);
}

/* Some items modify character abilities. */
const abil::Set *
shObject::getAbilityModifiers ()
{
    static const abil::Set
         /* STR CON AGI DEX INT PSI */
    BRPH = { 0,  0,  0,  0,  0, +1},
    BRPA = {+2,  0, -2,  0,  0,  0},
    PCPA = { 0,  0, +3, -4,  0,  0},
    RCPA = {+3,  0,  0,  0, -4,  0},
    BCPA = { 0,  0, -4,  0,  0, +3},
    GCPA = { 0, +3,  0,  0,  0, -4},
    MBA2 = {+1, +1, +1, +1, +1, +1},
    EBA2 = { 0, +1, +1,  0,  0,  0},
    EBA3 = { 0,  0,  0,  0,  0, +1},
    SBA3 = { 0, +1, +2, +1,  0,  0},
    SBA4 = { 0,  0,  0,  0,  0, +2},
    PSIB = { 0,  0,  0,  0,  0, -2},
    EXCR = { 0,  0, -1, -3, -1,  0};

    switch (mIlkId) {
    case kObjBrotherhoodPH:   return &BRPH;
    case kObjBrotherhoodPA:   return &BRPA;
    case kObjPinkCPA:         return &PCPA;
    case kObjRedCPA:          return &RCPA;
    case kObjBlueCPA:         return &BCPA;
    case kObjGreenCPA:        return &GCPA;
    case kObjMBioArmor2:      return &MBA2;
    case kObjEBioArmor2:      return &EBA2;
    case kObjEBioArmor3:      return &EBA3;
    case kObjSBioArmor3:      return &SBA3;
    case kObjSBioArmor4:      return &SBA4;
    case kObjPsiBlades:       return &PSIB;
    case kObjExcruciator:     return &EXCR;
    default: return NULL;
    }
}


void
shObject::applyConferredResistances (shCreature *target)
{
    if (is (obj::worn)) {
        const abil::Set *a = getAbilityModifiers ();
        if (a) {
            /* Change of abilities gives the item away. */
            set (obj::known_type);
            FOR_ALL_ABILITIES (i)
                target->mAbil._gear.mod (i, a->get (i));
        }
    }
    if (isA (kArmor) and is (obj::worn)) {
        target->mSpeedBoost += myIlk ()->mSpeedBoost;
        for (int i = 0; i < kMaxEnergyType; ++i) {
            target->mResistances[i] += myIlk ()->mResistances[i];
        }
        target->mPsiModifier += getPsiModifier ();
        target->mToHitModifier += myIlk ()->mToHitModifier;
        target->mDamageModifier += myIlk ()->mDamageModifier;
    } else if (isA (kImplant) and is (obj::worn)) {
        target->mPsiModifier += getPsiModifier ();
        if (isA (kObjCerebralCoprocessor)) {
            target->mAbil._gear._int += mEnhancement;
        } else if (isA (kObjMotoricCoordinator)) {
            target->mAbil._gear._dex += mEnhancement;
        } else if (isA (kObjReflexCoordinator)) {
            target->mAbil._gear._agi += mEnhancement;
        } else if (isA (kObjAdrenalineGenerator)) {
            target->mAbil._gear._str += mEnhancement;
        } else if (isA (kObjMyfilamentHypertensor)) {
            target->mAbil._gear._con += mEnhancement;
        } else if (isA (kObjPoisonResistor)) {
            for (int en = kDrnStr; en <= kDrnDex; ++en)
                target->mResistances[en] += 10;
            for (int en = kDmgStr; en <= kDmgDex; ++en)
                target->mResistances[en] += 10;
        } else if (isA (kObjGloryDevice)) {
            target->mFeats |= kExplodes;
        } else if (isA (kObjExcruciator)) {
            target->mSpeedBoost -= 15 - 5 * mBugginess;
        } else if (isA (kObjUngooma)) {
            target->mAbil._gear._int += mBugginess - 1;
        }
    }
}

/* Adds skill bonus unless target does not have the skill at all. */
void
shObject::applyConferredSkills (shCreature *target)
{
    shSkill *s;
    if (isA (kObjKhaydarin) and is (obj::wielded)) {
        s = target->getSkill (kConcentration);
        if (s)  s->mBonus += mCharges;
    } else if (isA (kObjM57Smartgun) and is (obj::wielded)) {
        s = target->getSkill (myIlk ()->mGunSkill);
        int base = 7 + 2 * isOptimized ();
        /* Yes, this hurts when hero is better at heavy guns. */
        if (s)  s->mBonus += base - s->mRanks;
    } else if (isA (kObjM56Smartgun) and is (obj::wielded)) {
        s = target->getSkill (myIlk ()->mGunSkill);
        int base = 3 + 2 * mBugginess;  /* Smaller bonus but reliable. */
        if (s)  s->mBonus += base;
    } else if (isA (kObjStabilizerBelt) and is (obj::worn) and is (obj::known_type)) {
        /* You must be aware of its function to benefit from the belt. */
        s = target->getSkill (kHeavyGun);
        if (s)  s->mBonus += 1 + isOptimized ();
    } else if (has_subtype (skillsoft) and is (obj::worn)) {
        shSkillCode c = myIlk ()->mBoostedSkill;
        if (c == kNoSkillCode) return;
        int bonus = isA (kObjSpotSkillsoft) or isA (kObjLockPickSkillsoft)
        /*  spot skillsoft    buggy:+0  debugged:+3  optimized:+6 */
            ? (mBugginess + 1) * 3
        /* other skillsoft    buggy:+0  debugged:+4  optimized:+8 */
            : (mBugginess + 1) * 4;
        s = target->getSkill (c);
        /* Two skillsoft implants should not add up.  Best value counts. */
        if (s)  s->mBonus = maxi (s->mBonus, bonus);
    } else if (isA (kObjMechaDendrites) and is (obj::worn)) {
        /* Mecha-dendrites are implanted in cerebellum and thus need not
           worry about duplicating skill bonus effect of some other implant
           of same kind.  Bonuses are added without maxi () guard. */
        s = target->getSkill (kRepair);
        /* Grants +1,+3,+5 to repair skill. */
        if (s)  s->mBonus = 4 + 2 * mBugginess;
        /* Grants +0,+1,+2 to programming skill if hero knows any of it. */
        s = target->getSkill (kHacking);
        if (s and s->mRanks) s->mBonus += 1 + mBugginess;
        /* Same for lock pick skill. */
        s = target->getSkill (kOpenLock);
        if (s and s->mRanks) s->mBonus += 1 + mBugginess;
    } else if (isA (kObjSBioArmor4) and is (obj::worn)) {
        for (int i = 0; i < kMaxHeroPower; ++i) {
            s = target->getSkill (kMutantPower, shMutantPower (i));
            if (s)  s->mBonus += maxi (0, 6 - MutantPowers[i].mLevel);
        }
    } else if (isA (kObjTorc) and is (obj::worn) and !isBuggy ()) {
        int boost = 1 + isOptimized ();
        /* Torc amplifies psionic powers better than metabolic ones. */
        for (int i = 0; i < kMaxHeroPower; ++i) {
            s = target->getSkill (kMutantPower, shMutantPower (i));
            if (s) {
                int bonus = boost;
                if (MutantPowers[i].mLevel <= 2)  ++bonus;
                if (MutantPowers[i].mAbility == abil::Con)  --bonus;
                if (MutantPowers[i].mAbility == abil::Int)  bonus = 0;
                s->mBonus += boost;
            }
        }
    }
}

int
shCreature::intr (shIntrinsic in)
{   /* If something blocks the intrinsic dropping it to negative it is
       not present.  The return is zero so in that case because this allows
       to to treat this function as predicate. */
    int res = mIntrinsics.get (in);
    return res < 0 ? 0 : res;
}

bool
shCreature::hasCamouflage ()
{   /* Light gives you away. */
    if (intr (kLightSource))
        return false;
    /* Smaller worms hide well in sludge. */
    if (myIlk ()->mType == kVermin and myIlk ()->mSize <= kSmall and
        Level->getSquare (mX, mY)->mTerr == kSewage)
            return true;
    if (!intr (kCamouflaged))
        return false;
    /* Darkness is good cover. */
    if (!mLevel->isLit (mX, mY, mX, mY))  return true;
    /* So are pit walls. */
    if (isInPit ())  return 1;
    /* Test orthogonal spaces. */
    for (int i = 0; i < 4; ++i) {
        int xmod = (i < 2) ? i*2 - 1 : 0;
        int ymod = (i > 1) ? i*2 - 5 : 0;
        if (mLevel->isInBounds (mX + xmod, mY + ymod) and
            /* TODO: Make sure is it isObstacle or isOcclusive? */
            mLevel->isObstacle (mX + xmod, mY + ymod))
            return true; /* One wall is enough. */
    }
    return false;
}

int
shCreature::canSee (int x, int y)
{
    if (intr (kBlind))  return 0;

    if (isHero ()) {
        if (areAdjacent (x, y, mX, mY)) {
            return 100;
        }
        bool lit = mLevel->isLit (x, y, mX, mY);
        if (mLevel->isInLOS (x, y)) {
            if (lit)
                return 100;
            int light = intr (kLightSource);
            if (light and distance (this, x, y) <= light * 5 + 2)
                return 100;
        }
        if (lit and seeByXRay (x, y))
            return 100;
    } else {
        return 100;
    }
    return 0;
}

int
shCreature::canSee (shCreature *c)
{
    if (intr (kBlind))
        return (c->mX == mX and c->mY == mY) ? 100 : 0;

    bool invis = c->intr (kInvisible);
    if (c->isHero ()) {
        /* Can always "see" self even if blind. */
        if (isHero ())  return 100;
        /* Invisible heroes have 50% to be sensed when adjacent. */
        if (mDisturbed and areAdjacent (mX, mY, c->mX, c->mY))
            return invis ? RNG (2) * 100 : 100;
        if (invis)  return 0;
        /* Naked eye sight test. */
        if (Level->isInLOS (mX, mY) and
             (!c->hasCamouflage () or mDisturbed))
        {
            return 100;
        }
        /* See through walls. */
        if  (seeByXRay (c->mX, c->mY))
            return 100;
    } else if (isHero ()) {
        if (invis)
            return 0;
        /* You always see everyone next to you unless invisible. */
        if (areAdjacent (mX, mY, c->mX, c->mY))
            return 100;
        /* Naked eye. */
        bool lit = mLevel->isLit (c->mX, c->mY, mX, mY);
        int dist = distance (this, c->mX, c->mY);
        if (mLevel->isInLOS (c->mX, c->mY)) {
            /* Lit place. */
            if (lit and !c->hasCamouflage ())
                return 100;
            /* See into darkness by light source. */
            int light = intr (kLightSource);
            if (light and dist <= light * 5 + 2)
                return 100;
            /* Spot creature by night vision or EM-Field vision. */
            int nv = intr (kNightVision);
            int em = intr (kEMFieldVision);
            if ((em and c->myIlk ()->mType == kAlien and dist <= em * 5 + 2) or
                (nv and c->radiates () and !c->hasCamouflage () and dist <= nv * 5 + 2))
            {
                return 100;
            }
        }
        /* See through walls. */
        if  (lit and seeByXRay (c->mX, c->mY))
            return 100;
    } else {
        return canSee (c->mX, c->mY);
    }
    return 0;
}

bool
shCreature::seeByXRay (int x, int y)
{
    int base = intr (kXRayVision);
    if (!base)
        return false;
    shSkill *s = getSkill (kMutantPower, kXRayVisionPower);
    int bonus = s ? s->getValue () / 3 : 0;
    return distance (this, x, y) <= 5 * (bonus + base) + 2;
}

bool
shCreature::canSenseLife (shCreature *c) {
    return (intr (kSenseLife) and c->isAlive () and
            distance (this, c->mX, c->mY) <= 5 * (mCLevel * 3 + 5) + 2);
}

bool
shCreature::canHearThoughts (shCreature *c)
{
    if (!c->hasMind ())
        return false;
    int tele = intr (kTelepathy);
    if (!tele)
        return false;
    shSkill *s = getSkill (kMutantPower, kTelepathyPower);
    int bonus = s ? s->getValue () * 3 : 0;
    return distance (this, c->mX, c->mY) <= 5 * (bonus + tele) + 2;
}

bool
shCreature::canTrackMotion (shCreature *c)
{
    int range = intr (kMotionDetection);
    return range and c->isMoving () and distance (this, c->mX, c->mY) <= range * 5 + 2;
}

bool
shCreature::canTrackMotion (shFeature *f)
{
    if (!f->isDoor ())  return false;
    int range = intr (kMotionDetection);
    return range and f->mDoor.mMoveTime + SLOWTURN >= Clock and
        distance (this, f->mX, f->mY) <= range * 5 + 2;
}

bool
shCreature::canFeelSteps (shCreature *c)
{
    if (intr (kFlying) or !c->isMoving ())  return false;
    int range = intr (kTremorsense);
    if (!range)  return false;
    shSkill *s = getSkill (kMutantPower, kTremorsensePower);
    int bonus = (s ? s->getValue () * 3 : 0) + mCLevel;
    return distance (this, c->mX, c->mY) <= 5 * (bonus + range) + 2;
}

bool
shCreature::canSmell (shCreature *c)
{
    if (!intr (kScent))  return false;
    return distance (this, c) <= 5 * 5 + 2;
}

bool
shCreature::isAwareOf (shCreature *c)
{
    return canSee (c) or canHearThoughts (c) or canSenseLife (c)
        or canTrackMotion (c) or canFeelSteps (c) or canSmell (c);
}

void
shCreature::earnXP (unsigned int amount)
{
    if (isHero ())  Hero.earnScore (amount);
    mXP += amount;
    if (mXP / 1000 >= mCLevel) {
        if (mXP / 1000 > mCLevel) {
            /* No multiple level gain. */
            mXP = mCLevel * 1000;
        }
        levelUp ();
    }
}

void
shCreature::beatChallenge (int challenge)
{
    if (challenge < -1)  return;
    int diff = challenge - mCLevel;

    if (-1 == challenge) {
        mXP += 1000 - mXP % 1000;
        levelUp ();
        return;
    }

    unsigned int amount = 125.0 * pow (2.0, (double) diff / 2.0);
    earnXP (amount);
}

void
shCreature::levelUp ()
{
    if (mState == kDead)  return;

    ++mCLevel;

    if (isHero ()) {
        mReflexSaveBonus = Hero.mProfession->mReflexSaveBonus * mCLevel / 4;
        mWillSaveBonus = Hero.mProfession->mWillSaveBonus * mCLevel / 4;
    }

    if (isHero ()) {
        I->p ("You've obtained level %d!", mCLevel);
        if (mCLevel % 3 == 0) {
            I->p ("Your professional rank is now %s.", Hero.getTitle ());
        }
        I->drawSideWin (this);
        I->pause ();
    }
    /* Gain hit points. */
    int x = ABILITY_MODIFIER (mAbil.Con ());
    if (isHero ()) {
        x += RNG (1, Hero.mProfession->mHitDieSize);
    } else {
        x += RNG (1, 8);
    }
    if (x < 1)  x = 1;
    if (isHero ())  ++x; /* Fudge things slightly in favor of the hero... */
    mMaxHP += x;
    mHP += x;

    if (isOrc () and needsRestoration ())
        restoration (RNG (1, 2));

    if (mCLevel % 4 == 0) {
        if (isHero ())  I->drawSideWin (this);
        gainAbility (true, 1);
        computeAC ();
    }
    if (isHero ()) {
        mSkillPoints += Hero.mProfession->mNumPracticedSkills;
        I->p ("You may advance your skills with the '%s' command.",
              I->getKeyForCommand (shInterface::kEditSkills));
        if (Hero.mProfession == XelNaga and mCLevel == 6) {
            mInnateIntrinsics.set (kJumpy, true);
            I->p ("You feel like jumping around.");
        }
        if (Hero.mProfession == Quarterback and mCLevel == 9) {
            mInnateIntrinsics.set (kCanSwim, true);
            I->p ("You suddenly remember you could swim!");
        }
        if (Hero.mProfession == XelNaga and mCLevel == 12) {
            I->p ("Your dormant instincts awake.");
        }
    }
    computeIntrinsics ();
}


//RETURNS: 1 if attack is deadly (caller responsible for calling die ()); o/w 0
int
shCreature::sufferAbilityDamage (abil::Index idx, int amount, bool perm)
{
    int oldscore = mAbil.curr (idx);
    int oldmodifier = ABILITY_MODIFIER (oldscore);

    if (perm)
        mAbil.hurt_mod (idx, -amount);
    else
        mAbil.temp_mod (idx, -amount);

    if (abil::Con == idx) {
        /* constitution damage results in lost HP */
        int hploss = mCLevel *
            (oldmodifier - ABILITY_MODIFIER (mAbil.curr (abil::Con)));

        mMaxHP -= hploss;
        if (mMaxHP < mCLevel) {
            hploss -= (mCLevel - mMaxHP);
            mMaxHP = mCLevel;
        }
        mHP -= hploss;
        if (mHP <= 0) {
            mHP = 0;
            if (isHero ()) {
                I->drawSideWin (this);
                I->drawLog ();
            }
            return 1;
        }
    } else if (abil::Psi == idx) {
        /* May lose the strength to maintain powers. */
        if (isHero () and mAbil.Psi () < 0) {
            /* Count number of active powers and their upkeep. */
            int on = 0;
            shMutantPower powers[kMaxHeroPower];
            for (int i = kNoMutantPower; i < kMaxHeroPower; ++i) {
                if (mMutantPowers[i] == MUT_POWER_ON) {
                    powers[on++] = shMutantPower (i);
                }
            }
            /* Drop powers as long as there is need. */
            int lost = 0;
            while (mAbil.Psi () < 0 and on) {
                /* Pick and turn off one power. */
                int chosen = RNG (on);
                stopMutantPower (powers[chosen]);
                /* Refund psi immediately.  This simulates ability damage
                   hitting points allocated to powers. */
                int cost = MutantPowers[chosen].maintainCost ();
                mAbil.perm_mod (abil::Psi, cost);
                /* Plug hole in array.  Another pick might be needed. */
                powers[chosen] = powers[on-1];
                --on;
                ++lost;
            }
            if (lost == 1)
                I->p ("You fail to maintain a power.");
            else if (lost)
                I->p ("You fail to maintain %d powers.", lost);
        }
    }

    computeIntrinsics ();

    /* Death by ability damage. */
    if (mAbil.totl (idx) <= 0) {
        if (isA (kMonGhoul) and idx == abil::Int) {
            /* Death is not immediate. */
            return 0;
        }
        if (isHero ()) {
            I->drawSideWin (this);
            I->drawLog ();
        }
        return 1;
    }
    return 0;
}


static int
reflectDir (shDirection *dir, int scatter)
{
    switch (RNG(scatter)) {
    case 0: *dir = uTurn (*dir); return 1;
    case 1: *dir = rightTurn (*dir); return 0;
    case 2: *dir = leftTurn (*dir); return 0;
    case 3: *dir = rightTurn (rightTurn (*dir)); return 0;
    case 4: *dir = leftTurn (leftTurn (*dir)); return 0;
    case 5: *dir = rightTurn (uTurn (*dir)); return 0;
    case 6: *dir = leftTurn (uTurn (*dir)); return 0;
    default: return 0;
    }
}

/* FIXME: Move to Fight.cpp */
/* if the attack is reflected, prints a message, modifies dir, and returns 1
   o/w returns 0
*/
int
shCreature::reflectAttack (shAttack *attack, shDirection *dir)
{
    bool reflect = intr (kReflection) > RNG (100);
    bool parry = mWeapon and mWeapon->isA (kObjLightSaber) and
                 getSkillModifier (kSword) * 5 > RNG (100);
    if ((kLaser == attack->mDamage[0].mEnergy and (reflect or parry)) or
        (kParticle == attack->mDamage[0].mEnergy and parry))
    {
        const char *who = the ();
        int refl = 0;

        if (parry) {
            refl = reflectDir (dir, 7);
            msg (fmt ("You parry the %s with %s!",
                 attack->noun (), YOUR (mWeapon)))
            or appear (fmt ("%s parries the %s with %s!", who,
                       attack->noun (), mWeapon->her (this)));
        } else if (Hero.cr ()->intr (kBlind)) { /* silently reflect */
            refl = reflectDir (dir, 7);
            return 1;
            /* TODO: Randomly choose what reflected the shot. */
        } else if (mHelmet and mHelmet->isA (kObjBrainShield)) {
            refl = reflectDir (dir, 7);
            I->p ("The %s is %s by %s shiny hat!", attack->noun (),
                  refl ? "reflected" : "deflected", namedher ());
            mHelmet->set (obj::known_type | obj::known_appearance);
        } else if (mBodyArmor and mBodyArmor->isA (kObjReflecSuit)) {
            refl = reflectDir (dir, 7);
            I->p ("The %s is %s by %s shiny armor!", attack->noun (),
                  refl ? "reflected" : "deflected", namedher ());
            mBodyArmor->set (obj::known_type | obj::known_appearance);
        } else { /* TODO: Check for innate reflection before using this. */
            refl = reflectDir (dir, 7);
            I->p ("The %s is %s by %s shiny %s!", attack->noun (),
                  refl ? "reflected" : "deflected", namedher (),
                  isSlime () ? "surface" : "skin");
        }
        return 1;
    } else {
        return 0;
    }
}

/* Imported from Fight.cpp: */
extern const char *youHitMonMesg (shAttack::Type);
extern const char *youShootMonMesg (shAttack::Type);
extern const char *monShootsMonMesg (shAttack::Type);

int
shCreature::suffer_toxin (shAttack *attack, shCreature *attacker)
{
    int drain = 0, damage = 0, resist = 0;
    abil::Index ability = abil::No;
    for (int i = 0; i < ATKDMG; ++i) {
        shEnergyType energy = attack->mDamage[i].mEnergy;
        if (energy < kDrnStr or energy > kDmgPsi)
            continue;
        if (RNG (100) >= attack->mDamage[i].mChance)
            continue;

        resist = getResistance (energy);
        int amt = RNG (attack->mDamage[i].mLow, attack->mDamage[i].mHigh);
        amt = maxi (0, amt - resist);

        if (energy >= kDrnStr and energy <= kDrnPsi)
            drain += amt;
        if (energy >= kDmgStr and energy <= kDmgPsi)
            damage += amt;

        ability = abil::Index ((int (energy) - int (kDrnStr)) % NUM_ABIL + 1);
    }

    if (!(drain or damage))
        return 0;

    char *msgbuf = GetBuf ();
    int l = 0;
    msgbuf[0] = 0;

    if (!(shAttack::kPoisonRay == attack->mType or
        shAttack::kPrick == attack->mType or /* Too obvious. */
        shAttack::kNoAttack == attack->mType))
    {
        l = snprintf (msgbuf, SHBUFLEN, "%s was poisonous!  ",
            attacker->her (attack->noun ()));
    }

    int neutr = 0;
    if (usesPower (kBGAwareness)) {
        if (damage) {
            neutr = mini (damage, mAbil.Psi ());
            damage -= neutr;
        }

        if (drain) {
            const int factor = 3;
            int cost = drain / factor + (drain % factor ? 1 : 0);
            cost = mini (cost, mAbil.Psi () - neutr);
            neutr += cost;
            drain -= mini (drain, cost * factor);
        }
    }
    bool is_harmed = damage or drain;

    const char *sh_abil = abil::shortn (this->myIlk ()->mType, ability);
    if (!is_harmed and !neutr and resist) {
        l += snprintf (msgbuf+l, SHBUFLEN-l, "You resist.");
    } else if (!neutr and resist) {
        l += snprintf (msgbuf+l, SHBUFLEN-l, "You resist partially. (-%d %s)",
            damage+drain, sh_abil);
    } else if (neutr) {
        /* These messages are long enough to put them on their own line. */
        if (strlen (msgbuf)) {
            msg (msgbuf);
            msgbuf[0] = 0;
        }

        if (!is_harmed and resist) {
            msg (fmt ("You neutralize some of the toxin and resist the rest. (-%d Psi)", neutr));
        } else if (!is_harmed) {
            msg (fmt ("You neutralize the toxin. (-%d Psi)", neutr));
        } else if (resist) {
            msg (fmt ("You partially neutralize and resist. (-%d %s) (-%d Psi)", damage+drain, sh_abil, neutr));
        } else {
            msg (fmt ("You partially neutralize the toxin. (-%d %s) (-%d Psi)", damage+drain, sh_abil, neutr));
        }
    } else {
        l += snprintf (msgbuf+l, SHBUFLEN-l, "You feel bad! (-%d %s)",
            damage+drain, sh_abil);
    }

    if (neutr)
        mAbil.temp_mod (abil::Psi, -neutr);

    if (strlen (msgbuf)) {
        msg (msgbuf);
        msgbuf[0] = 0;
    }

    if (drain) {
        mAbil.temp_mod (ability, -drain);
        if (mAbil.totl (ability) <= 0)
            return 1;
    }

    if (damage and sufferAbilityDamage (ability, damage, 0))
        return 1;  /* Dead, dead, dead! */


    if (is_harmed and NOT0 (attacker,->isA (kMonRadspider)) and
        !isA (kMonRadspider) and !mMutantPowers[kShootWebs] and
        !RNG (11 - is_harmed))
    {
        getMutantPower (kShootWebs);
    }

    return 0;
}

/* works out damage done by an attack that has hit.  applies resistances,
   special defences, special damage (e.g. stunning), and subtracts HP.
   returns: 1 if attack kills us; o/w 0
*/
int
shCreature::sufferDamage (shAttack *attack, shCreature *attacker,
    shObject *weapon, int bonus, int multiplier, int divisor)
{
    if (kDead == mState)
        return 0;

    interrupt ();

    /* Quarterbacks are expert at kicking and take no damage. */
    if (isHero () and attack == &Attacks[kAttKickedWall] and
        Hero.mProfession == Quarterback)
        return 0;

    int totaldamage = 0;
    int shieldworked = 0;
    int stopped = 0;
    int absorbed = 0;
    int torcdiff = 0;
    int lethal = 0; /* At least one point of lethal damage was sent. */
    int oldhp = mHP;
    bool abil_dam = false;
    bool LOS = mLevel->existsLOS (Hero.cr ()->mX, Hero.cr ()->mY, mX, mY);
    bool herosees = Hero.cr ()->canSee (this) or
         (attack->isLightGenerating () and LOS);
    bool heroseestile = Hero.cr ()->canSee (mX, mY) and LOS;
    int dmgtable[ATKDMG] = {0, 0, 0};
    bool process[ATKDMG];
    shObjectVector reveal; /* For tracking discovered absorber items. */

    const char *thewho = the ();

    /* Recognize this type of flaslight wielded by monster as dangerous. */
    if (weapon and weapon->isA (kObjLightSaber) and isHero () and !intr (kBlind))
        weapon->set (obj::known_type);

    /* Hit with imbued weapon. */
    if (attacker and attacker->usesPower (kImbue) and
        (attack->isMeleeAttack () or attack->isMissileAttack ()))
    {
        bonus += mini (8, maxi (1, attacker->getSkillModifier (kMutantPower, kImbue) / 2));
    }

    if (attacker and attacker->isHero () and mHidden)
        mHidden = 0; /* Reveal hit (accidentally or not) targets. */

    /* Iterate through three possible types of damage. */
    /* Phase 1: Collect damage. */
    for (int i = 0; i < ATKDMG; ++i) {
        shEnergyType energy = attack->mDamage[i].mEnergy;
        if (kNoEnergy == energy)
            break;

        /* Some damages do not kick in always. */
        if (RNG (100) >= attack->mDamage[i].mChance) {
            process[i] = false;
            continue;
        }
        process[i] = true;

        int psiattack = 0;
        int damage = 0;

        if (!shAttack::isLethal (energy))
            multiplier = divisor = 1;

        /* Compute base damage. */
        if (energy != kPsychic or !attacker or !weapon) {
            damage = multiplier *
                RNG (attack->mDamage[i].mLow, attack->mDamage[i].mHigh);
        } else { /* Attack with weapon dealing psychic damage. */
            int base = maxi (1, attacker->mAbil.Psi ());
            damage = multiplier * RNG (1, base);
            if (mImplants[shObjectIlk::kNeck] and attacker and
                attacker->mImplants[shObjectIlk::kNeck])
            {
                torcdiff = this->mImplants[shObjectIlk::kNeck]->mBugginess -
                       attacker->mImplants[shObjectIlk::kNeck]->mBugginess;
                /* Difference can be only -1 or +1. */
                if (torcdiff) torcdiff /= torcdiff > 0 ? torcdiff : -torcdiff;
                bonus += damage * -torcdiff * 3 / 4;
            }
            psiattack = 1;
        }

        /* Bugs react to bugginess status of melee attacks. */
        if (isInsect () and attack->isMeleeAttack () and weapon and
            (energy == kConcussive or energy == kHackNSlash))
        {
            if (weapon->isBugProof ()) {
                /* No change. */
            } else if (weapon->isDebugged ()) { /* 150% */
                damage = damage * 3 / 2;
            } else if (weapon->isBuggy ()) {    /*  50% */
                damage /= 2;
            } /* Optimized does not interact specially. */
        }

        /* Whacking with things in melee is influenced by strength applied. */
        if (attacker and attack->isMeleeAttack () and
            (energy == kConcussive or energy == kHackNSlash))
        {
            bonus += ABILITY_BONUS (attacker->mAbil.Str ());
        }

        I->diag ("Rolling damage %d*(%d-%d)%+d = %d",
                 multiplier, psiattack ? 1 : attack->mDamage[i].mLow,
                 psiattack ? attacker->mAbil.Psi () : attack->mDamage[i].mHigh,
                 bonus, damage + bonus);

        /* Global damage bonus influences many ways of doing harm but should
           not affect poisons, effect durations and other such things. */
        if (!shAttack::isSpecial (energy)) {
            damage += bonus;
            /* Full bonus is applied only to first eligible energy type. */
            bonus = 0;
        }

        /* This is applied always because it represents partially evading an
           attack and thus getting affected less by all its components. */
        damage /= divisor;

        /* No energy type should deal less than one damage point by itself. */
        if (damage <= 0)  damage = 1;

        /* Possibly nullify some of the damage with a shield. */
        if (intr (kShielded) and
            (attack->isMissileAttack () or attack->isAimedAttack ()) and
            !attack->bypassesShield ())
        {
            if (attack->isAbsorbedForFree (i)) {
                I->diag ("shield absorbed %d for free", damage);
                damage = 0;
            } else {
                stopped += loseEnergy (damage);
                if (stopped) {
                    shieldworked = 1;
                }
                damage -= stopped;
                I->diag ("shield stopped %d", stopped);
            }
        }

        /* Reduce damage according to resistances. */
        int resist = getResistance (energy);
        damage = damage - resist;
        if (damage < 0)  damage = 0;

        /* Not resisted damage may be absorbed. */
        if (damage and energy == kElectrical) {
            int absorb = getShockCapacity ();
            if (absorb) {
                absorb = mini (absorb, damage);
                damage -= absorb;
                absorbShock (absorb, &reveal);
                absorbed += absorb;
            }
        }

        dmgtable[i] = damage; /* Save for later effect processing. */

        /* If you successfully hurt yourself with your own grenade
           you now know what it does. */
        if (damage and attacker == this and isHero () and
            weapon and weapon->isThrownWeapon () and
            energy != kRadiological) /* Hard to be certain in this case. */
        { /* TODO: Should also identify stack the grenade was part of. */
            weapon->set (obj::known_type);
        }

        /* Count lethal damage. */
        if (damage and shAttack::isLethal (energy)) {
            totaldamage += damage;
            lethal = 1;
        }
    }

    /* Phase 2: Special attack effects. */
    int idx = attack->findEnergyType (kSpecial);
    int spcdmg = idx != -1 ? dmgtable[idx] : 0;
    /* Test for specific attack identifiers. */
    if (attack == &Attacks[kAttVatSpill] and isA (kMonVatSlime)) {
        mHP += dmgtable[0];
        if (mHP > mMaxHP) {
            mMaxHP += RNG (1, 3);
            mHP = mini (mMaxHP, mHP);
        }
        appear (fmt ("%s %s revitalized by the sludge spill.", the (), are ()));
        return 0;
    }
    /* Test for specific attack types. */
    switch (attack->mType) {
    case shAttack::kAugmentationRay:
    {   /* Gain ability ray. */
        if (!isAlive ())  break;
        bool control = weapon and weapon->isOptimized () and !RNG (3);
        gainAbility (control, 1);
        if (weapon and isHero () and attacker->isHero ())  weapon->set (obj::known_type);
        break;
    }
    case shAttack::kBreatheTraffic:
        inflict (kHosed, FULLTURN * spcdmg);
        break;
    case shAttack::kCreditDraining:
    {
        int amount = loseMoney (spcdmg);
        if (amount) {
            I->p ("Your wallet feels lighter.  (-$%d)", amount);
            /* Some is money is permanently lost so the attack
               actually is harmful instead of being 100% joke. */
            attacker->gainMoney (amount * 2 / 3);
            attacker->mMaxHP += amount / 10;
            attacker->mHP += amount / 5;
            if (attacker->mHP > attacker->mMaxHP) {
                attacker->mHP = attacker->mMaxHP;
            }
            attacker->transport (-1, -1);
            attacker->mHidden = 0;
        }
        return 0; /* Attacker is away already. */
    }
    case shAttack::kCryolator:
        if (totaldamage >= mHP) {
            mHP = 1;
            inflict (kFrozen, FULLTURN * RNG (3, 8));
            totaldamage = 0;
            dmgtable[0] = 0;
            lethal = 0;
        }
        break;
    case shAttack::kDecontaminationRay:
        if (isHero ()) {
            int prev = mRad;
            mRad -= spcdmg;
            if (mRad < 0)  mRad = 0;
            if (mRad) {
                I->p ("You feel a bit less contaminated.");
            } else if (prev) {
                I->p ("You feel purified.");
            }
            /* Identify decontamination ray gun. */
            if (weapon and (mRad or prev)) {
                weapon->set (obj::known_type);
                if (weapon->mOwner == this)  weapon->announce ();
            }
        }
        break;
    case shAttack::kExtractBrain:
    {
        if (!attacker) {
            I->p ("You hear the program bugs craving for your gray matter.");
            break;
        }
        if (!isHero ())  break; /* Not implemented for monsters. */
        /* TODO: Implement for monsters.  Not a big problem. */
        if (mHelmet) {
            shObject *helmet = mHelmet;
            I->p ("%s removes %s!", THE (attacker), YOUR (helmet));
            doff (helmet);
            removeObjectFromInventory (helmet);
            if (!attacker->addObjectToInventory (helmet))
                Level->putObject (helmet, attacker->mX, attacker->mY);
            break;
        }
        int i;
        /* Only implants in frontal, parietal and occipital lobes
           need to be extracted in order to get at your brain. */
        for (i = shObjectIlk::kFrontalLobe;
             i <= shObjectIlk::kOccipitalLobe; ++i)
        {
            if (mImplants[i]) {
                shObject *impl = mImplants[i];
                I->p ("%s extracts your %s!", THE (attacker), YOUR (impl));
                doff (impl);
                removeObjectFromInventory (impl);
                if (!attacker->addObjectToInventory (impl))
                    Level->putObject (impl, attacker->mX, attacker->mY);
                break;
            }
        }
        /* Extraction took place this attack.  This is enough. */
        if (i <= shObjectIlk::kOccipitalLobe)  break;
        if (Hero.getStoryFlag ("brain incision")) {
            I->p ("%s extracts your brain!", THE (attacker));
            mGlyph.mSym = '!';
            mGlyph.mColor = kWhite;
            mGlyph.mTileX = 0;
            mGlyph.mTileY = kRowCanister;
            mAbil._max._str = mAbil._totl._str = 0; /* Stats are set this way */
            mAbil._max._con = mAbil._totl._con = 0; /* because you have just  */
            mAbil._max._dex = mAbil._totl._dex = 0; /* been separated from    */
            mAbil._max._agi = mAbil._totl._agi = 0; /* the rest of your body. */
            I->p ("You are now brain in a jar.");
            return 1;
        } else { /* This is warning an instakill is coming your way. */
            I->p ("%s makes an incision into your skull!", THE (attacker));
            Hero.setStoryFlag ("brain incision", 1);
        }
    }
    case shAttack::kFlare:
        if (isA (kMonSmartMissile))  return 1;  /* Fooled by heat. */
        break;
    case shAttack::kHalf:
        totaldamage -= dmgtable[0];
        dmgtable[0] = maxi (dmgtable[0], mHP / 2);
        totaldamage += dmgtable[0];
        break;
    case shAttack::kHealingRay:
        if (isAlive ())  healing (spcdmg, 0);
        break;
    case shAttack::kPlague:
        if (mBodyArmor
            and (mBodyArmor->isA (kObjMBioArmor1)
                 or mBodyArmor->isA (kObjGreenCPA)))
        {
            break; /* These armors grant immunity to plague. */
        }
        /* Some leeway before first effect. */
        inflict (kPlagued, RNG (6, 16) * FULLTURN);
        break;
    case shAttack::kRestorationRay:
        if (restoration (1) and weapon and attacker->isHero ())
            weapon->set (obj::known_type);
        break;
    case shAttack::kWaterRay:
        if (isA (kMonManEatingPlant) or
            isA (kMonGreenTomato) or isA (kMonRedTomato))
        {
            if (attacker and attacker->isHero () and attacker->canSee (this)) {
                I->p ("You water %s.  It appears to be grateful.", the ());
            }
            /* Pacify. */
            mDisposition = kIndifferent;
            if (isA (kMonManEatingPlant) and attacker and attacker->isHero ()
                and attacker->canHearThoughts (this))
            {
                I->p ("You sense %s thinking blood would be more welcome.", the ());
            }
        }
        break;
    default: break;
    }

    /* Phase 3: Messages. */
    if (torcdiff and isHero ()) {
        I->p ("%s %s incoming psionic force.",
            YOUR (mImplants[shObjectIlk::kNeck]),
            torcdiff < 0 ? "amplifies" : "dampens");
    }
    /* Print a mesage about your shield. */
    if (shieldworked) {
        bool fizzle = (0 == countEnergy ());
        see (fmt ("%s force shield nullifies %s damage%s (%d dam)",
             namedher (), totaldamage ? "some of the" : "the",
             fizzle ? " and fizzles out!" : "!", stopped));
        if (fizzle and mBelt and mBelt->isA (kObjShieldBelt))
            mBelt->clear (obj::active);
    }

    bool showdam = isHero () or herosees or
        (attacker and attacker->isHero () and attack->isMeleeAttack ());
    showdam = showdam and lethal;
    if (totaldamage >= mHP and !isA (kMonUsenetTroll)) {
        if (attack->isMeleeAttack () and attacker and attacker->isHero ()) {
            if (weapon and weapon->myIlk ()->mMeleeAttack and
                Attacks[weapon->myIlk ()->mMeleeAttack].mType == shAttack::kClub
                and (isA (kMonGreenTomato) or isA (kMonRedTomato)))
            { /* YAFM for clubbed to death killer tomatoes. */
                I->p ("*SPLAT*");
            } else {
                pDeathMessage (the (), kSlain, 1);
            }
        } else if (attack->isMissileAttack () and attacker and
                   attacker->isHero () and attack->mEffect != shAttack::kBurst)
        {
            pDeathMessage (the (), kSlain, 1);
        } else if (attack->isAimedAttack () and attacker and
            attacker->isHero () and weapon and weapon->myIlk ()->mGunAttack and
            Attacks[weapon->myIlk ()->mGunAttack].mEffect == shAttack::kSingle)
        {
            pDeathMessage (isHero () ? "yourself" : the (), kSlain, 1);
        }
        mHP = 0;
    } else {
        int dmgsum = 0;
        for (int i = 0; i < ATKDMG; ++i)
            dmgsum += dmgtable[i];
        /* Attack has only nonlethal energies. */
        bool specialonly = !totaldamage and dmgsum;
        char punct = totaldamage ? (multiplier > 1 ? '!' : '.') : '?';
        /* Hit in melee. */
        if (attack->isMeleeAttack () and attacker and attacker->isHero ()) {
            const char *hitmesg = youHitMonMesg (attack->mType);
            /* It is a little difficult to bite with sealed helmet on. */
            if (attacker->mHelmet and attacker->mHelmet->isSealedArmor () and
                attack->mType == shAttack::kBite)
            {
                hitmesg = youHitMonMesg (shAttack::kHeadButt);
            }
            /* Same for raking with boots on. */
            if (attacker->mBoots and attack->mType == shAttack::kRake)
            {
                hitmesg = youHitMonMesg (shAttack::kKick);
            }
            /* Hits granting experience are termed abductions. */
            if (Hero.cr ()->mGlyph.mSym == 'g' and !is (kNoExp) and
                attack->dealsEnergy (kViolating) and
                dmgtable[attack->findEnergyType (kViolating)])
            {
                hitmesg = " abduct";
                punct = '!';
            } else if (attack->dealsEnergy (kViolating) and
                dmgtable[attack->findEnergyType (kViolating)])
            {
                punct = '.';
            }

            I->p ("You%s %s%c", hitmesg, isHero () ? "yourself" : the (), punct);
        /* Hit with thrown weapon. */
        } else if (attack->isMissileAttack () and attacker and
            attacker->isHero () and
            !specialonly and (attack->mEffect == shAttack::kSingle or
                              attack->mType == shAttack::kDisc))
        {
            I->p ("You hit %s%c", isHero () ? "yourself" : the (), punct);
        /* Ranged hit from a gun shooting individual bullets. */
        } else if (attack->isAimedAttack () and attacker and lethal and weapon
            and (weapon->myIlk ()->mGunAttack or weapon->myIlk ()->mZapAttack)
            and attack->mEffect == shAttack::kSingle
            and (heroseestile or herosees))
        {
            const char *hit = attacker->isHero ()
                ? youShootMonMesg (attack->mType)
                : monShootsMonMesg (attack->mType);
            const char *whom = this == attacker ? herself () : the ();
            I->p ("%s %s %s%c", attacker->the (), hit, whom, punct);
        /* Monster hitting hero with bullet guns and weapons like that. */
        } else if (lethal and attack->isAimedAttack () and isHero () and
            !attacker->isHero () and attack->mEffect == shAttack::kSingle)
        {
            I->p ("You are hit%c", punct);
        }
    }
    /* Descriptions for unique attacks. */
    if (!herosees) {
        /* Mute. */
    } else if (attack == &Attacks[kAttGaussRay] and totaldamage) {
        I->p ("Sparks fly from %s!", the ());
    } else if (attack == &Attacks[kAttAcidWave]) {
        I->p ("The acid splash %ss %s!", !mHP ? "dissolve" : "hit", the ());
    }

    /* Show damage in parentheses.  It makes combat more tactical. */
    if (showdam) {
        I->nonlOnce ();
        I->p ("  (%d dam)", totaldamage);
    }

    /* Show absorbed damage in parentheses. */
    if (absorbed) {
        I->nonlOnce ();
        I->p ("  (%d absorbed)", absorbed);
    }

    /* Have you discovered that some of your gear can absorb energy? */
    for (int i = 0; i < reveal.count (); ++i) {
        shObject *obj = reveal.get (i);
        if (i == 0) {
            I->p ("Hey, %s can absorb energy!", YOUR (obj));
        } else {
            I->p ("Hey, %s can do this too!", YOUR (obj));
        }
        obj->set (obj::known_type | obj::known_charges);
        obj->announce ();
    }

    /* Rage quit? */
    if (totaldamage >= mHP and isA (kMonUsenetTroll)) {
        if (herosees or Hero.cr ()->canHearThoughts (this)) {
            I->p ("%s rage quits!", the ());
        } else {
            I->p ("You hear someone loudly exclaim \"I quit!\".");
        }
        mHP = 0;
    }

    /* Phase 4: Deal damage. */
    if (mHP == 0)  goto skip_phase_four;

    damageEquipment (attack, kNoEnergy); /* Apply attack type effects. */

    /* Damage will be counted again more accurately and stored in
       'totaldamage'.  Some special attacks can hurt HP in certain cases. */
    totaldamage = 0;
    for (int i = 0; i < ATKDMG; ++i) {
        shEnergyType energy = attack->mDamage[i].mEnergy;
        if (kNoEnergy == energy)
            break;

        if (!process[i])  continue; /* Damage type not triggered. */

        int damage = dmgtable[i];
        if (!damage)  continue;

        /* Possibly damage some equipment.  This occurs even
           if you suffer no harm yourself.  */
        if (shAttack::kNoAttack == attack->mType) {
            /* Special attack methods won't affect your equipment */
        } else if (kElectrical == energy or kMagnetic == energy or
                   kBurning == energy    or kBugging == energy or
                   kPlasma == energy     or kCorrosive == energy)
        {
            damageEquipment (attack, energy);
        }

        if (!dmgtable[i])  continue; /* Damage completely resisted. */

        /* Apply energy-specific special effects */

        switch (energy) {
        case kDrnStr: case kDmgStr: case kDrnCon: case kDmgCon:
        case kDrnAgi: case kDmgAgi: case kDrnDex: case kDmgDex:
        case kDrnInt: case kDmgInt: case kDrnPsi: case kDmgPsi:
            /* To generate better messages it is better to handle
               all stat drain attacks together. */
            abil_dam = true;
            break;
        case kBlinding:
            if (!intr (kBlind) and damage) {
                if (isHero ()) {
                    I->p ("You are blinded!");
                    I->pauseXY(mX, mY);
                }
                makeBlinded (FULLTURN * damage);
            }
            break;
        case kMesmerizing:
            if (!is (kAsleep)) {
                if (isHero ()) {
                    if (damage) {
                        I->p ("You fall asleep!");
                        I->pauseXY(mX, mY);
                    } else {
                        I->p ("You yawn.");
                    }
                } else {
                    if (damage) {
                        if (herosees or Hero.cr ()->canHearThoughts (this)) {
                            I->p ("%s falls asleep!", THE (this));
                        }
                    } else if (herosees) {
                        I->p ("%s yawns.", THE (this));
                    }
                }
                if (damage) {
                    inflict (kAsleep, FULLTURN * damage);
                }
            }
            break;
        case kSickening:
            if (usesPower (kBGAwareness) and mAbil.Psi ()) {
                if (isHero ())
                    mAbil.temp_mod (abil::Psi, -1);
            } else {
                inflict (kSickened, FULLTURN * damage);
            }
            break;
        case kStunning:
            inflict (kStunned, FULLTURN * damage);
            break;
        case kConfusing:
            inflict (kConfused, FULLTURN * damage);
            break;
        case kViolating:
            if (!damage and herosees) {
                msg ("You aren't affected.")
                or I->p ("%s doesn't seem to notice.", thewho);
            }
            if (damage) {
                inflict (kViolated, FULLTURN * damage);
                if (attacker and attacker->isHero () and
                    attacker->mGlyph.mSym == 'g')
                { /* Make an abduction! */
                    mConditions |= kNoExp;
                    attacker->beatChallenge (mCLevel);
                }
                if (NOT0 (weapon,->isA (kObjAnalProbe)) and isHero () and
                    mGlyph.mSym == 'g')
                {
                    weapon->identify ();
                }
            }
            break;
        case kParalyzing:
            inflict (kParalyzed, FULLTURN * damage);
            break;
        case kDisintegrating:
            if (isHero ()) {
                shObject *obj;
                if (NULL != (obj = mCloak) or
                    NULL != (obj = mBodyArmor) or
                    NULL != (obj = mJumpsuit))
                {
                    I->p ("%s is annihilated!", YOUR (obj));
                    removeObjectFromInventory (obj);
                    delete obj;
                    continue;
                }
                return 1;
            } else if (damage) {
                return 1;
            } else {
                see (fmt ("%s resists!", thewho));
            }
            break;
        case kMagnetic:
            /* An easy way to identify gauss ray gun: fire it at a robot. */
            if (damage and Hero.cr ()->canSee (this) and
                weapon and weapon->isA (kObjGaussRayGun))
            {
                weapon->set (obj::known_type);
            }
            totaldamage += damage;
            break;
        case kRadiological:
            if (isHero ()) {
                takeRads (damage * 2);
            } else if (damage) { /* treat like poison */
                if (sufferAbilityDamage (abil::Con, RNG (6), 0)) {
                    /* dead, dead, dead! */
                    return 1;
                } else if (herosees) {
                    I->p ("%s seems weakened.", thewho);
                    if (weapon and weapon->isA (kObjGammaRayGun)) {
                        weapon->set (obj::known_type | obj::known_appearance);
                    }
                }
                sterilize ();
            }
            if (usesPower (kGammaSight) and !intr (kBlind)) {
                if (getSkillModifier (kMutantPower, kGammaSight) < 5) {
                    if (isHero ())
                        I->p ("Intensive radiation blinds you!");
                    makeBlinded (FULLTURN * maxi (2, damage / 5));
                }
            }
            break;
        case kTransporting:
            if (attacker->isHero () and attacker->canSee (this) and weapon)
                weapon->set (obj::known_type);

            if (attack->mDamage[i].mHigh == 100) {
                this->transport (-1, -1);
            } else {
                trn::ranged (this, attack->mDamage[i].mLow, attack->mDamage[i].mHigh, 0);
            }
            break;
        case kWebbing:
            if (attack->mEffect != shAttack::kBurst) {
                msg ("You are entangled in a web!")
                or see (fmt ("%s %s entangled in a web!", thewho, are ()));
            }
            if (mTrapped.mWebbed)  damage /= 2;
            mTrapped.mWebbed += damage;
            break;
        case kAccelerating:
            inflict (kSpeedy, FULLTURN * damage);
            break;
        case kDecelerating:
            inflict (kSlowed, FULLTURN * damage);
            break;
        case kSpecial:
            /* Do not add up damage. */
            break;
        default: /* Various plainly harmful energy types. */
            totaldamage += damage;
            break;
        }
    }

    if (abil_dam) {
        int res = suffer_toxin (attack, attacker);
        if (res)
            return res;
        /* For balance, monsters take double HP damage in addition to str drain. */
        /*
        if (!isHero ())
            totaldamage += (damage *= 2);
        else
            damage = 0;
        */
    }
    /* TODO: system shock check for massive damage */
    skip_phase_four:

    /* Reduce hit points. */
    mHP = maxi (0, mHP - totaldamage);
    I->diag ("Dealt %d damage to %s (%p), now has %d / %d HP",
             totaldamage, the (), this, mHP, mMaxHP);

    /* Phase 5: After attack effects. */
    /* Test for specific weapons. */
    if (weapon) {
        switch (weapon->mIlkId) {
        case kObjBloodSword:
            /* Alive creatures are assumed to have blood or equivalent. */
            if (!isAlive ())  break;
            /* Skin was not pierced. */
            if (!totaldamage)  break;
            /* Bad for the sword and no enhancement increase. */
            if (intr (kAcidBlood) and !weapon->is (obj::fooproof)) {
                if (weapon->sufferDamage (kAttAcidBlood1, attacker, mX, mY)) {
                    if (weapon->mOwner) {
                        weapon->mOwner->unwield (weapon);
                        weapon->mOwner->removeObjectFromInventory (weapon);
                    }
                    delete weapon;
                }
                break;
            }
            /* At full potency already. */
            if (weapon->mEnhancement >= weapon->myIlk ()->mMaxEnhancement)
                break;
            /* Enhancement is increased on kills and 50% if it was negative. */
            if ((weapon->mEnhancement < 0 and RNG (2)) or mHP == 0) {
                /* Hero needs to witness both attacker and victim. */
                if (attacker and Hero.cr ()->canSee (attacker) and
                    Hero.cr ()->canSee (this))
                {
                    I->p ("%s drains %s!", THE (weapon), THE (this));
                    weapon->set (obj::known_type);
                    if (attacker->isHero ())
                        weapon->set (obj::known_enhancement);
                }
                ++weapon->mEnhancement;
            }
            break;
        default:
            break;
        }
    }
    /* Pain may wake up asleep creatures. */
    if (is (kAsleep)) {
        if (isHero ()) {
            if (RNG (1, 20) + totaldamage > 16)
                cure (kAsleep);
        } else {
            if (is (kAsleep) and RNG (1, 20) > 16)
                cure (kAsleep);
        }
    }
    /* Warn the player of impeding doom. */
    if (isHero () and intr (kHealthMonitoring) and mHP > 0 and
        mHP < hpWarningThreshold () and
        oldhp >= hpWarningThreshold ())
    {
        I->p ("You are about to die!");
        I->drawLog ();
    }
    /* Refresh hit points after every attack. */
    if (isHero ())  I->drawSideWin (this);
     /* Die if HP <= 0. */
    if (mHP <= 0) {
        if (isA (kMonBorg) and attacker and attacker->isHero ()) {
            /* The borg adapt to defend against the attacks you use
               against them!  I am so evil!! -- CADV */

            /* Too bad this is not saved. -- MB */
            ++myIlk ()->mNaturalArmorBonus;
#if 0
            for (i = 0; i < 2; i++) {
                shEnergyType energy = (shEnergyType) attack->mDamage[i].mEnergy;
                if (kNoEnergy = energy) {
                    break;
                }
                if (getResistance (energy) < 10) {
                    myIlk ()->mInnateResistances[energy] += 10;
                    break;
                }
            }
#endif
        }

        return 1;
    }
    if (attacker and !isHero () and mStrategy == kLurk) {
        /* Provoke attacked monsters - go hunting! */
        mTactic = kNewEnemy;
    }
    bool anydamage = (dmgtable[0] + dmgtable[1] + dmgtable[2]);
    if (attacker and attacker->isHero () and !isHero () and anydamage) {
        newEnemy (attacker); /* Get angry! */
    }
    mDisturbed = true;

    if (isA (kMonOrzScubaDiver) and attack->isMeleeAttack ())
        trn::ranged (this, 3, 6, 0);

    return 0;
}


void
shCreature::pDeathMessage (const char *monname, shCauseOfDeath how,
                           int presenttense /* = 0 */)
{
    if (kAnnihilated == how) {
        I->p ("%s is annihilated!", monname);
        return;
    }
    if (feat (kExplodes) and kSlain == how and !is (kFrozen)) {
        see (fmt ("%s explodes!", the ()))
            or (distance (Hero.cr (), mX, mY) <= 5 * 20
                and hear ("You hear an explosion!"))
            or I->p ("You hear an explosion in the distance.");
        return;
    }

    if (presenttense) {
        if (is (kFrozen))
            I->p ("You shatter %s!", monname);
        else if (Hero.cr ()->is (kXenosHunter) and isXenos ())
            I->p ("You purge %s!", monname);
        else if (Hero.cr ()->is (kXenosHunter) and isHeretic ())
            I->p ("You cleanse %s!", monname);
        else if (isAlive ())
            I->p ("You kill %s!", monname);
        else if (isRobot ())
            I->p ("You disable %s!", monname);
        else if (isProgram ())
            I->p ("You derez %s!", monname);
        else
            I->p ("You destroy %s!", monname);
    } else {
        if (is (kFrozen))
            I->p ("%s shatters!", monname);
        else if (Hero.cr ()->is (kXenosHunter) and isXenos ())
            I->p ("%s is purged!", monname);
        else if (Hero.cr ()->is (kXenosHunter) and isHeretic ())
            I->p ("%s is cleansed!", monname);
        else if (isAlive ())
            I->p ("%s is killed!", monname);
        else if (isRobot ())
            I->p ("%s is disabled!", monname);
        else if (isProgram ())
            I->p ("%s is derezzed!", monname);
        else
            I->p ("%s is destroyed!", monname);
    }
}


int
shCreature::die (shCauseOfDeath how, shCreature *killer,
    shObject *implement, shAttackId attack, const char *killstr)
{
    return die (how, killer, implement, &Attacks[attack], killstr);
}

int
shCreature::die (shCauseOfDeath how, shCreature *killer,
    shObject *implement, shAttack *attack, const char *killstr)
{
    debug.log ("%s (%p) is dead.", the (), this);
    mHowDead = how;
    if (kSuicide != how and !isHero ()) {
        ++myIlk ()->mKills;
        if (killer->isHero () and Hero.mProfession == Yautja and
            (isA (kMonAlienWarrior) or isA (kMonAlienQueen)) and
            killer->canSee (this))
        {
            I->p ("RAAWWWRRRR!");
        }
    } else if (isHero () and (killer == this or how == kSuicide)) {
        ++myIlk ()->mKills;
    }

    /* Dying while frozen does not trigger usual slain effects. */
    if (is (kFrozen) and how == kSlain)
        how = kKilled;

    /* Assume any death is hero's fault.  It almost always is.  Moreover this
       solution is easy to code, kludgeless and invites to use pets and kill
       monsters using tricky tactics.  Giving experience when that stupid
       tribble falls into a pit and dies on its own is a minor price to pay. */
    if (!(mConditions & kNoExp) and !isHero () and !isPet () and kSuicide != how) {
        /* Multipliers can be too easily farmed for XP.  Arbitrary cap. */
        if (!intr (kMultiplier) or mCLevel + 3 >= killer->mCLevel) {
            Hero.cr ()->beatChallenge (mCLevel);
        }
    }

    if (isPet ()) {
        Hero.mPets.remove (this);
        if (how != kSuicide and !Hero.cr ()->canSee (this))
            I->p ("You have a sad feeling for a moment, and then it passes.");
    }

    /* Once a new pass over creatures is made it will be deleted.
       See Game.cpp:gameLoop (). */
    mState = kDead;

    if (intr (kAcidBlood) and how == kSlain) {
        if (mZ == -1) {
            shFeature *f = mLevel->getFeature (mX, mY);
            if (f and f->mType == shFeature::kPit) {
                f->mType = shFeature::kAcidPit;
                if (Hero.cr ()->canSee (mX, mY)) {
                    I->p ("Acidic blood fills %s pit.",
                        f->mTrapUnknown ? "a" : "the");
                    f->mTrapUnknown = 0;
                }
            }
        } else {
            shAttackId blood = kAttAcidBlood1;
            /* Alien Queens spray much more blood than everyone else. */
            if (isA (kMonAlienQueen))  blood = kAttAcidBlood2;
            /* Very experienced Xel'Naga have stronger acid blood. */
            if (isA (kMonXelNaga) and mCLevel > 15)  blood = kAttAcidBlood2;
            Level->attackEffect (blood, NULL, mX, mY, kOrigin, this);
        }
    }

    bool goesboom = feat (kExplodes) and (how == kSlain or how == kSuicide);
    if (goesboom) {
        int glory_devices = 0;
        FOR_ALL_LOBES (l) {
            if (NOT0 (mImplants[l],->isA (kObjGloryDevice)))
                ++glory_devices;
        }
        shAttackId atk = kAttDummy;
        if (glory_devices) { /* The more the merrier. */
            atk = shAttackId (kAttGlory1 + glory_devices - 1);
        } else if (isA (kMonKohrAh)) {
            atk = kAttFlameNova;
        } else if (attack) {
            /* Find explode attack. */
            for (int i = 0; i < MAXATTACKS; ++i) {
                if (&Attacks[myIlk ()->mAttacks[i].mAttId] == attack) {
                    atk = myIlk ()->mAttacks[i].mAttId;
                    break;
                }
                if (&Attacks[myIlk ()->mRangedAttacks[i].mAttId] == attack) {
                    atk = myIlk ()->mAttacks[i].mAttId;
                    break;
                }
            }
            if (!atk) {
                atk = kAttExplodingMonster;
            }
        } else {
            atk = kAttExplodingMonster;
        }
        Level->attackEffect (atk, NULL, mX, mY, kOrigin, this, mCLevel);
    }

    if (isHero ())  Hero.death (how, killer, implement, attack, killstr);

    mWeapon = mJumpsuit = mBodyArmor = mCloak = mBoots =
        mHelmet = mGoggles = mBelt = NULL;
    for (int i = shObjectIlk::kFrontalLobe; i <= shObjectIlk::kCerebellum; ++i)
        mImplants[i] = NULL;

    int rbolt = 0;
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *obj = mInventory->get (i);
        if (obj->isA (kObjGloryDevice) and obj->is (obj::worn)) {
            delete obj; /* It explodes. */
            continue;
        }
        obj->clear (obj::worn);
        obj->clear (obj::wielded);
        if (obj->is (obj::unpaid) and !Level->isInShop (mX, mY)) {
            obj->clear (obj::unpaid);
        }
        if (!rbolt and obj->isA (kObjRestrainingBolt)) {
            rbolt = 1;
            if (!RNG (3) or
                obj->sufferDamage (kAttRestrainingBoltWear, killer, mX, mY))
            { /* Buggy bolts go awry surely.  Non-buggy sometimes do too. */
                delete obj;
                continue;
            }
        }
        if (GameOver) {
            delete obj;
        } else if (kAnnihilated == how and !obj->isIndestructible ()) {
            delete obj;
        } else {
            Level->putObject (obj, mX, mY);
        }
    }
    mInventory->reset ();

    if (how == kAnnihilated) {
        /* Nothing remains. */
    } else if (isA (kMonKillerRabbit)) {
        Level->putObject (new shObject (kObjRabbitFoot), mX, mY);
    } else if (isRobot () and !goesboom) {
        shObject *wreck;
        if (!isA (kMonSmartBomb)) {
            wreck = createWreck (this);
        } else {
            wreck = new shObject (kObjProximityMine);
        }
        Level->putObject (wreck, mX, mY);
    }

    Level->removeCreature (this);
    if (mLevel->rememberedCreature (mX, mY)) {
        mLevel->forgetCreature (mX, mY);
    }
    return 1;
}

/* Returns elapsed time in miliseconds.  Usually is instaneous. */
int
shCreature::revealSelf ()
{
    const char *a_thing;
    int mimic = 0;
    bool iswatery = mLevel->isWatery (mX, mY);
    shFeature *f = mLevel->getFeature (mX, mY);
    shObjectVector *v = mLevel->getObjects (mX, mY);

    bool known = mHidden < 0;
    mHidden = 0;
    if (feat (kBurrow) and !iswatery) { /* Unburrowing takes time. */
        appear (fmt ("%s unburrows%c", known ? the () : an (), known ? '.' : '!'))
        and Hero.interrupt (); /* Needed because the creature most likely
                                  will not attack immediately. */
        return LONGTURN;
    }
    if (known)
        return 0; /* No need to print a message since we knew it was there. */

    if (!Hero.cr ()->canSee (this))  return 0; /* Not seen - no message. */

    const char *a_monster = an ();

    if (mMimic == kMmObject) {
        mimic = 1;
        mMimic = kMmNothing;
        a_thing = mMimickedObject->mVague.mName;
    } else {
        mimic = 0;
        int found = 0;
        if (v and feat (kHideUnderObject)) {
            for (int i = 0; i < v->count (); ++i) {
                shObject *obj = v->get (i);
                if (canHideUnder (obj)) {
                    a_thing = obj->an ();
                    found = 1;
                    break;
                }
            }
        }
        if (found) {
            /* Skip. */
        } else if (f and canHideUnder (f)) {
            a_thing = f->the ();
        } else if (iswatery and feat (kHideUnderObject)) {
            a_thing = mLevel->getSquare (mX, mY) ->the ();
        } else { /* Impossible! */
            a_thing = "shroud of bugginess";
        }
    }

    if (mimic) {
        I->p ("The %s morphs into %s!", a_thing, a_monster);
    } else {
        I->p ("%s was hiding under %s!", a_monster, a_thing);
    }
    return 0;
}


/* check to see if the creature is getting irradiated
   returns: number of rads taken
   modifies: nothing
*/
int
shCreature::checkRadiation ()
{
    int res = 0;
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *obj = mInventory->get (i);
        if (obj->isRadioactive ())
            res += 2 * obj->mCount;
    }
    res += Level->isRadioactive (mX, mY);

    return res;
}

/* Returns number of turns creature is stuck for in a pit.
   -1 - can jump out at will
    0 - not in a pit
   >0 - actions needed to climb out */
int
shCreature::isInPit (void)
{
    if (mZ == -1 and mTrapped.mInPit) {
        if (intr (kJumpy))
            return -1;

        shFeature *f = mLevel->getFeature (mX, mY);

        if (intr (kCanSwim) and f and f->mType == shFeature::kSewagePit)
            return -1;

        if (intr (kFlying) and (!f or f->mType != shFeature::kSewagePit))
            return -1;

        return mTrapped.mInPit;
    }
    return 0;
}

bool
shCreature::isInShop ()
{
    return mLevel->isInShop (mX, mY);
}

int
shCreature::isTrapped ()
{
    return mTrapped.mInPit + mTrapped.mDrowning +
           mTrapped.mTaped + mTrapped.mWebbed;
}

bool
shCreature::isUnderwater ()
{
    if (mZ >= 0 or !mLevel) return false;
    shFeature *f = mLevel->getFeature (mX, mY);
    if (f and shFeature::kSewagePit == f->mType) return true;
    return false;
}

bool
shCreature::usesPower (shMutantPower power)
{
    if (power == kGammaSight and intr (kBlind))
        return false;
    return mMutantPowers[power] == MUT_POWER_ON;
}

int
shCreature::owns (shObject *obj)
{
    return mInventory->find (obj) >= 0;
}

/* Returns number of brain implants of this type installed. */
int
shCreature::hasInstalled (shObjId id)
{
    int cnt = 0;
    for (int i = shObjectIlk::kFrontalLobe; i <= shObjectIlk::kCerebellum; ++i)
        if (mImplants[i] and mImplants[i]->isA (id))
            ++cnt;
    return cnt;
}


int
shCreature::tryToFreeSelf ()
{
    if (feat (kSessile))  return FULLTURN;
    const char *s = !isHero () ? "s" : "";
    const char *y = !isHero () ? "ies" : "y";
    shFeature *f = mLevel->getFeature (mX, mY);
    shFeature::Type type = f ? f->mType : shFeature::kMaxFeatureType;
    bool seen = Hero.cr ()->canSee (this);

    if (f and isInPit ()) {
        f->mTrapMonUnknown = 0;
        if (seen)  f->mTrapUnknown = 0;
    }

    /* Problem to try take care about now.
       0 - none, 1 - webbing, 2 - taping, 3 - being in a pit */
    int problem = 0;

    /* One cannot get out of pit if webbed or taped. */
    if (mTrapped.mWebbed and mTrapped.mTaped) {
        problem = RNG (1, 2);
    } else if (mTrapped.mWebbed) {
        problem = 1;
    } else if (mTrapped.mTaped) {
        problem = 2;
    } else if (mTrapped.mInPit) {
        problem = 3;
    }

    switch (problem) {
    default:
    case 0:
        return 0;
    case 1:
        --mTrapped.mWebbed;
        if (!mTrapped.mWebbed and seen) {
            if (f and f->mType == shFeature::kWeb) {
                I->p ("%s free%s %s from the web.",
                      the (), s, herself ());
            } else {
                I->p ("%s break%s the webs holding %s!",
                      the (), s, herself ());
            }
        }
        return FULLTURN;
    case 2:
        --mTrapped.mTaped;
        if (!mTrapped.mTaped and seen) {
            I->p ("%s rip%s duct tape holding %s to pieces!",
                  the (), s, herself ());
        }
        return FULLTURN;
    case 3:
        if (!isHero () and type != shFeature::kSewagePit) {
            bool hasWeb = false;
            for (int i = 0; i < MAXATTACKS; ++i) {
                shAttackId atk = myIlk ()->mRangedAttacks[i].mAttId;
                if (atk and Attacks[atk].mType == shAttack::kWeb) {
                    hasWeb = true;
                    break;
                }
            }
            if (hasWeb)  /* Free yourself that way. */
                return shootWeb (kUp);
        }
        --mTrapped.mInPit;
        if (!mTrapped.mInPit)  mTrapped.mDrowning = 0;
        if (intr (kFlying) and type != shFeature::kSewagePit) {
            mTrapped.mInPit = 0;
            mTrapped.mDrowning = 0;
            mZ = 0;
            see (fmt ("%s fl%s out of %s.", the (), y,
                 f ? THE (f) : "the bugginess zone"));
            return FULLTURN;
        } else if (intr (kCanSwim) and type == shFeature::kSewagePit) {
            mTrapped.mInPit = 0;
            mTrapped.mDrowning = 0;
            mZ = 0;
            see (fmt ("%s swim%s to shallow sewage.", the (), s));
            return FULLTURN;
        } else if (intr (kJumpy)) {
            mTrapped.mInPit = 0;
            mTrapped.mDrowning = 0;
            mZ = 0;
            see (fmt ("%s jump%s out of %s.", the (), s,
                 f ? THE (f) : "the bugginess zone"));
            return FULLTURN;
        } else if (!mTrapped.mInPit and Hero.cr ()->canSee (this)) {
            if (!f) {
                I->p ("%s free%s %s from the bugginess zone.",
                    THE (this), isHero () ? "" : "s", this->herself ());
                return FULLTURN;
            }
            switch (type) {
            case shFeature::kPit:
            case shFeature::kAcidPit:
                I->p ("%s climb%s out of %s.", the (), s, THE (f));
                mTrapped.mInPit = 0;
                mZ = 0;
                break;
            case shFeature::kSewagePit:
                I->p ("%s swim%s to shallow sewage.", the (), s);
                mTrapped.mInPit = 0;
                mTrapped.mDrowning = 0;
                mZ = 0;
                break;
            default:
                I->p ("%s free%s %s from %s.", the (), s, herself (), THE (f));
                break;
            }
            return FULLTURN;
        }
        return FULLTURN;
    }

    /* It is error if this point is reached so make it long to be noticeable. */
    return LONGTURN * 5;
}

void
shCreature::removeObjectFromInventory (shObject *obj)
{
    using namespace obj;
    mInventory->remove (obj);
    mWeight -= obj->getMass ();
    if (obj->is (worn))
        doff (obj);

    obj->clear (worn | wielded | active | prepared | designated);
    if (mWeapon == obj)
        mWeapon = NULL;

    computeIntrinsics ();
    computeSkills ();
    computeAC ();
}


shObject *
shCreature::removeOneObjectFromInventory (shObject *obj)
{
    if (1 == obj->mCount) {
        removeObjectFromInventory (obj);
        return obj;
    } else {
        mWeight -= obj->myIlk ()->mWeight;
        return obj->split (1);
    }
}


void
shCreature::useUpOneObjectFromInventory (shObject *obj)
{
    if (obj->is (obj::unpaid)) {
        usedUpItem (obj, 1, "use");
    }
    if (1 == obj->mCount) {
        removeObjectFromInventory (obj);
        delete obj;
    } else {
        --obj->mCount;
        mWeight -= obj->myIlk ()->mWeight;
        computeBurden ();
    }
}


//RETURNS: number of objects used
int
shCreature::useUpSomeObjectsFromInventory (shObject *obj, int cnt)
{
    if (obj->mCount > cnt) {
        obj->mCount -= cnt;
        mWeight -= cnt * obj->myIlk ()->mWeight;
        if (obj->is (obj::unpaid)) {
            usedUpItem (obj, cnt, "use");
        }
        computeBurden ();
        return cnt;
    } else {
        cnt = obj->mCount;
        if (obj->is (obj::unpaid)) {
            usedUpItem (obj, cnt, "use");
        }
        removeObjectFromInventory (obj);
        delete obj;
        return cnt;
    }
}


shObject *
shCreature::removeSomeObjectsFromInventory (shObject *obj, int cnt)
{
    if (obj->mCount > cnt) {
        obj = obj->split (cnt);
        mWeight -= cnt * obj->myIlk ()->mWeight;
        computeBurden ();
        return obj;
    } else {
        removeObjectFromInventory (obj);
        return obj;
    }
}


int
shCreature::countEnergy (int *tankamount)
{
    shObjectVector v;
    int res = 0;

    /* Plain energy cells. */
    selectObjects (&v, mInventory, kObjEnergyCell);
    for (int i = 0; i < v.count (); i++)
        res += v.get (i) -> mCount;

    /* Energy containers. */
    v.reset ();
    selectObjects (&v, mInventory, power_plant);
    selectObjects (&v, mInventory, energy_tank);
    if (mBelt and mBelt->isA (kObjEnergyBelt))  v.add (mBelt);
    if (mBodyArmor and mBodyArmor->isA (kObjTeslaSuit))  v.add (mBodyArmor);
    for (int i = 0; i <= shObjectIlk::kCerebellum; ++i)
        if (mImplants[i] and mImplants[i]->isA (kObjShockCapacitor))
            v.add (mImplants[i]);

    /* Count power in containers. */
    if (tankamount) *tankamount = 0;
    for (int i = 0; i < v.count (); i++) {
        shObject *obj = v.get (i);
        res += obj->mCharges;
        if (tankamount and obj->is (obj::known_type)) {
            *tankamount += obj->myIlk ()->mMaxCharges;
        }
    }

    return res;
}

/* Tries to extract 'cnt' energy from items contained in 'v'. */
static int  /* Returns # of energy left to extract from elsewhere. */
drawEnergy (shObjectVector *v, int cnt, bool isHero, int unpaid_allowed)
{
    for (int i = 0; i < v->count (); i++) {
        shObject *obj = v->get (i);
        if (isHero and obj->is (obj::unpaid)) {
            if (!unpaid_allowed) {
                continue;
            } else if (obj->mCharges) {
                Hero.cr ()->usedUpItem (obj, 1, "discharge");
                obj->clear (obj::unpaid);
            }
        }

        if (obj->mCharges > cnt) {
            obj->mCharges -= cnt;
            return 0;
        } else {
            cnt -= obj->mCharges;
            obj->mCharges = 0;
        }
    }
    return cnt;
}

/* Expends some energy from the certain sources, which are enumerated
   in the procedure.  First tries owned things but if that is not enough
   second pass is performed where unpaid items are also considered.
*/
int  /*  Returns: # of energy units spent.  */
shCreature::loseEnergy (int cnt)
{
    int goal = cnt;
    shObjectVector v;
    for (int pass = 0; pass < 2; ++pass) {
        /* 1) Worn tesla armor and installed shock capacitors. */
        if (mBodyArmor and mBodyArmor->isA (kObjTeslaSuit))  v.add (mBodyArmor);
        for (int i = 0; i <= shObjectIlk::kCerebellum; ++i)
            if (mImplants[i] and mImplants[i]->isA (kObjShockCapacitor))
                v.add (mImplants[i]);
        cnt = drawEnergy (&v, cnt, isHero (), pass);
        if (!cnt)
        v.reset ();

        /* 2) Self-recharging power plants. */
        selectObjects (&v, mInventory, power_plant);
        cnt = drawEnergy (&v, cnt, isHero (), pass);
        v.reset ();

        /* 3) Unprotected energy cells. */
        selectObjects (&v, mInventory, kObjEnergyCell);
        for (int i = 0; i < v.count (); i++) {
            shObject *obj = v.get (i);
            int unpaid = isHero () and obj->is (obj::unpaid);
            if (0 == pass and unpaid) {
                continue;
            }
            cnt -= useUpSomeObjectsFromInventory (obj, cnt);
            if (cnt == 0)
                return goal;
        }
        v.reset ();

        /* 4) Chargeable energy sources: energy belt and energy tanks. */
        if (mBelt and mBelt->isA (kObjEnergyBelt)) v.add (mBelt);
        selectObjects (&v, mInventory, energy_tank);
        cnt = drawEnergy (&v, cnt, isHero (), pass);
        v.reset ();
    }
    return goal - cnt;
}


void
shCreature::gainEnergy (int amount)
{
    shObjectVector v;
    int i;

    selectObjects (&v, mInventory, power_plant);
    for (i = 0; i < v.count (); i++) {
        shObject *obj = v.get (i);

        if (obj->mCharges < 100) {
            obj->mCharges += amount;
            if (obj->mCharges > 100) {
                amount -= (obj->mCharges - 100);
                obj->mCharges = 100;
            } else {
                return;
            }
        }
    }
}


int
shCreature::countMoney ()
{
    shObjectVector v;
    int res = 0;
    int i;

    selectObjects (&v, mInventory, kMoney);
    for (i = 0; i < v.count (); i++) {
        res += v.get (i) -> mCount;
    }
    return res;
}


int
shCreature::loseMoney (int amount)
{
    shObjectVector v;
    int res = 0;
    int i;

    selectObjects (&v, mInventory, kMoney);
    for (i = 0; i < v.count (); i++) {
        shObject *obj = v.get (i);
        int tmp = useUpSomeObjectsFromInventory (obj, amount);
        amount -= tmp;
        res += tmp;
    }
    return res;
}


int
shCreature::gainMoney (int amount)
{
    shObject *obj = new shObject (kObjMoney);
    obj->mCount = amount;
    addObjectToInventory (obj, 1);
    return amount;
}


/* Returns 1 if successful. */
int
shCreature::wield (shObject *obj, int quiet /* = 0 */ )
{
    shObject *oldweapon = mWeapon;
    if (mWeapon) {
        if (mWeapon == obj) {
            if (!quiet)  msg ("You're wielding that already!");
            return 0;
        }
        if (mWeapon->isWeldedWeapon () and !isOrc ()) {
            bool seen = !quiet and
                (msg (fmt ("%s is welded to your hand!", YOUR (mWeapon)))
                 or appear (fmt ("%s tries to get rid of %s unsuccessfully.",
                            THE (this), mWeapon->her (this))));
            if (seen)
                mWeapon->set (obj::known_bugginess);
            return 0;
        }
        if (!unwield (mWeapon))
            return 0;
    }
    if (!obj) {
        if (!oldweapon) {
            if (!quiet)  msg ("You're already unarmed.");
            return 0;
        } else {
            if (!quiet) {
                msg ("You are unarmed.")
                or appear (fmt ("%s puts %s weapon away.", THE (this), her ()));
            }
            if (isHero ())  Hero.resetStoryFlag ("strange weapon message");
            mWeapon = NULL;
            return 1;
        }
    } else if (obj->is (obj::worn)) {
        msg ("You can't wield that because you're wearing it!");
        return 0;
    } else {
        mWeapon = obj;
        obj->set (obj::wielded);
        if (isHero ()) {
            obj->clear (obj::prepared);
            Hero.resetStoryFlag ("strange weapon message");
        }
        computeSkills (); /* Compute before checking for Smartgun. */
        if (quiet) {
            /* No message at all. */
        } else if (msg (fmt ("You now wield %s.", AN (obj)))) {
            /* The action was taken by hero. */
        } else if (!mHidden and Hero.cr ()->canSee (this) and mWeapon) {
            /* See enemy ready weapon. */
            mWeapon->set (obj::known_appearance);
            if (isRobot () and mWeapon->isThrownWeapon ()) {
                I->p ("%s loads %s!", the (), mWeapon->an ());
            } else {
                if (mWeapon->isA (kObjLightSaber))
                    mWeapon->set (obj::known_type);
                I->p ("%s wields %s!", the (), mWeapon->an ());
            }
            Hero.interrupt ();
        }

        if (!isHero ()) {
            /* No messages or revelations for monsters. */
        } else if (obj->isA (kObjCryolator)) {
            obj->set (obj::known_charges);  /* It has ammo display. */
        } else if (obj->isA (kObjChaosSword)) {
            obj->setBuggy();
            if (!quiet)
                I->p ("You hear maniacal laughter in the distance.");
        } else if (obj->isA (kObjM56Smartgun) or obj->isA (kObjM57Smartgun)) {
            if (!intr (kBlind) and !obj->is (obj::known_type)) {
                if (!quiet) {
                    I->p ("You find out this weapon has automatic aim module.");
                }
                obj->set (obj::known_type);
            }
            if (!quiet and obj->isA (kObjM57Smartgun)) {
                shSkillCode c = obj->myIlk ()->mGunSkill;
                shSkill *skill = getSkill (c);
                if (skill and skill->mBonus < 0) {
                    I->p ("Auto-aim hampers you because your skill is greater.");
                }
            }
        } else if (obj->isA (kObjKhaydarin) and obj->mCharges) {
            if (!is (kConfused) and !is (kStunned)) {
                if (!quiet)
                    I->p ("Your mind finds peace.");
                obj->set (obj::known_charges);
            }
        }
        computeIntrinsics ();
        return 1;
    }
    //return 0;
}


int
shCreature::unwield (shObject *obj)
{
    assert (obj == mWeapon);
    mWeapon = NULL;
    obj->clear (obj::wielded);
    if (isHero ())  Hero.resetStoryFlag ("strange weapon message");
    computeIntrinsics ();
    computeSkills ();
    return 1;
}


void
shCreature::drop (shObject *obj)
{
    msg (fmt ("You drop %s.", AN (obj)));
    obj->clear (obj::thrown);
    obj->mOwner = NULL;
    obj = Level->putObject (obj, mX, mY);
    if (obj and Level->isInShop (mX, mY))
        maybeSellItem (obj);
}


int
shCreature::don (shObject *obj, int quiet /* = 0 */ )
{
    shIntrinsicData before;
    before.clone (mIntrinsics);
    unsigned int encumbrance = mConditions & kOverloaded;

    if (obj->has_subtype (body_armor) and NULL == mBodyArmor) {
        mBodyArmor = obj;
    } else if (obj->has_subtype (jumpsuit) and NULL == mJumpsuit) {
        mJumpsuit = obj;
    } else if (obj->has_subtype (helmet) and NULL == mHelmet) {
        shObject *imp = mImplants[shObjectIlk::kCerebellum];
        if (imp and imp->isA (kObjMechaDendrites) and obj->isSealedArmor ()) {
            if (!quiet and isHero ()) {
                I->p ("%s conflict with %s.", YOUR (imp), obj->theQuick ());
            }
            return 0;
        }
        if (mGoggles and obj->isA (kObjBioMask)) {
            if (!quiet and isHero ()) {
                I->p ("%s conflict with %s.", YOUR (mGoggles), obj->theQuick ());
            }
            return 0;
        }
        mHelmet = obj;
    } else if (obj->has_subtype (goggles) and NULL == mGoggles) {
        if (mHelmet and mHelmet->isA (kObjBioMask)) {
            if (!quiet and isHero ()) {
                I->p ("%s conflicts with %s.", YOUR (mHelmet), obj->theQuick ());
            }
            return 0;
        }
        mGoggles = obj;
    } else if ((obj->has_subtype (cloak) or obj->isA (kObjPlasmaCaster)) and
               NULL == mCloak)
    {
        mCloak = obj;
    } else if (obj->has_subtype (boots) and NULL == mBoots) {
        mBoots = obj;
    }  else if (obj->has_subtype (belt) and NULL == mBelt) {
        mBelt = obj;
    } else if (obj->isA (kImplant)) {
        shObjectIlk::Site site = obj->myIlk ()->mSite;
        if (shObjectIlk::kAnyBrain == site) {
            int i;
            for (i = 0; i <= shObjectIlk::kCerebellum; ++i) {
                if (!mImplants[i]) {
                    site = (shObjectIlk::Site) i;
                    break;;
                }
            }
            if (i > shObjectIlk::kCerebellum) {
                if (isHero () and !quiet) {
                    I->p ("There's no more room in your brain for this implant!");
                }
                return 0;
            }
        } else if (shObjectIlk::kAnyEar == site) {
            site = shObjectIlk::kLeftEar;
        } else if (shObjectIlk::kAnyEye == site) {
            site = shObjectIlk::kRightEyeball;
        }

        if (mImplants[site]) {
            if (isHero () and !quiet) {
                if (site == shObjectIlk::kNeck) {
                    I->p ("%s's field repels the new %s!",
                        YOUR(mImplants[site]), obj->inv ());
                } else {
                    I->p ("You already have an implant installed in your %s.",
                          describeImplantSite (site));
                }
            }
            return 0;
        }
        if (obj->isA (kObjMechaDendrites) and
            mHelmet and mHelmet->isSealedArmor ())
        {
            return 0; /* Quietly because this condition can only be triggered */
        }             /* by automatic donning at the start of new game. */
        /* Wearing two Excruciators is instant death. */
        if (obj->isA (kObjExcruciator) and hasInstalled (kObjExcruciator) and
            obj->is (obj::known_type))
        {
            if (isHero ())
                I->p ("You cannot bring yourself to wear another Excruciator.");
            return 0;
        }
        mImplants[site] = obj;
        if (isHero ()) {
            if (!quiet) {
                I->p ("You install %s in your %s.", THE (obj),
                    describeImplantSite (site));
            }
            obj->mImplantSite = site;
        }
    } else {
        return 0;
    }
    obj->set (obj::worn);
    computeIntrinsics (quiet);
    computeSkills ();
    computeAC ();
    if (isHero ()) {
        if (!intr (kCamouflaged) and before.get (kCamouflaged)) {
            if (!quiet) {
                I->p ("Covering %s deprives you of camouflage.",
                    obj != mJumpsuit and mJumpsuit ? YOUR (mJumpsuit) : "your body");
            }
        }
        if (obj->isA (kArmor) and obj->isEnhanceable ()) {
                obj->set (obj::known_enhancement);
        }
        if (obj->isA (kObjHealthMonitor) and !(before.get (kHealthMonitoring))) {
            obj->set (obj::known_type);
        } else if (obj->isA (kObjTissueRegenerator)) {
            mLastRegen = Clock;
        } else if (obj->isA (kObjMechaDendrites)) {
            obj->set (obj::known_bugginess);
        } else if (obj->has_subtype (skillsoft) and !obj->isBuggy ()) {
            if (!quiet) {
                I->p ("You gain access to technical knowledge database.");
            }
            obj->set (obj::known_type);
            obj->set (obj::known_bugginess);
        } else if (obj->isA (kObjChameleonSuit)) {
            if (intr (kCamouflaged) and !(before.get (kCamouflaged))) {
                if (!mBodyArmor and !mCloak) {
                    if (!quiet)  I->p ("You blend in with your surroundings.");
                    obj->set (obj::known_type);
                } else if (obj->is (obj::known_type) and !quiet) {
                    /* FIXME: Above condition is not enough.
                              This is never printed. */
                    I->p ("You would blend in with your surroundings if you hadn't covered your suit.");
                }
            }
        } else if (obj->isA (kObjUngooma)) {
            if (!obj->is (obj::known_type)) {
                I->p ("You got yourself a%s parasitic friend it seems.",
                    hasInstalled (kObjUngooma) > 1 ? "nother" : "");
                obj->set (obj::known_type);
            }
            obj->set (obj::known_bugginess);
        } else if (obj->isA (kObjSuspensorBelt)) {
            /* Is revealed by change to encumbrance. */
            if (encumbrance != (mConditions & kOverloaded))
                obj->set (obj::known_type);
        } else if (obj->isA (kObjStabilizerBelt)) {
            /* As above. */
            if (encumbrance != (mConditions & kOverloaded)) {
                obj->set (obj::known_type);
            }
            /* +2 to heavy guns is noticeable.  If it gives +1 it could be
               either buggy or debugged so bugginess is not revealed yet. */
            if (obj->is (obj::known_type) and obj->isOptimized ())
                obj->set (obj::known_bugginess);
        } else if (obj->isA (kObjEnergyBelt)) {
            /* No message. Energy display indicates what item is this. */
            obj->set (obj::known_type);
            obj->set (obj::known_charges);
        } else if (obj->isA (kObjShieldBelt) and !(before.get (kShielded))) {
            if (!quiet) {
                if (!intr (kBlind)) {
                    I->p ("You are surrounded by a force field.");
                } else {
                    I->p ("There is a feeling of stasis around you.");
                }
            }
            obj->set (obj::known_type | obj::active);
            computeIntrinsics (quiet);
        /*
        } else if (obj->isA ("jetpack") and !(before & kFlying)) {
            if (!quiet) {
                I->p ("You zoom into the air!");
            }
            obj->set (obj::known_type);
            mTrapped.mInPit = 0;
            mTrapped.mDrowning = 0;
        */
        } else if (obj->isA (kObjXRayGoggles) and !intr (kBlind) and
                   !(before.get (kXRayVision)))
        {
            if (!quiet) {
                I->p ("You can see through things!");
            }
            obj->set (obj::known_type);
        } else if (obj->isA (kObjCerebralCoprocessor) or
                   obj->isA (kObjMotoricCoordinator) or
                   obj->isA (kObjReflexCoordinator) or
                   obj->isA (kObjAdrenalineGenerator) or
                   obj->isA (kObjMyfilamentHypertensor) or
                  (obj->isA (kObjPsiAmp) and Hero.isPsyker ()))
        {
            if (obj->mEnhancement) { /* Stat mod gives it away. */
                obj->set (obj::known_type);
                obj->set (obj::known_enhancement);
            } else if (obj->is (obj::known_type)) { /* Identify +0 if you know type. */
                obj->set (obj::known_enhancement);
            }
            /* Psionic amplifier:
               Usually identifying a +0 would require one to check percentage
               chances of mutant powers.  Psions are considered so expert at
               noticing psionic aura disturbances they can know psionic
               amplifiers just by having them interact with their brain. */
        } else if (obj->isA (kObjShockCapacitor)) {
            if (obj->is (obj::known_type))  obj->set (obj::known_charges);
            if (obj->isBuggy () and obj->mCharges and RNG (2)) {
                I->p ("KZAPP!  Your brain is shocked!");
                obj->set (obj::known_bugginess);
                obj->set (obj::known_type);
                obj->set (obj::known_charges);
                if (sufferAbilityDamage (abil::Int, 1)) {
                    die (kMisc, "fried his brain");
                    return 1;
                }
                checkConcentration ();
                inflict (kConfused, FULLTURN * maxi (3, RNG (obj->mCharges)));
                obj->mCharges = 0;
            }
        } else if (obj->isA (kObjExcruciator)) {
            if (hasInstalled (kObjExcruciator) == 1) {
                I->p ("Unbelieveable pain torments you!");
                if (is (kViolated))  cure (kViolated);
                inflict (kParalyzed, FULLTURN * 5);
            } else {
                I->p ("The implant turns out to be another Excruciator.");
                I->p ("Your suffering is increased exponentially pushing you into agony.");
                die (kKilled, "pain induced agony");
            }
            obj->set (obj::known_type);
        } else if (obj->isA (kObjStormtrooperHelmet) and !intr (kBlind)) {
            if (!quiet) {
                I->p ("It's hard to see out of this helmet.");
            }
            obj->set (obj::known_type);
        } else if (obj->isA (kObjTeslaSuit) and obj->is (obj::known_type)) {
            obj->set (obj::known_charges);
        } else if (obj->isA (kObjRedCPA)) {
            if (!quiet) {
                I->p ("A voice roars in your head:");
                I->p ("Blood for the blood god, %s!", Hero.getTitle ());
            }
            obj->identify ();
            obj->setBuggy ();
        } else if (obj->isA (kObjPinkCPA)) {
            if (!quiet) {
                I->p ("A voice whispers in your ear:");
                I->p ("Pleasure me, %s!", Hero.getTitle ());
            }
            obj->identify ();
            obj->setBuggy ();
        } else if (obj->isA (kObjBlueCPA)) {
            if (!quiet) {
                I->p ("You hear a thousand silent voices.");
            }
            obj->identify ();
            obj->setBuggy ();
        } else if (obj->isA (kObjGreenCPA)) {
            if (!quiet) {
                I->p ("%s gurgles and speaks:", obj->the());
                I->p ("Come bask in my filth, lowly %s.", Hero.getTitle ());
            }
            obj->identify ();
            obj->setBuggy ();
        } else if (obj->isA (kObjBioArmor)) {
            if (!quiet) {
                I->p ("%s quivers and envelops your body!.", obj->the());
            }
            obj->set (obj::known_type);
        } else if (obj->isA (kObjTorc) and !obj->isBuggy ()) {
            Hero.torcCheck ();
        }
        obj->clear (obj::worn);
        if (!quiet)
            obj->announce ();
        obj->set (obj::worn);
    }
    return 1;
}


int
shCreature::doff (shObject *obj, int quiet /* = 0 */ )
{
    shIntrinsicData before;
    before.clone (mIntrinsics);
    unsigned int encumbrance = mConditions & kOverloaded;

    if (obj == mBodyArmor) {
        mBodyArmor = NULL;
    } else if (obj == mJumpsuit) {
        mJumpsuit = NULL;
    } else if (obj == mHelmet) {
        mHelmet = NULL;
    } else if (obj == mGoggles) {
        mGoggles = NULL;
	} else if (obj == mCloak) {
		mCloak = NULL;
    } else if (obj == mBelt) {
        mBelt = NULL;
	} else if (obj == mBoots) {
		mBoots = NULL;
    } else {
        int i;
        for (i = 0; i < shObjectIlk::kMaxSite; ++i) {
            if (obj == mImplants[i]) {
                mImplants[i] = NULL;
                break;
            }
        }
        if (i == shObjectIlk::kMaxSite) return 0;
    }

    obj->clear (obj::worn);
    obj->clear (obj::active);
    computeIntrinsics (quiet);
    computeSkills ();
    computeAC ();
    if (isHero ()) {
        if (obj->isA (kObjTissueRegenerator)) {
            mLastRegen = MAXTIME;
        } else if (obj->isA (kObjChameleonSuit)) {
            if (!quiet and hasCamouflage ())
                I->p ("You no longer blend in with your surroundings.");
        /*
        } else if (obj->isA ("jetpack")) {
            if (!isFlying ()) {
                obj->set (obj::known_type);
                I->p ("You float down.");
            }
        */
        } else if (obj->isA (kObjSuspensorBelt)) {
            /* Is revealed by change to encumbrance. */
            if (encumbrance != (mConditions & kOverloaded))
                obj->set (obj::known_type);
        } else if (obj->isA (kObjStabilizerBelt)) {
            /* As above. */
            if (encumbrance != (mConditions & kOverloaded)) {
                obj->set (obj::known_type);
            }
            /* Bugginess revealed always because having the ability to remove
               the belt means it has to be debugged if it grants +1 to heavy
               guns.  Unless you are an orc in which case only optimized belts
               status can be deduced by taking it off. */
            if (obj->is (obj::known_type) and
                (!isOrc () or obj->isOptimized ()))
            {
                obj->set (obj::known_bugginess);
            }
        } else if (obj->isA (kObjStormtrooperHelmet) and !intr (kBlind)) {
            I->p ("You can see much better now.");
        } else if (obj->isA (kObjTorc) and !obj->isBuggy () and
                   !hasInstalled (kObjExcruciator))
        {
            I->p ("The torc's rare earth circuits are singed as you remove it from your neck!");
            if (!intr (kBlind))  I->p ("Its color deadens.");
            if (sufferDamage (kAttTorcRemoval)) {
                char *buf = GetBuf ();
                snprintf (buf, SHBUFLEN, "removing %s", THE (obj));
                die (kKilled, buf);
            }
            --obj->mBugginess;
        }
        if (intr (kCamouflaged) and !(before.get (kCamouflaged))) {
            if (!quiet) {
                I->p ("By uncovering %s you %sgain camouflage.",
                    mJumpsuit ? YOUR (mJumpsuit) : "your body",
                    mJumpsuit and !mJumpsuit->is (obj::known_type) ? "" : "re");
                if (mJumpsuit and !mJumpsuit->is (obj::known_type)) {
                    mJumpsuit->set (obj::known_type);
                    mJumpsuit->announce ();
                }
            }
        }
    }
    return 1;
}

/* If a creature gets hit with certain energy type what items get affected? */
void
shCreature::damageEquipment (shAttack *attack, shEnergyType energy)
{
    shObjectVector v;
    shObject *obj = NULL;

    if (kCorrosive == energy) {
        if (mWeapon)  v.add (mWeapon);
        if (mCloak) {
            v.add (mCloak);
        } else if (mBodyArmor) {
            v.add (mBodyArmor);
        } else if (mJumpsuit) {
            v.add (mJumpsuit);
        }
        if (mBoots)  v.add (mBoots);
        if (mBelt)  v.add (mBelt);
		if (mCloak)  v.add (mCloak);
        if (mHelmet)  v.add (mHelmet);
        if (mGoggles)  v.add (mGoggles);
    } else if (kElectrical == energy) {
        if (RNG (2))
            /* Only unprotected energy cells are subject to shortening out. */
            for (int i = 0; i < mInventory->count (); ++i) {
                shObject *obj = mInventory->get (i);
                if (obj and obj->isA (kObjEnergyCell)) {
                    v.add (obj);
                    /* Be content with finding a single stack of cells because
                       if all of them were picked up for attack players might
                       be tempted to split their cells to protect implants.
                       This would be boring. */
                    break;
                }
            }
        /* Helmet granting resistance to electricity protects implants. */
        if (!mHelmet or mHelmet->myIlk ()->mResistances[kElectrical] <= 0) {
            /* Any shock capacitors are last implants to be ejected
               because these have greatest electricity tolerance. */
            int impl = 0;
            for (int i = 0; i < shObjectIlk::kMaxSite; i++)
                if (mImplants[i] and !mImplants[i]->isA (kObjShockCapacitor)
                    and !mImplants[i]->isA (kObjUngooma)) {
                    v.add (mImplants[i]);
                    ++impl;
                }
            if (!impl)  for (int i = 0; i < shObjectIlk::kMaxSite; i++)
                if (mImplants[i] and !mImplants[i]->isA (kObjUngooma))
                    v.add (mImplants[i]);
        }
    } else if (kMagnetic == energy) { /* Get everything. */
        for (int i = 0; i < mInventory->count (); ++i)
            v.add (mInventory->get (i));
    } else if (kBurning == energy or kPlasma == energy) {
        if (mCloak) {
            v.add (mCloak);
        } else if (mBodyArmor) {
            v.add (mBodyArmor);
        } else if (mJumpsuit) {
            v.add (mJumpsuit);
        }
        if (mHelmet) v.add (mHelmet);
        if (mBelt) v.add (mBelt);
        if (mBoots) v.add (mBoots);
		if (mCloak) v.add (mCloak);
        if (mGoggles) v.add (mGoggles);
        if (!RNG (3))  selectObjects (&v, mInventory, kFloppyDisk);
    } else if (kFreezing == energy) {
        if (!RNG (3))  selectObjects (&v, mInventory, kCanister);
    } else if (kBugging == energy) {
        selectObjectsByFunction (&v, mInventory, &shObject::isBuggy, true);
    } else if (shAttack::is_toxin (energy)) {
      /* For reticulan bio armor and green chaos power armor. */
        if (mBodyArmor)  v.add (mBodyArmor);
    }

    /* Bio armors respond to restoration, healing and plague. */
    if (attack->mType == shAttack::kRestorationRay or
        attack->mType == shAttack::kHealingRay or
        attack->mType == shAttack::kPlague)
    {
        if (mBodyArmor)  v.add (mBodyArmor);
    }

    /* Choose one victim randomly. */
    if (v.count ()) {
        obj = v.get (RNG (v.count ()));
        int num;
        if ((num = obj->sufferDamage (energy, attack, NULL, mX, mY))) {
            usedUpItem (obj, num, "destroy");
            delete removeOneObjectFromInventory (obj);
        }
    }
    computeIntrinsics ();
    computeAC ();
}


shObject *
shCreature::getKeyForDoor (shFeature *f)
{
    shObjectIlk *keyneeded = f->keyNeededForDoor ();
    shObjectVector mykeys;

    /* Try your keycards first. */
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *obj = mInventory->get (i);
        if (obj->has_subtype (keycard) and
            /* It is in working condition or you are not sure. */
            (!obj->isBuggy () or !obj->is (obj::known_bugginess)) and
           ( /* Unlocks all doors. */
            (obj->isA (kObjMasterKeycard) and obj->is (obj::known_type)) or
             /* Unlawfully unlocks all doors. */
            (obj->is (obj::cracked | obj::known_cracked)) or
             /* Is right key */
            (obj->isA (keyneeded->mId) and
            /* You see door lock color and keycard color or... */
             (!intr (kBlind) or
            /* ...you know both lock color and this keycard color. */
              (obj->is (obj::known_appearance) and
               (Level->mRemembered[f->mX][f->mY].mDoor & shFeature::kColorCode))
             ))
           ))
            mykeys.add (obj);
    }

    if (mykeys.count ()) { /* Pick random fitting key for fun. */
        int choose = RNG (mykeys.count ());
        return mykeys.get (choose);
    }

    mykeys.reset (); /* Time to try breaching security. */
    selectObjects (&mykeys, mInventory, kObjLockPick);
    if (NOT0 (mImplants[shObjectIlk::kCerebellum],->isA (kObjMechaDendrites)))
        mykeys.add (mImplants[shObjectIlk::kCerebellum]);

    if (mykeys.count ()) {
        int choose = RNG (mykeys.count ());
        return mykeys.get (choose);
    }
    return NULL;
}


//RETURNS: 1 if successful, 0 o/w
int
shCreature::openDoor (int x, int y)
{
    shFeature *f = mLevel->getFeature (x,y);
    if (shFeature::kDoorClosed != f->mType or
        !isAdjacent (x, y))
    {
        return 0;
    }
    if (0 == numHands () and !isA (kMonDalek)) {
        return 0;
    }
    if (shFeature::kLocked & f->mDoor.mFlags) {
        if (isHero ()) {
            if (f->isLockBrokenDoor ()) {
                I->p ("The door is held shut by a broken lock!");
            } else if (f->isRetinaDoor ()) {
                I->p ("\"Initiating retina scan...\"");
                I->pause ();
                if (mImplants[shObjectIlk::kRightEyeball] and
                    mImplants[shObjectIlk::kRightEyeball]
                    ->isA (kObjBOFHEye))
                {
                    I->p ("\"Identification positive.  "
                          "Welcome, Mr. Operator!\"");
                    mImplants[shObjectIlk::kRightEyeball]->set (obj::known_type);
                    f->unlockDoor ();
                    f->mType = shFeature::kDoorOpen;
                    f->mDoor.mMoveTime = Clock;
                    return 1;
                } else {
                    I->p ("\"Identification negative.  Access denied.\"");
                }
            } else {
                /* Safe, will not return NULL because lock broken
                   case is handled above. */
                shObjectIlk *keyneeded = f->keyNeededForDoor ();
                shObject *key = getKeyForDoor (f);
                if (key) {
                    return useKey (key, f);
                }
                I->p ("The door is locked.  There is %s lock.",
                    intr (kBlind) ? "some kind of keycard"
                               : keyneeded->mAppearance.mName);
                I->p ("You do not seem to have anything to unlock it with.");
                I->p ("Maybe try kicking it (with '%s' key) or shooting it?",
                      I->getKeyForCommand (shInterface::kKick));
            }
        } else if (isA (kMonDalek)) {
            /* Daleks can lock pick doors with intact locks but not retina
               scanners.  Daleks are said to perform a million of operations
               per second according to some Doctor Who episode. */
            if (!f->isRetinaDoor () and !f->isLockBrokenDoor ())
            { /* Unlock! */
                f->unlockDoor ();
                if (Hero.cr ()->canSee (x, y)) {
                    I->p ("The lock on the door emits a cascade of lights and unlocks!");
                    I->pauseXY (x, y);
                }
                return 1;
            }
        }
        return 0;
    }
    f->mType = shFeature::kDoorOpen;
    f->mDoor.mMoveTime = Clock;
    if (mLevel == Hero.cr ()->mLevel)
        Level->computeVisibility (Hero.cr ());
    return 1;
}


//RETURNS: 1 if successful, 0 o/w
int
shCreature::closeDoor (int x, int y)
{
    shFeature *f = mLevel->getFeature (x,y);
    if ((shFeature::kDoorOpen != f->mType and !isHero ()) or
        !isAdjacent (x, y) or
        0 != mLevel->countObjects (x, y))
    {
        return 0;
    }
    if (0 == numHands ()) {
        return 0;
    }
    if (shFeature::kDoorOpen == f->mType) { /* Close a door. */
        f->mType = shFeature::kDoorClosed;
        f->mDoor.mMoveTime = Clock;
        if (f->isRetinaDoor ())
            f->lockDoor ();
        if (mLevel == Hero.cr ()->mLevel)
            Level->computeVisibility (Hero.cr ());
    } else { /* Hero wishes to lock a closed door. */
        if (!f->isLockDoor ()) {
            I->p ("It's already closed.");
            return 0;
        }
        if (f->isLockBrokenDoor ()) {
            I->p ("Its lock is broken.  You need to fix it before you can lock it.");
            return 0;
        }
        if (f->isLockedDoor ()) {
            I->p ("It's already closed and locked.");
            return 0;
        }
        if (f->isRetinaDoor ()) {
            I->p ("You reactivate the door's retina scanner.");
            f->lockDoor ();
            return 1;
        }
        shObject *key = getKeyForDoor (f);
        if (key) {
            return useKey (key, f);
        }

        I->p ("You do not seem to have any means to lock this door.");
        shObjectIlk *keyneeded = f->keyNeededForDoor ();
        I->p ("It has %s lock.", intr (kBlind) ? "some kind of keycard"
                                            : keyneeded->mAppearance.mName);
    }
    return 1;
}


/* returns true if the creature's movement's would trigger a motion detector */
int
shCreature::isMoving ()
{
    if (mHidden) {
        return 0;
    }
    if (mLastMoveTime + 2500 > Clock) {
        return 1;
    }
    switch (myIlk ()->mType) {
    case kBot:
    case kConstruct:
    case kDroid:
    case kEgg:
    case kAlien:
    case kProgram:
    case kOoze:
        /* these creatures have the ability to remain perfectly still */
        return 0;
    case kCyborg:
    case kAberration:
    case kAnimal:
    case kBeast:
    case kHumanoid:
    case kMutant:
    case kInsect:
    case kOutsider:
    case kVermin:
    case kZerg:
        /* the creatures shift around enough when lurking that they are
           still detectable */
        return 1;
    case kMaxCreatureType:
        break;
    }
    return 0;
}


static int
permuteScore (int base, int score)
{
    score += (base - 10);
    return score < 3 ? 3 : score;
}


void
shCreature::rollAbilityScores (
    unsigned int strbas, unsigned int conbas, unsigned int agibas,
    unsigned int dexbas, unsigned int intbas, unsigned int psibas)
{
    unsigned int init[6] = {strbas, conbas, agibas, dexbas, intbas, psibas};
    if (isHero ()) {
        unsigned int totalpoints = strbas + conbas + agibas + dexbas + intbas + psibas;

        /* Distribute some points randomly. */
        while (totalpoints < 71) {
            abil::Index idx = abil::random_index ();
            int score = init[idx - abil::Str];

            if (score < 17 and score >= RNG (3, 18)) {
                /* Prefer to increase already high scores. */
                int boost = RNG (1, 2);
                init[idx - abil::Str] += boost;
                totalpoints += boost;
            }
        }
    } else if (mCLevel < 4 and RNG (6)) {
        FOR_ALL_ABILITIES (a)
            init[a - abil::Str] = permuteScore (init[a - abil::Str], 6 + NDX (2, 3));
    } else {
        init[0] = permuteScore (strbas, 7 + NDX (2, 2));
        init[1] = permuteScore (conbas, 7 + NDX (2, 2));
        init[2] = permuteScore (agibas, 5 + NDX (2, 4));
        init[3] = permuteScore (dexbas, 5 + NDX (2, 4));
        init[4] = permuteScore (intbas, 5 + NDX (2, 4));
        init[5] = permuteScore (psibas, 7 + NDX (2, 2));
    }

    mAbil.init (init);
}


void
shCreature::rollHitPoints (int hitdice, int diesize)
{
    mMaxHP = 0;
    for (int i = 0; i < hitdice; i++) {
        int roll = (0 == i and isHero ()) ? diesize : RNG (1, diesize)
            + ABILITY_MODIFIER (mAbil.Con ());;
        mMaxHP += roll > 1 ? roll : 1;
    }
    mHP = mMaxHP;
}


shThingSize
shCreature::getSize ()
{
    return myIlk ()->mSize;
}

int
shCreature::getSizeACMod (shThingSize size)
{
    switch (getSize ()) {
        case kFine: return +8;
        case kDiminutive: return +4;
        case kTiny: return +2;
        case kSmall: return +1;
        case kMedium: return 0;
        case kLarge: return -1;
        case kHuge: return -2;
        case kGigantic: return -4;
        case kColossal: return -8;
        default:
            debug.log ("computeAC: unknown size: (%d)!", size);
            return 0;
    }
}

int
shCreature::getAC (int flatfooted)
{
    return flatfooted ? mAC - ABILITY_BONUS (mAbil.Agi ()) : mAC;
}

int
shCreature::getTouchAC (int flatfooted)
{
    return flatfooted ? 10 + ABILITY_PENALTY (mAbil.Agi ())
                      : 10 + ABILITY_MODIFIER (mAbil.Agi ());
}

void
shCreature::computeAC ()
{
    mAC = 10 + mNaturalArmorBonus;
    mAC += ABILITY_MODIFIER (mAbil.Agi ());
    mAC += getSizeACMod (getSize());

    if (mBodyArmor) {
        mAC += mBodyArmor->getArmorBonus ();
    }
    if (mJumpsuit) {
        mAC += mJumpsuit->getArmorBonus ();
    }
    if (mHelmet) {
        mAC += mHelmet->getArmorBonus ();
    }
    if (mBelt) {
        mAC += mBelt->getArmorBonus ();
    }
    if (mBoots) {
        mAC += mBoots->getArmorBonus ();
    }
    if (mCloak) {
        mAC += mCloak->getArmorBonus ();
    }
}

/* Returns true if the creature should be sick from the sewer smell. */
bool
shCreature::sewerSmells ()
{
    if (mLevel
        and (mLevel->mType == shMapLevel::kSewer or
             mLevel->mType == shMapLevel::kSewerPlant)
        and !intr (kAirSupply) and !intr (kBreathless)
        and !mResistances[kSickening])
    {
        return true;
    }
    return false;
}

void
shCreature::computeIntrinsics (bool quiet)
{
    bool smelled = sewerSmells ();
    mFeats = myIlk ()->mFeats;

    /* Racial intrinsics that may be cleared by mutant powers
       turned off need to be accounted for here. */
    if (isHero () and Hero.mProfession == XelNaga and mCLevel >= 12) {
        mInnateIntrinsics.set (kTelepathy, 9);
    }
    mIntrinsics.clone (mInnateIntrinsics);
    memcpy (&mResistances, &mInnateResistances, sizeof (mResistances));
    mPsiModifier = 0;
    mToHitModifier = 0;
    mDamageModifier = 0;
    mSpeedBoost = 0;
    /* Reset ability modifiers. */
    FOR_ALL_ABILITIES (a) {
        mAbil._max.mod (a, -mAbil.gear (a));
        mAbil._totl.mod (a, -mAbil.gear (a));
        mAbil._gear.set (a, 0);
    }
    /* Modify creature attributes by its equipment. */
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *obj = mInventory->get (i);
        obj->applyConferredIntrinsics (this);
        obj->applyConferredResistances (this);
    }
    /* Brain shield blocks all sources of telepathy. */
    if (mHelmet and mHelmet->isA (kObjBrainShield)) {
        mIntrinsics.set (kTelepathy, false);
    }
    /* Activated haste mutant power modifies speed. */
    if (usesPower (kHaste)) {
        int skill = getSkillModifier (kMutantPower, kHaste);
        mSpeedBoost += 20 + 2 * mini (skill, 10);
    }
    /* Apply ability bonuses from gear. */
    FOR_ALL_ABILITIES (a) {
        mAbil._max.mod (a, mAbil.gear (a));
        mAbil._totl.mod (a, mAbil.gear (a));
    }
    /* Compute speed. */
    mSpeed = myIlk ()->mSpeed + mSpeedBoost;
    mSpeed += 5 * ABILITY_BONUS (mAbil.Agi ());
    mBurdenMod = 0;
    /* Other gear properties. */
    if (mGoggles and mGoggles->isA (kObjPerilGlasses) and
        mGoggles->is (obj::toggled) and !intr (kXRayVision))
    {
        setBlind ();
    }
    /* Intense radiation blinds gammasensitives. */
    if (usesPower (kGammaSight) and mLevel->isPermaRadioactive () and
        !intr (kBlind))
    {
        if (isHero () and !Hero.getStoryFlag ("gammasense")) {
            I->p ("Arrghhh!  So much radiation hurts your eyes!");
            Hero.setStoryFlag ("gammasense", 1);
        }
        setBlind ();
    }
    /* To hit bonus from active War Flow. */
    if (usesPower (kWarFlow)) {
        int skill = getSkillModifier (kMutantPower, kWarFlow);
        mToHitModifier += maxi (2, 2 + skill / 4);
    }
    if (intr (kBlind)) {
        /* Vision enhancements do no good (harm) when blind. */
        if (mGoggles) {
            mToHitModifier -= mGoggles->myIlk ()->mToHitModifier;
            mDamageModifier -= mGoggles->myIlk ()->mDamageModifier;
        }
        if (mHelmet) {
            mToHitModifier -= mHelmet->myIlk ()->mToHitModifier;
            mDamageModifier -= mHelmet->myIlk ()->mDamageModifier;
        }
    }
    if (isA (kMonGhoul) and mRad >= 300)
        mIntrinsics.set (kAutoRegeneration, true);

    /* How much can you carry? */
    int carry = mAbil.Str () + mAbil.Con ();
    /* Stabilizer belt makes this a bit easier. */
    if (mBelt and mBelt->isA (kObjStabilizerBelt))
        carry += 2 + 1 * mBelt->mBugginess;
    /* Suspensor belt is more effective in this regard. */
    if (mBelt and mBelt->isA (kObjSuspensorBelt))
        carry += 5 + 3 * mBelt->mBugginess;
    mCarryingCapacity = 75 * mini (60, carry);

    /* Having air supply protects from negative effects of sewer stench. */
    bool airsupply = intr (kAirSupply);
    if (smelled and airsupply) {
        if (!quiet)  msg ("Fresh air!");
    } else if (!smelled and sewerSmells ()) {
        if (!quiet)  msg ("What a terrible smell!");
        inflict (kSickened, 2000);
    }

    /* For diving air supply is critical. */
    if (mTrapped.mDrowning and airsupply) {
        mTrapped.mDrowning = 0;
        if (!quiet)  msg ("You can breathe!");
    } else if (!mTrapped.mDrowning and airsupply and isUnderwater ()) {
        mTrapped.mDrowning = mAbil.Con ();
        if (!quiet)  msg ("You are holding your breath!");
    }

    /* Effects of temporary conditions upon speed. */
    if (is (kViolated))   mSpeed -= 50;
    if (is (kSlowed))     mSpeed -= 50;
    if (is (kHosed))      mSpeed -= 100;
    if (is (kSpeedy))     mSpeed += 100;

    /* Are you out to purge xenos scum?  Let us see your attire. */
    if (isHero () and Hero.mProfession == SpaceMarine) {
        mConditions &= ~kXenosHunter;
        if ((mHelmet and mHelmet->isXenosHunterGear ()) or
            (mBodyArmor and mBodyArmor->isXenosHunterGear ()) or
            (mCloak and mCloak->isXenosHunterGear ()))
        {
            mConditions |= kXenosHunter;
        }
    }

    computeBurden (quiet);
}

void
shCreature::computeBurden (bool quiet)
{   /* Some items may not encumber in right conditions. */
    int eff_weight = mWeight;
    /* Self-powered armor does not encumber. */
    if (mBodyArmor and mBodyArmor->isPoweredArmor ()) {
        eff_weight -= mBodyArmor->getMass ();
    }
    if (mHelmet and mHelmet->isPoweredArmor ()) {
        eff_weight -= mHelmet->getMass ();
    }
    if (mBoots and mBoots->isPoweredArmor ()) {
        eff_weight -= mBoots->getMass ();
    }

    /* Determine current slowdown from carrying. */
    unsigned int newburden;
    int newmod;
    if (eff_weight <= mCarryingCapacity) {
        newburden = 0;
        newmod = 0;
    } else if (eff_weight <= mCarryingCapacity * 2) {
        newburden = kBurdened;
        newmod = -50;
    } else if (eff_weight <= mCarryingCapacity * 3) {
        newburden = kStrained;
        newmod = -100;
    } else if (eff_weight <= mCarryingCapacity * 4) {
        newburden = kOvertaxed;
        newmod = -300;
    } else {
        newburden = kOverloaded;
        newmod = -700;
    }

    /* Adjust speed penalty based on burden difference. */
    mSpeed += newmod - mBurdenMod;
    mBurdenMod = newmod;

    /* Print a message. */
    unsigned int oldburden = mConditions & kOverloaded;
    if (isHero () and oldburden != newburden) {
        if (!quiet) {
            if ((unsigned) oldburden > (unsigned) newburden) {
                if (0 == newburden) {
                    I->p ("You movement is no longer encumbered.");
                } else {
                    I->p ("Your movement is somewhat less encumbered now.");
                }
            } else if (kBurdened == newburden) {
                I->p ("You are encumbered by your load.");
            } else if (kStrained == newburden) {
                I->p ("You strain to manage your load.");
            } else if (kOvertaxed == newburden) {
                I->p ("You can barely move under this load.");
            } else {
                I->p ("You collapse under your load.");
            }
        }
        I->drawSideWin (this);
        oldburden = newburden;
    }
    /* Set condition flags. */
    mConditions &= ~kOverloaded;
    mConditions |= newburden;
}
/*
  RETURNS: a d20 roll, with small possibility of rolling higher than 20,
           to give a sporting chance of achieving a high result
*/

static int
absorptionCapacity (shObject *obj)
{
    if (!obj) {
        return 0;
    } else if (obj->isA (kObjTeslaSuit)) {
        int armorcap = obj->myIlk ()->mMaxCharges - obj->mCharges;
        int absorption = obj->myIlk ()->mMaxCharges / (4 - obj->mBugginess);
        return mini (armorcap, absorption);
    } else if (obj->isA (kObjShockCapacitor)) {
        int implantcap = (18 + 6 * obj->mBugginess) - obj->mCharges;
        int absorption = (18 + 6 * obj->mBugginess) / 3;
        return mini (implantcap, absorption);
    }
    return 0;
}

int
shCreature::getShockCapacity (void)
{
    int capacity = 0;
    capacity += absorptionCapacity (mBodyArmor);
    for (int i = 0; i <= shObjectIlk::kCerebellum; ++i)
        capacity += absorptionCapacity (mImplants[i]);
    return capacity;
}

/* Assumes creature posesses means to absorb passed damage amount.
   Excess damage gets lost in the void. */
void
shCreature::absorbShock (int damage, shObjectVector *r)
{   /* Find electricity sinks. */
    shObjectVector v;
    if (mBodyArmor and mBodyArmor->isA (kObjTeslaSuit))
        v.add (mBodyArmor);
    for (int i = 0; i <= shObjectIlk::kCerebellum; ++i)
        if (mImplants[i] and mImplants[i]->isA (kObjShockCapacitor))
            v.add (mImplants[i]);
    /* Distribute damage. */
    while (v.count () and damage) {
        /* Pick one randomly. */
        int index = RNG (v.count ());
        shObject *obj = v.get (index);
        int capacity = mini (damage, absorptionCapacity (obj));
        obj->mCharges += capacity;
        damage -= capacity;
        if (capacity and isHero () and !obj->is (obj::known_type))
            r->add (obj);
        v.removeByIndex (index);
    }
}

void
shCreature::free_from_traps ()
{
    mTrapped.mInPit = 0;
    mTrapped.mDrowning = 0;
    if (mZ < 0)
        mZ = 0;
}

void
shCreature::innate_knowledge (shObject *obj)
{
    assert (obj);
    if (!obj or !isHero ())
        return;

    Hero.mProfession->examine_item (obj);
}

void
shCreature::pregnancy ()
{
    if (!mImpregnation)
        return;

    ++mImpregnation;
    if (mImpregnation > 100) {
        int x = mX, y = mY;
        shMonId type = !RNG (18) ? kMonAlienPrincess : kMonChestburster;
        bool noPlace = Level->findAdjacentUnoccupiedSquare (&x, &y);
        shCreature *baby = shCreature::monster (type);
        if (baby and noPlace) {
            /* No place to put baby?  Just wait but make sure sickness continues. */
            inflict (kSickened, 10 * FULLTURN);
            inflict (kStunned, 3 * FULLTURN); /* periodical stun */
            delete baby;
            --mImpregnation;
        } else {
            mLevel->putCreature (baby, x, y);
            if (isHero ())  I->drawScreen ();
            mImpregnation = 0; /* for post mortem */
            msg ("The alien creature explodes from your chest!") or
                appear (fmt ("An alien creature explodes from %s's chest!", the ()));
            die (kMisc, "Died during childbirth");
        }
    } else if (mImpregnation == 95) {
        see (fmt ("%s %s violently ill!", the (), are ()));
        inflict (kStunned, 20 * FULLTURN);
        interrupt ();
    } else if (mImpregnation == 90) {
        see (fmt ("%s vomit%s!", the (), isHero () ? "" : "s"));
        inflict (kSickened, 30 * FULLTURN);
        interrupt ();
    } else if (mImpregnation == 75) {
        msg ("You feel very sick.") or
            appear (fmt ("%s appears to be sick.", the ()));
        inflict (kSickened, 10 * FULLTURN);
        interrupt ();
    } else if (mImpregnation == 50) {
        msg ("You feel something moving inside you!");
        interrupt ();
    } else if (mImpregnation == 25) {
        msg ("You feel a little queasy.");
        interrupt ();
    }
}

void
shCreature::abortion (bool quiet)
{
    if (kDead != mState and mImpregnation) {
        if (!quiet)  msg ("The parasitic alien inside you is killed!");
        mImpregnation = 0;
        beatChallenge (MonIlks[kMonFacehugger].mBaseLevel);
        if (isHero ()) {
            ++MonIlks[kMonAlienEmbryo].mKills;
        }
    }
}

int
sportingD20 ()
{
    int result;

    result = RNG (1, 21);
    if (21 == result) {
        do {
            result += RNG (6);
        } while (! RNG (3));
    }
    return result;
}

/* The following four routines return true if message was actually printed. */
bool
shCreature::msg (const char *str) /* Something hero feels or knows. */
{
    if (!isHero ())  return false;
    I->p (str);
    return true;
}

bool
shCreature::see (const char *str) /* Something hero sees. */
{
    if (!Hero.cr ()->canSee (this))  return false;
    I->p (str);
    return true;
}

bool
shCreature::hear (const char *str) /* Something hero hears. */
{   /* Deafness is not implemented yet. */
    if (!isHero () /*or is (kDeaf)*/)  return false;
    I->p (str);
    return true;
}

bool
shCreature::appear (const char *str) /* Hero sees creature do something. */
{
    if (isHero () or !Hero.cr ()->canSee (this))  return false;
    I->p (str);
    return true;
}

bool
shCreature::visual (const char *str) /* Hero sees creature do something. */
{
    if (!isHero () or intr (kBlind))  return false;
    I->p (str);
    return true;
}
