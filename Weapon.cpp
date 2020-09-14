#include "Global.h"
#include "Util.h"
#include "Object.h"
#include "Hero.h"
#include "Creature.h"
#include "Weapon.h"

static int
selectWeaponFireMode (shCreature *user, shObject *gun)
{
    user->msg (fmt ("You switch your weapon to %s fire mode.",
        gun->is (obj::toggled) ? "single" : "burst"));
    gun->toggle (obj::toggled);
    return HALFTURN;
}

static int
selectPowerLevel (shCreature *user, shObject *gun)
{
    user->msg (fmt ("You switch your weapon to %s power setting.",
        gun->is (obj::toggled) ? "low" : "high"));
    gun->toggle (obj::toggled);
    return HALFTURN;
}

static int /* Mostly a YAFM. */
useAnalProbe (shCreature *user, shObject *probe)
{
    if (user->mGlyph.mSym == 'g') { /* Reticulan. */
        user->sufferDamage (probe->myIlk ()->mMeleeAttack);
        probe->set (obj::known_exact);
        I->p ("Your experience tells you this is %s.", probe->inv ());
        return FULLTURN;
    } else {
        I->p ("You sick bastard!  I am not going to do it!");
        return 0;
    }
}

static int
chargePlasmaCaster (shCreature *user, shObject *caster)
{
    const int CHARGE_COST[2] = {24, 36}; /* Energy cells needed. */
    caster->set (obj::known_charges);
    if (caster->mCharges == 2) {
        I->p ("You cannot charge it further.");
        return 0;
    } else {
        int next = caster->mCharges;
        if (user->countEnergy () >= CHARGE_COST[next]) {
            user->loseEnergy (CHARGE_COST[next]);
            ++caster->mCharges;
            I->p ("You charge the plasma caster.");
            return caster->mCharges * FULLTURN;
        } else {
            I->p ("You need %d energy to add %s charge.", CHARGE_COST[next],
                  next ? "second" : "first");
            return 0;
        }
    }
}

static int
chargeCryolator (shCreature *user, shObject *cryolator)
{
    int found = 0;
    shMenu *menu = I->newMenu ("Pick ammunition", 0);
    for (int i = 0; i < user->mInventory->count (); ++i) {
        shObject *obj = user->mInventory->get (i);
        if (obj->isA (kObjLNO) and obj->is (obj::known_type)) {
            menu->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
            ++found;
        }
    }
    if (!found) {
        I->p ("You appear to have no canisters of liquid nitrogen.");
        return 0;
    }
    shObject *obj;
    if (menu->getPtrResult ((const void **) &obj)) {
        obj = user->removeOneObjectFromInventory (obj);
        cryolator->mCharges += RNG (60, 75);
        cryolator->mCharges =
            mini (cryolator->myIlk ()->mMaxCharges, cryolator->mCharges);
        I->p ("You charge %s with %s.", YOUR (cryolator), THE (obj));
        delete obj;
        return FULLTURN;
    } else
        return 0;
}

static int
quaffBloodSword (shCreature *user, shObject *sword)
{
    if (sword->mEnhancement == -sword->myIlk ()->mMaxEnhancement) {
        I->p ("%s is empty.", YOUR (sword));
        if (sword->is (obj::known_enhancement))
            return 0;
        sword->set (obj::known_enhancement);
        return HALFTURN;
    }
    if (sword->mEnhancement > 0) {
        int heal = sword->mEnhancement * sword->mEnhancement;
        if (sword->mEnhancement == sword->myIlk ()->mMaxEnhancement and
            !sword->isBuggy () and user->needsRestoration ())
        {
            int bonus = sword->isOptimized () and !RNG (3);
            user->restoration (1 + bonus);
            heal /= 2 + 3 * bonus;
        }
        user->healing (heal, 0);
    } else {
        I->p ("Yeech!  This tastes awful.");
    }
    sword->mEnhancement = -sword->myIlk ()->mMaxEnhancement;
    return FULLTURN;
}

void
initializeWeapons ()
{
    AllIlks[kObjAnalProbe].mUseFunc = useAnalProbe;
    AllIlks[kObjPlasmaCaster].mUseFunc = chargePlasmaCaster;
    AllIlks[kObjCryolator].mUseFunc = chargeCryolator;

    AllIlks[kObjPulseRifle].mUseFunc = selectWeaponFireMode;
    AllIlks[kObjM56Smartgun].mUseFunc = selectWeaponFireMode;
    AllIlks[kObjM57Smartgun].mUseFunc = selectWeaponFireMode;
    AllIlks[kObjCombatTranslocator].mUseFunc = selectPowerLevel;

    AllIlks[kObjBloodSword].mQuaffFunc = quaffBloodSword;
}


int
shObject::isAmmo (shObject *weapon)
{
    if (NULL == weapon) {
        return isA (kProjectile);
    }
    return isA (weapon->myIlk ()->mAmmoType);
}


/* returns: 1 if the creature has enough ammo to shoot the weapon once,
              or if the weapon doesn't need ammo; 0 o/w
 */
int
shCreature::hasAmmo (shObject *weapon)
{
    shObjectIlk *ilk = weapon->myIlk ();
    shObjId ammo = weapon->isA (kRayGun) ? kObjNothing : ilk->mAmmoType;
    int i;
    int n = 0;

    if (kObjNothing == ammo) {
        if (weapon->isChargeable ()) {
            if (weapon->mCharges) {
                return 1;
            } else {
                if (isHero ()) {
                    weapon->set (obj::known_charges);
                }
                return 0;
            }
        }
        return 1;
    }
    n = ilk->mAmmoBurst;
    if (kObjEnergyCell == ammo) {
        return countEnergy () >= n ? 1 : 0;
    }
    for (i = 0; n > 0 and i < mInventory->count (); i++) {
        shObject *obj = mInventory->get (i);
        if (obj->isA (ammo)) {
            if (obj->isChargeable ())
                n -= obj->mCharges;
            else
                n -= obj->mCount;
        }
    }
    return n <= 0 ? 1 : 0;
}


//RETURNS: number of rounds taken on success; 0 if no ammo available;
//         -1 if none were needed
//         For weapons that use energy cells, returns 1 or 0 depending on
//          whether or not enough cells were consumed

int
shCreature::expendAmmo (shObject *weapon, int cnt /* = 0 */ )
{
    shObjectIlk *ilk = weapon->myIlk ();
    shObjId ammo = ilk->mAmmoType;
    int n = 0;

    if (0 == cnt) {
        if (weapon->isSelectiveFireWeapon () and !weapon->is (obj::toggled)) {
            cnt = 1;
        } else {
            cnt = ilk->mAmmoBurst;
        }
    }
    if (weapon->isChargeable () and !weapon->isA (kObjPlasmaCaster)) {
        if (weapon->mCharges > cnt) {
            weapon->mCharges -= cnt;
        } else {
            cnt = weapon->mCharges;
            weapon->mCharges = 0;
        }
        return cnt;
    } else if (kObjNothing == ammo) {
        return -1;
    }
    if (kObjEnergyCell == ammo) {
        if (countEnergy () >= cnt) {
            loseEnergy (cnt);
            return 1;
        } else {
            return 0;
        }
    }

    for (int i = 0; n < cnt and i < mInventory->count (); ++i) {
        shObject *obj = mInventory->get (i);
        if (obj->isA (ammo)) {
            if (obj->isChargeable ()) {
                if (obj->mCharges < cnt)
                    return 0;
                obj->mCharges -= cnt;
                int remaining = obj->mCharges / obj->myIlk ()->mMaxCharges;
                if (obj->mCharges % obj->myIlk ()->mMaxCharges)  ++remaining;
                useUpSomeObjectsFromInventory (obj, obj->mCount - remaining);
                n = cnt;
                break;
            }
            if (kObjBullet != ammo and obj->mCount < cnt) {
                return 0; /* Only bullet weapons may burst right now. */
            }
            int x = useUpSomeObjectsFromInventory (obj, cnt - n);
            if (x) {
                n += x;
                --i;
            }
        }
    }
    return n;
}

namespace wpn {

shAttack *
get_gun_attack (shCreature *c, shObject **wpn, int &hit_mod, int &rounds)
{
    shObject *weapon = *wpn;
    shAttack *attack = NULL;

    if (weapon->has_subtype (computer)) { /* Control shoulder cannon. */
        if (weapon->mEnhancement == -5) { /* No OS, no shooting. */
            if (c->isHero ()) {
                I->p ("%s does not respond.", YOUR (weapon));
                weapon->set (obj::known_enhancement);
            }

            return NULL;
        }
        hit_mod -= c->mCloak->myIlk ()->mToHitModifier; /* Negate recoil. */
        hit_mod += weapon->mEnhancement; /* OS modifier. */

        /* Consider only the plasma caster from now on. */
        *wpn = c->mCloak;
        weapon = c->mCloak;
    }

    if (weapon->isA (kObjCombatTranslocator)) {
        attack = &Attacks[weapon->myIlk ()->mGunAttack];

        if (weapon->is (obj::toggled)) {
            attack->mDamage[0].mLow = 3;
            attack->mDamage[0].mHigh = 5;
            rounds = c->expendAmmo (weapon, 35);
        } else {
            attack->mDamage[0].mLow = 2;
            attack->mDamage[0].mHigh = 3;
            rounds = c->expendAmmo (weapon, 15);
        }

        if (weapon->isBuggy ())
            --attack->mDamage[0].mLow;

        if (weapon->isOptimized ())
            ++attack->mDamage[0].mHigh;

    } else if (weapon->isA (kObjPlasmaCaster)) {
        if (weapon->mCharges) {
            attack = &Attacks[kAttPlasmaCaster1];
            attack->mRadius = weapon->mCharges;
            weapon->mCharges = 0;
            rounds = 1;
        } else {
            attack = &Attacks[weapon->myIlk ()->mGunAttack];
            rounds = c->expendAmmo (weapon);
        }
    } else if (weapon->isA (kWeapon)) {
        attack = &Attacks[weapon->myIlk ()->mGunAttack];
        rounds = c->expendAmmo (weapon);
    } else if (weapon->isA (kRayGun)) {
        attack = &Attacks[weapon->myIlk ()->mZapAttack];
        if (weapon->mCharges) {
            rounds = 1;
            --weapon->mCharges;
        } else {
            rounds = 0;
        }
    }

    assert (attack);
    return attack;
}

}
