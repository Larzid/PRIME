#include "Creature.h"
#include "Object.h"
#include "Property.h"
#include <string.h>

#define PROPS(object, ...) \
static const shProperties PROP##object [] = \
{\
__VA_ARGS__, 0, kNoIntrinsic, 0\
};\
properties[kObj##object] = PROP##object;\

const shProperties *properties[kObjNumIlks];

void
initializeProperties ()
{
    using namespace cond;
    memset (properties, 0, kObjNumIlks * sizeof (shProperties *));

PROPS(StormtrooperHelmet,
worn, kAirSupply, true);

PROPS(SpaceHelmet,
worn, kAirSupply, true);

PROPS(ElvenCH,
worn, kAirSupply, true);

PROPS(ElvenEH,
worn, kAirSupply, true);

PROPS(AquamarinePH,
worn, kAirSupply, true);

PROPS(MeanGreenPH,
worn, kAirSupply, true);

PROPS(BrotherhoodPH,
worn, kAirSupply, true,
worn | active, kLightSource, 3);

PROPS(BioMask,
worn, kAirSupply, true,
worn, kTranslation, true,
worn | toggled, kEMFieldVision, true,
worn | not_toggled, kNightVision, true);

PROPS(BrainShield,
worn, kBrainShielded, true,
worn, kReflection, 15);

PROPS(Blindfold,
worn, kBlind, true);

PROPS(PerilGlasses,
worn, kPerilSensing, true);

PROPS(NVGoggles,
worn, kNightVision, true);

PROPS(XRayGoggles,
worn | buggy,     kXRayVision, 4,
worn | debugged,  kXRayVision, 4,
worn | optimized, kXRayVision, 5);

PROPS(ShieldBelt,
worn | active, kShielded, true);

PROPS(CloakingBelt,
worn | active, kInvisible, true);

PROPS(JumpBoots,
worn, kJumpy, true);

PROPS(ChameleonSuit,
worn | uncovered, kCamouflaged, true);

PROPS(ReflecSuit,
worn | uncovered, kReflection, 70);

PROPS(DeepBluePA,
worn | active | optimized, kLightSource, 6,
worn | active | debugged,  kLightSource, 5,
worn | active | buggy,     kLightSource, 3);

PROPS(BioArmor,
worn | on_bare_skin, kAutoRegeneration, true);

PROPS(MBioArmor0,
worn | on_bare_skin, kAutoRegeneration, true);

PROPS(MBioArmor1,
worn | on_bare_skin, kAutoRegeneration, true);

PROPS(MBioArmor2,
worn | on_bare_skin, kAutoRegeneration, true);

PROPS(EBioArmor1,
worn | on_bare_skin, kAutoRegeneration, true);

PROPS(EBioArmor2,
worn | on_bare_skin, kAutoRegeneration, true);

PROPS(EBioArmor3,
worn | on_bare_skin, kAutoRegeneration, true);

PROPS(SBioArmor1,
worn | on_bare_skin, kAutoRegeneration, true);

PROPS(SBioArmor2,
worn | on_bare_skin, kAutoRegeneration, true);

PROPS(SBioArmor3,
worn | on_bare_skin, kAutoRegeneration, true);

PROPS(SBioArmor4,
worn | on_bare_skin, kAutoRegeneration, true);

PROPS(GreenCPA,
worn | on_bare_skin, kAutoRegeneration, true);

PROPS(BlueCPA,
worn, kXRayVision, 7);

PROPS(Flashlight,
active | optimized, kLightSource, 6,
active | debugged,  kLightSource, 5,
active | buggy,     kLightSource, 3);

PROPS(MotionTracker,
active | optimized, kMotionDetection, 13,
active | debugged,  kMotionDetection, 10,
active | buggy,     kMotionDetection, 7);

PROPS(GeigerCounter,
active | optimized, kRadiationDetection, 1,
active | debugged,  kRadiationDetection, 1);

PROPS(Khaydarin,
wielded | mult_chg, kTelepathy, 10);

PROPS(BabelFish,
worn, kTranslation, true);

PROPS(HealthMonitor,
worn, kHealthMonitoring, true);

PROPS(TissueRegenerator,
worn, kAutoRegeneration, true);

PROPS(Narcoleptor,
worn, kNarcolepsy, true);

PROPS(SearchSkillsoft,
worn, kAutoSearching, true);

PROPS(BOFHEye,
worn, kNightVision, true);

PROPS(Torc,
worn | buggy,     kTelepathy, 3,
worn | debugged,  kTelepathy, 6,
worn | optimized, kTelepathy, 12);

PROPS(RadiationProcessor,
worn | optimized, kRadiationProcessing, 14,
worn | debugged,  kRadiationProcessing, 9,
worn | buggy,     kRadiationProcessing, 4);

PROPS(WarpStabilizer,
worn | mult_enhc | buggy,     kWarpControl, 3,
worn | mult_enhc | debugged,  kWarpControl, 6,
worn | mult_enhc | optimized, kWarpControl, 9);

}

extern bool isIntrAdditive (shIntrinsic intr);

void
shObject::applyConferredIntrinsics (shCreature *target)
{
    if (!properties[mIlkId])  return;
    for (const shProperties *p = properties[mIlkId]; p->value; ++p) {
        unsigned int i = 1;
        int value = p->value;
        bool ok = true;
        /* Check all preconditions. */
        for (; ok and i <= cond::sword_skill; i <<= 1)
            if (p->cond & i)  switch (i) {
            case cond::buggy:
                ok = isBuggy (); break;
            case cond::debugged:
                ok = isDebugged (); break;
            case cond::optimized:
                ok = isOptimized (); break;
            case cond::nonbuggy:
                ok = !isBuggy (); break;
            case cond::toggled:
                ok = is (obj::toggled); break;
            case cond::not_toggled:
                ok = !is (obj::toggled); break;
            case cond::active:
                ok = is (obj::active); break;
            case cond::inactive:
                ok = !is (obj::active); break;
            case cond::wielded:
                ok = is (obj::wielded); break;
            case cond::worn:
                ok = is (obj::worn); break;
            case cond::has_charges:
                ok = mCharges; break;
            case cond::uncovered:
                if (target->mBodyArmor == this and
                    (target->mCloak and
                     !target->mCloak->isA (kObjTransparentJacket)))
                    ok = false;
                else if (target->mJumpsuit == this and
                         (target->mBodyArmor or
                          (target->mCloak and
                           !target->mCloak->isA (kObjTransparentJacket))))
                    ok = false;
                else if (target->mGoggles == this and
                         target->mHelmet and target->mHelmet->isSealedArmor ())
                    ok = false;
                break;
            case cond::on_bare_skin:
                if (target->mBodyArmor == this and target->mJumpsuit)
                    ok = false;
                else if (target->mCloak == this and
                         (target->mBodyArmor or target->mJumpsuit))
                    ok = false;
                break;
            /* Some conditions are not really conditions.  They affect value. */
            case cond::mult_chg:
                value *= mCharges; break;
            case cond::mult_enhc:
                value *= mEnhancement; break;
            case cond::sword_skill:
                value *= target->getSkillModifier (kSword);
                break;
            default:
                assert (false);
                ok = false;
                break;
            }
        /* If any one condition was not matched ignore this boon. */
        if ((i >> 1) != cond::sword_skill)  continue;
        /* Add the intrinsic. */
        if (isIntrAdditive (p->intr)) {
            target->mIntrinsics.mod (p->intr, value);
        } else {
            int current = target->mIntrinsics.get (p->intr);
            target->mIntrinsics.set (p->intr, maxi (current, value));
        }
    }
}
