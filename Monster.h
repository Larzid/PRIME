#ifndef MONSTER_H
#define MONSTER_H

#include "Util.h"
#include "Creature.h"
#include "AttackType.h"
#include "Interface.h"

struct shMonsterIlk
{
    shMonId mId;
    // TODO: This does not belong in monsterIlk.  Move to Hero.
    int mKills;             // how many died during game
    shCreatureType mType;

    const char *mName;

    shThingSize mSize;
    int mHitDice;
    int mBaseLevel;

    int mStr;
    int mCon;
    int mAgi;
    int mDex;
    int mInt;
    int mPsi;

    int mSpeed;

    int mGender;
    int mNumHands;    // number of hands that can wield weapons

    int mNaturalArmorBonus;
    int mFeats;
    int mInnateResistances[kMaxEnergyType];
#define MAXINTR 2
    shIntrinsic mIntrinsics[MAXINTR];
#define MAXPOWERS 5
    shMutantPower mPowers[MAXPOWERS];
    int mNumPowers;
#define MAXATTACKS 5
    struct shAttackData {
        shAttackId mAttId;
        int mProb;
    } mAttacks[MAXATTACKS], mRangedAttacks[MAXATTACKS];
    int mAttackSum;

    shGlyph mGlyph;

    shCreature::Strategy mDefaultStrategy;
    shCreature::Disposition mDefaultDisposition;
    int mPeacefulChance;

    void spoilers (FILE *);
};


void initializeMonsters ();

shMonId pickAMonsterIlk (int level);

void monsterSpoilers ();

#define NUMLIZARDS 3
extern shAttackId lizard[NUMLIZARDS];
extern void addLizardBreaths ();

#endif
