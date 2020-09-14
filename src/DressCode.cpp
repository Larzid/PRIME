#include "DressCode.h"
#include "Hero.h"

bool
Janitor_code (shCreature *c)
{
    return c->myIlk ()->mType == kHumanoid
        and NOT0 (c->mWeapon,->isA (kObjMop))
        and NOT0 (c->mJumpsuit,->isA (kObjJanitorUniform))
        and (   !c->mBodyArmor
             or (    c->isHero ()
                 and Hero.mProfession == Janitor));
}

bool
Ninja_code (shCreature *c)
{
    return c->myIlk ()->mType == kHumanoid
        and c->mWeapon != NULL
        and c->mWeapon->myIlk ()->mMeleeSkill == kSword
        and c->mGoggles != NULL
        and c->mBodyArmor == NULL
        and NOT0 (c->mJumpsuit,->isA (kObjChameleonSuit));
}

bool
Stormtrooper_code (shCreature *c)
{
    return c->myIlk ()->mType == kHumanoid
        and c->mWeapon != NULL
        and (   c->mWeapon->isA (kObjBlasterPistol)
             or c->mWeapon->isA (kObjBlasterRifle))
        and NOT0 (c->mBodyArmor,->isA (kObjStormtrooperSuit))
        and NOT0 (c->mHelmet,->isA (kObjStormtrooperHelmet))
        and NOT0 (c->mBoots,->isA (kObjStormtrooperBoots));
}

bool
Troubleshooter_code (shCreature *c)
{
    return c->myIlk ()->mType == kHumanoid
        and c->mWeapon != NULL
        and (   c->mWeapon->isA (kObjLaserPistol)
             or c->mWeapon->isA (kObjLaserRifle)
             or c->mWeapon->isA (kObjLaserCannon))
        and c->mJumpsuit != NULL
        and (   c->mJumpsuit->myIlk ()->mAppearance.mGlyph.mColor == kRed
             or c->mJumpsuit->myIlk ()->mAppearance.mGlyph.mColor == kOrange)
        and NOT0 (c->mBodyArmor,->isA (kObjReflecSuit));
}

bool
looks_like (shCreature *c, dress::DressCode code)
{
    typedef bool (*dress_code_fn)(shCreature *);
    dress_code_fn dress_predicate[dress::Num_DressCode] =
    {
        Janitor_code,
        Ninja_code,
        Stormtrooper_code,
        Troubleshooter_code
    };

    assert (code != dress::Num_DressCode);
    if (code == dress::Num_DressCode)
        return false;

    if (dress_predicate[code] == NULL)
        return false;

    return (*dress_predicate[code])(c);
}
