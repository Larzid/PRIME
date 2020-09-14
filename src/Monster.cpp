#include <math.h>
#include <stdio.h>
#include "Global.h"
#include "Util.h"
#include "Monster.h"
#include "Creature.h"
#include "gen/ObjectIlk.h"
#include "Object.h"
#include "Interface.h"
#include "Hero.h"
#include "gen/MonsterIlk.h"

#define MAXMONSTERLEVEL 25
static shVector <shMonId> MonsterIlksGroupedByLevel[MAXMONSTERLEVEL];

shAttackId lizard[NUMLIZARDS];
void addLizardBreaths ();

static int
sym2TileY (char sym)
{
    if (sym == '@') {
        return 2;
    } else if (sym <= 'Z') {
        return sym - 'A' + kRowBigA;
    } else {
        return sym - 'a' + kRowLittleA;
    }
}

void
initializeMonsters ()
{
    for (int i = 0; i < kMonNumberOf; ++i) {
        shMonsterIlk *ilk = &MonIlks[i];
        ilk->mId = shMonId (i);
        if (ilk->mGlyph.mTileY == -1)
            ilk->mGlyph.mTileY = sym2TileY (ilk->mGlyph.mSym);

        if (ilk->mGlyph.mTileX == 0) { /* White ASCII character. */
            /* NotEye will repaint the glyph to right color.
               24 is simply far enough to right to be safely assumed free. */
            ilk->mGlyph.mTileX = 24 + ilk->mGlyph.mColor;
        }

        for (int j = 0; j < MAXPOWERS; ++j) {
            ilk->mPowers[j]  and  ++ilk->mNumPowers;
        }

        for (int j = 0; j < MAXATTACKS; ++j) {
            ilk->mAttackSum += ilk->mAttacks[j].mProb;
        }

        // Temporarily, until monster generation mechanism is improved
        MonsterIlksGroupedByLevel[ilk->mBaseLevel].add (ilk->mId);
    }


    int roll = RNG (3);
    lizard[0] = roll == 0 ? kAttHeatRay :
                roll == 1 ? kAttFreezeRay :
                            kAttPoisonRay;
    roll = RNG (6);
    lizard[1] = roll == 0 ? kAttHeatRay :
                roll == 1 ? kAttFreezeRay :
                roll == 2 ? kAttPoisonRay :
                roll == 3 ? kAttTransporterRay :
                roll == 4 ? kAttGammaRay :
                            kAttStasisRay;
    roll = RNG (6);
    lizard[2] = roll == 0 ? kAttPoisonRay :
                roll == 1 ? kAttGammaRay :
                roll == 2 ? kAttTransporterRay :
                roll == 3 ? kAttGaussRay :
                roll == 4 ? kAttStasisRay :
                            kAttDisintegrationRay;
}

void addLizardBreaths ()
{   /* Clone and modify ray gun zap effect. */
    for (int i = 0; i < NUMLIZARDS; ++i) {
        shAttack *atk = &Attacks[shAttackId (kAttLizardBreath1 + i)];
        memcpy (atk, &Attacks[lizard[i]], sizeof (shAttack));
        atk->mRange = 8;
        atk->mAttackTime = LONGTURN - HALFTURN * i;
        atk->mDamage[0].mHigh = atk->mDamage[0].mHigh * (i+1) / 2;
    }
}

shCreature *
shCreature::monster (shMonId id)
{
    shCreature *c = new shCreature ();
    c->becomeAMonster (id);
    return c;
}

static void
set_toxin_resistance (shCreature *c, int res_lvl)
{
    for (int e = kDrnStr; e <= kDmgPsi; ++e)
        c->mInnateResistances[e] = res_lvl;
}

/* Equipment.cpp defines regular monster posessions. */
extern const char **equipment[kMonNumberOf];
void equip_monster (shCreature *);

void
shCreature::getEquipmentSkills()
{
    bool gotweapon = false;
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *obj = mInventory->get (i);
        if (!mWeapon and obj->isA (kWeapon)) {
            if (obj->myIlk ()->mMeleeSkill != kNoSkillCode) {
                gainRank (obj->myIlk ()->mMeleeSkill, 1 + mCLevel * 2/3);
            }
            if (obj->myIlk ()->mGunSkill != kNoSkillCode) {
                gainRank (obj->myIlk ()->mGunSkill, 1 + mCLevel * 2/3);
            }
            gotweapon = true;
            /* don't wield until hero is in sight so he can see message */
            //wield (obj, 1);
        } else if (obj->isA (kArmor) or obj->isA (kImplant)) {
            don (obj, true);
        }
    }

    if (!gotweapon) {
        gainRank (kUnarmedCombat, 1 + mCLevel);
    }
}

void
shCreature::becomeAMonster (shMonId id)
{
    shMonsterIlk *ilk = &MonIlks[id];
    mIlkId = id;

    strncpy (mName, ilk->mName, 30);
    mCLevel = ilk->mBaseLevel;
    mXP = (mCLevel - 1) * 1000 + RNG (0, 999);
    mNaturalArmorBonus = ilk->mNaturalArmorBonus;
    mReflexSaveBonus = 0;

    mDir = kNoDirection;
    mTame = 0;
    mStrategy = ilk->mDefaultStrategy;
    mDisposition = ilk->mDefaultDisposition;
    mTactic = kReady;
    mDestX = -1;
    mEnemyX = -1;
    mPlannedMoveIndex = -1;
    mSpellTimer = 0;

    memcpy (mInnateResistances, ilk->mInnateResistances, sizeof (mInnateResistances));
    for (int i = 0; i < MAXINTR; ++i)
        if (ilk->mIntrinsics[i].mIntrinsic != kNoIntrinsic)
            mInnateIntrinsics.set (ilk->mIntrinsics[i].mIntrinsic, ilk->mIntrinsics[i].mGrade);
    for (int i = 0; i < ilk->mNumPowers; ++i)
        if (ilk->mPowers[i] == kTelepathyPower)
            mInnateIntrinsics.set (kTelepathy, 8); /* Assume always on. */
    mFeats = ilk->mFeats;

#define IMMUNE 122

    switch (ilk->mType) {
    case kMutant:
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateResistances[kMagnetic] = IMMUNE;
        break;
    case kHumanoid:
        mInnateResistances[kMagnetic] = IMMUNE;
        break;
    case kAnimal:
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateIntrinsics.set (kScent, true);
        break;
    case kInsect:
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateIntrinsics.set (kScent, true);
        break;
    case kOutsider:
        mInnateResistances[kMagnetic] = IMMUNE;
        break;
    case kBot:
        set_toxin_resistance (this, IMMUNE);
        mInnateResistances[kPsychic] = IMMUNE;
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateResistances[kViolating] = IMMUNE;
        mInnateResistances[kMesmerizing] = IMMUNE;
        mInnateResistances[kBlinding] = IMMUNE;
        mInnateIntrinsics.set (kBreathless, true);
        break;
    case kDroid:
        set_toxin_resistance (this, IMMUNE);
        mInnateResistances[kPsychic] = IMMUNE;
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateResistances[kViolating] = IMMUNE;
        mInnateResistances[kMesmerizing] = IMMUNE;
        mInnateResistances[kBlinding] = IMMUNE;
        mInnateIntrinsics.set (kBreathless, true);
        break;
    case kProgram:
        set_toxin_resistance (this, IMMUNE);
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateResistances[kViolating] = IMMUNE;
        /* Processes can be put to sleep so no kMesmerizing immunity. */
        mInnateIntrinsics.set (kBreathless, true);
        break;
    case kConstruct:
        set_toxin_resistance (this, IMMUNE);
        mInnateResistances[kPsychic] = IMMUNE;
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateResistances[kStunning] = IMMUNE;
        mInnateResistances[kConfusing] = IMMUNE;
        mInnateResistances[kViolating] = IMMUNE;
        mInnateResistances[kParalyzing] = IMMUNE;
        mInnateResistances[kMesmerizing] = IMMUNE;
        mInnateResistances[kBlinding] = IMMUNE;
        mInnateIntrinsics.set (kBreathless, true);
        break;
    case kOoze:
        set_toxin_resistance (this, 3);
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateResistances[kViolating] = IMMUNE;
        mInnateResistances[kWebbing] = IMMUNE;
        mInnateIntrinsics.set (kBreathless, true);
        break;
    case kAberration:
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateResistances[kRadiological] = IMMUNE;
        mInnateResistances[kViolating] = IMMUNE;
        break;
    case kCyborg:
        set_toxin_resistance (this, 2);
        mInnateResistances[kRadiological] = 10;
        mInnateIntrinsics.set (kAirSupply, true);
        break;
    case kEgg:
        mInnateResistances[kViolating] = IMMUNE;
        mInnateIntrinsics.set (kBreathless, true);
        /* fall through */
    case kBeast:
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateIntrinsics.set (kScent, true);
        break;
    case kVermin:
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateIntrinsics.set (kScent, true);
        mInnateIntrinsics.set (kCanSwim, true);
        break;
    case kZerg:
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateResistances[kConfusing] = IMMUNE;
        mInnateIntrinsics.set (kAutoRegeneration, true);
        break;
    case kAlien:
        mAlienEgg.mHatchChance = 0;
        mInnateResistances[kMagnetic] = IMMUNE;
        mInnateResistances[kRadiological] = 10;
        mInnateResistances[kCorrosive] = IMMUNE;
        mInnateIntrinsics.set (kSenseLife, true);
        mInnateIntrinsics.set (kScent, true);
        if (!isA (kMonAlienQueen)) /* Too heavy and bloated. */
            mInnateIntrinsics.set (kJumpy, true);
        gainRank (kUnarmedCombat, mCLevel/2);
        break;
    default:
        assert (false);
        debug.log ("Alert! Unknown monster type");
        mInnateResistances[kMagnetic] = IMMUNE;
    }

#undef IMMUNE

    if (ilk->mGender == 3) {
        mGender = RNG (1, 2);
    } else {
        mGender = ilk->mGender;
    }

    /* roll ability scores */
    rollAbilityScores (ilk->mStr, ilk->mCon, ilk->mAgi,
                       ilk->mDex, ilk->mInt, ilk->mPsi);

    /* roll hit points */
    rollHitPoints (ilk->mHitDice, 8);

    /* setup speed */
    mSpeed = ilk->mSpeed;
    computeAC ();

    mGlyph = ilk->mGlyph;
    if (isA (kMonMutantNinjaTurtle))
        mGlyph.mTileX += RNG (4); /* Got four of those. */
    mState = kActing;

    /* Maybe get some adventure skills. */
    if (isAlive () and myIlk ()->mNumPowers)
        gainRank (kConcentration, mCLevel + RNG (5));

    equip_monster (this);
    getEquipmentSkills ();

    /* Maybe monster gets some more treasure. */
    shCreatureType t = ilk->mType;
    if (feat (kNoTreasure) or
        kAnimal == t  or kBeast == t    or kAberration == t or
        kInsect == t  or kVermin == t   or kOutsider == t or
        kEgg == t     or kOoze == t     or kAlien == t or
        kBot == t     or kZerg == t     or kConstruct == t)
    { /* No treasure requested or a monster does not want treasure. */
    } else {
        if (RNG (50) <= 5 + mCLevel) {
            shObject *cash = new shObject (kObjMoney);
            cash->mCount = NDX (mCLevel + 1, 10);
            addObjectToInventory (cash);
        }
        if (RNG (80) <= 5 + mCLevel) {
            shObject *obj = generateObject ();

            while (obj->getMass () > 5000) {
                delete obj;
                obj = generateObject ();
            }

            addObjectToInventory (obj);
        }
    }

    specialAdjustments ();

    if (intr (kFlying))  mZ = 1;

    computeIntrinsics ();

    if (RNG (100) < ilk->mPeacefulChance) {
        mDisposition = kIndifferent;
    }

    debug.log ("spawned %s with %d HP speed %d", mName, mHP, mSpeed);
}


void
shCreature::specialAdjustments (void)
{
    switch (mIlkId) {
    case kMonBeneGesserit:
        gainRank (MutantPowers[kTheVoice].getCode (), 10 + RNG (6), kTheVoice);
        break;
    case kMonBrainMold:
        /* Their base mental blast is too weak to be a threat. */
        gainRank (MutantPowers[kMentalBlast].getCode (), RNG (6, 12), kMentalBlast);
        break;
    case kMonDarkTemplar:
        /* The scythe has high inherent malus.  This almost neutralizes it. */
        gainRank (kMeleeWeapon, 12);
        break;
    case kMonMelnorme:
    {   /* Initialize trader data. */
        /* No room bounds are known before being placed on map. */
        mMelnorme.mSX = -1;
        /* Set up knowledge and services. */
        mMelnorme.mKnowledgeExhausted = 0;
        for (int i = 0; i < kMelnMaxService; ++i)
            mMelnorme.mPermute[i] = i;
        shuffle (mMelnorme.mPermute, kMelnMaxService, sizeof (int));
    }
    default:
        break;
    }
}
