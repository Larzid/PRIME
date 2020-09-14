#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "ObjectIlk.h"
#include "Creature.h"
#include "Hero.h"

const char *
shObjectIlk::getRayGunColor ()
{
    char *buf = GetBuf ();
    strcpy (buf, mAppearance.mName);
    char *p = buf;
    while (*p != ' ')
        ++p;
    *p = 0;
    return buf;
}

static const struct shCanToRayGun {
    shObjId mCanIlk, mGunIlk;
    int mDice, mSides;
} fillTable[] = {
    {kObjMutagenCanister,       kObjGammaRayGun,            2, 10},
    {kObjFullHealingCanister,   kObjHealingRayGun,          6,  3},
    {kObjRadAway,               kObjDecontaminationRayGun,  5,  3},
    {kObjWater,                 kObjSquirtRayGun,           3,  4},
    {kObjLNO,                   kObjFreezeRayGun,           3,  4},
    {kObjNapalm,                kObjHeatRayGun,             3,  4},
    {kObjAntimatter,            kObjDisintegrationRayGun,   3,  3},
    {kObjSuperGlue,             kObjStasisRayGun,           2,  6},
    {kObjPoisonCanister,        kObjPoisonRayGun,           2,  6},
    {kObjPlasmaCanister,        kObjGaussRayGun,            2,  6},
    {kObjSpiceMelange,          kObjTransporterRayGun,      2,  6},
    {kObjHealingCanister,       kObjHealingRayGun,          2,  6},
    {kObjSpeedCanister,         kObjAccelerationRayGun,     2,  5},
    {kObjSpeedCanister,         kObjDecelerationRayGun,     2,  5},
    {kObjRestorationCanister,   kObjRestorationRayGun,      2,  4}
};
static const int nfills = sizeof (fillTable) / sizeof (struct shCanToRayGun);

/* Returns time elapsed. */
int
shCreature::loadRayGun (shObject *gun)
{
    assert (isHero ());
    if (!isHero ())  return FULLTURN;

    shObjectVector v;
    shObject *can;

    selectObjects (&v, mInventory, kCanister);
    can = quickPickItem (&v, "reload with", 0);
    if (!can)  return 0;

    int index;
    for (index = 0; index < nfills; ++index)
        if (fillTable[index].mCanIlk == can->mIlkId)
            break;

    bool loaded = false;

    if (can->isA (kObjGainAbilityCanister) and !can->isBuggy ()) {
        /* Buggy cans will ruin the ray gun. */
        gun->mIlkId = kObjAugmentationRayGun;
        gun->mCharges += can->isOptimized () ? NUM_ABIL : 1;
        loaded = true;
    } else if (index == nfills) {
        /* Remaining canisters that *should* ruin ray guns:
           beverages (beer, nano/nuka cola, bbb, water, coffee)
           universal solvent - has different message
           canned alien embryo
           brain
        */
        msg (fmt ("Your ray gun is %s!",
              can->isA (kObjUniversalSolvent) ? "dissolved" : "ruined"));
        can->maybeName ();
        useUpOneObjectFromInventory (can);
        useUpOneObjectFromInventory (gun);
        return FULLTURN;
    } else {
        int mod = 0;
        if (fillTable[index].mCanIlk == kObjSpeedCanister) {
            if (can->isBuggy ())
                ++index; /* Use deceleration instead. */
        } else {
            mod = can->mBugginess;
        }
        gun->clear (obj::known_type); /* Not doing it would identify new ilk. */
        gun->mIlkId = fillTable[index].mGunIlk;
        gun->mCharges += NDX (fillTable[index].mDice + mod, fillTable[index].mSides);
        loaded = true;
    }

    if (loaded) {
        if (!intr (kBlind)) {
            I->p ("The light on the ray gun is %s now.", gun->myIlk ()->getRayGunColor ());
        } else {
            gun->clear (obj::known_appearance);
        }
        if (can->is (obj::known_type)) {
            gun->set (obj::known_type);
        } else if (gun->is (obj::known_type)) {
            can->set (obj::known_type);
        } else {
            can->maybeName ();
        }
    }

    useUpOneObjectFromInventory (can);

    return FULLTURN;
}

static int
loadEmptyRayGun (shCreature *user, shObject *raygun)
{
    return user->loadRayGun (raygun);
}

int
getChargesForRayGun (shObjId raygun)
{
    if (raygun == kObjAugmentationRayGun)
        return 1;
    for (int i = 0; i < nfills; ++i)
        if (fillTable[i].mGunIlk == raygun)
            return NDX (fillTable[i].mDice, fillTable[i].mSides);
    return 0;
}

void
initializeRayGuns (void)
{
    AllIlks[kObjEmptyRayGun].mUseFunc = loadEmptyRayGun;
}
