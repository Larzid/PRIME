#include "Global.h"
#include "Util.h"
#include "Hero.h"
#include "Object.h"
#include "Interface.h"

static int
useHealthMonitor (shCreature *user, shObject *monitor)
{
    if (monitor->isBuggy ()) {
        user->msg ("Firmware malfunction.  Unable to gauge radiation level.");
        monitor->set (obj::known_bugginess);
        return 0;
    }
    user->msg (fmt ("Reported radiation dosage absorbed by your body is %s.",
        user->radLevels ()));
    return 0;
}

static int
useGloryDevice (shCreature *user, shObject *)
{
    if (user->isHero () and
        I->yn ("Activate the glory device and die a martyr's death?"))
    {
        user->die (kSuicide); /* Death routine handles everything. */
    }
    return 0;
}

static int
useSpotSkillsoft (shCreature *user, shObject *implant)
{
    if (user->intr (kBlind)) {
        user->msg ("You need sight to make use of it.");
        return 0;
    }
    if (!implant->mCharges) {
        user->msg (fmt ("%s is out of charges.", YOUR (implant)));
        if (user->isHero () and !implant->is (obj::known_charges)) {
            implant->set (obj::known_charges);
            return HALFTURN;
        } else {
            return 0;
        }
    }
    --implant->mCharges;
    if (!user->spotStuff (true))
        user->msg ("You see nothing special.");
    return FULLTURN;
}

void
initializeImplants ()
{
    AllIlks[kObjHealthMonitor].mUseFunc = useHealthMonitor;
    AllIlks[kObjGloryDevice].mUseFunc = useGloryDevice;
    AllIlks[kObjSpotSkillsoft].mUseFunc = useSpotSkillsoft;
}


const char *
describeImplantSite (shObjectIlk::Site site)
{
    switch (site) {
    case shObjectIlk::kFrontalLobe: return "frontal lobe";
    case shObjectIlk::kTemporalLobe: return "temporal lobe";
    case shObjectIlk::kParietalLobe: return "parietal lobe";
    case shObjectIlk::kOccipitalLobe: return "occipital lobe";
    case shObjectIlk::kCerebellum: return "cerebellum";
    case shObjectIlk::kLeftEar: return "left ear";
    case shObjectIlk::kRightEyeball: return "right eyeball";
    case shObjectIlk::kNeck: return "neck";
    default:
        return "brain";
    }
}

namespace imp {

bool
is_abil_booster (shObject *imp)
{
    switch (imp->mIlkId) {
    case kObjCerebralCoprocessor:
    case kObjMotoricCoordinator:
    case kObjReflexCoordinator:
    case kObjAdrenalineGenerator:
    case kObjMyfilamentHypertensor:
        return true;
    default:
        return false;
    }
    return false;
}

}
