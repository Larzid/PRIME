#ifndef MUTANT_H
#define MUTANT_H

#include "Ability.h"

#define MUT_POWER_ON 2
#define MUT_POWER_PRESENT 1
#define MUT_POWER_ABSENT 0

enum shMutantPower {
    kNoMutantPower = 0x0,
    /* Level 1 */
    kIllumination,
    kDigestion,
    kHypnosis,
    kRegeneration,
    kGammaSight,
    kPsiVomit,
    kDeepsight,
    /* Level 2 */
    kOpticBlast,
    kHaste,
    kTelepathyPower,
    kTremorsensePower,
    kShootWebs,
    kCharge,
    kWarFlow,
    /* Level 3 */
    kMentalBlast,
    kRestoration,
    kUnderstanding,
    kImbue,
    kKinesis,
    //kGravityCrush,
    kCauseFear,
    /* Level 4 */
    kAdrenalineControl,
    kXRayVisionPower,
    kTheVoice,
    kMagMayhem,
    //kSwordOfTheMind,
    //kSaltTheWound,
    /* Level 5 */
    kTeleport,
    kBGAwareness,
    kShockwave,
    //kCharm,
    //kMarkOfHate,
    /* Level 6 */
    kAutolysis,
    kFlightPower,
    //kGroundBurst,
    /* Level 8 */
    kPsionicStorm,
    //kStasisField,
/* monster only powers*/
    kHallucinate,
    kMaxHeroPower = kHallucinate,
    kTerrify,
    kDarkness,
    kMaxMutantPower = kDarkness,
    kCeaseAndDesist, /* lawyer first */
    kSeizeEvidence,
    kSueForDamages,
    kSummonWitness,  /* lawyer last */
    kLaunchMissile,  /* rocket turret */
    kMaxAllPower
};

struct shMutantPowerData {
    int mLevel;
    char mLetter;
    abil::Index mAbility;
    shSkillCode mSkill;
    const char *mName;
    int (*mFunc) (shCreature *);
    int (*mOffFunc) (shCreature *);

    bool isPersistent (void);
    shSkillCode getCode (void);
    int maintainCost (void);
};

extern shMutantPowerData MutantPowers[kMaxHeroPower];
const char *getMutantPowerName (shMutantPower id);

#endif
