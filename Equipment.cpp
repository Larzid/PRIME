#include "Equipment.h"
#include "Creature.h"
#include <string.h>
#include <ctype.h>

const EquipmentLine *stuff[kMonNumberOf];

static void
alter_flag (shObject *obj, char flag, bool clear)
{
    obj::Flag f;
    switch (flag) {
    case 'c': f = obj::cracked; break;
    case 'i': f = obj::infected; break;
    case 'p': f = obj::fooproof; break;
    case 't': f = obj::toggled; break;
    default: f = obj::Flag (0); break;
    }

    if (clear)
        obj->clear (f);
    else
        obj->set (f);
}

static void
object_property (shObject *obj, const char *property)
{
    size_t ln = strlen (property);
    if ((ln == 1 and !isdigit (property[0])) or
        (ln == 2 and property[0] == '!'))
    {
        alter_flag (obj, property[ln-1], ln == 2);
        return;
    }

    switch (property[0]) {
    case '+': case '-':
    {
        long enh = strtol (property, NULL, 10);
        obj->mEnhancement = enh;
        return;
    }
    case 'b':
    {
        long beatitude = strtol (&property[2], NULL, 10);
        if (property[1] == '=') {
            obj->mBugginess = beatitude;
        } else if (property[1] == '!' and obj->mBugginess == beatitude) {
            obj->mBugginess = beatitude ? 0 : RNG (2) * 2 - 1;
        }
        return;
    }
    case 'c': /* syntax:  c:15..30 */
    {
        char *max = NULL;
        long chrg_min = strtol (&property[2], &max, 10);
        long chrg_max = chrg_min;
        if (*max) {
            chrg_max = strtol (&max[2], NULL, 10);
            obj->mCharges = RNG (chrg_min, chrg_max);
        } else {
            obj->mCharges = chrg_min;
        }
        return;
    }
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': /* syntax:  6d8+15 */
    {
        char *next = NULL;
        long dice = strtol (property, &next, 10);
        if (!*next) {
            obj->mCount = dice;
            return;
        }
        if (*next == '%') /* this is percentage chance, ignore token */
            return;
        long die = strtol (&next[1], &next, 10);
        obj->mCount = NDX (dice, die);
        if (!*next) {
            return;
        }
        long mod = strtol (next, NULL, 10);
        obj->mCount += mod;
        return;
    }
    default:
        break;
    }
}

static void
modify_object (shObject *obj, const char *base_mod_str)
{
    if (!base_mod_str)
        return;

    char *mod_str = GetBuf ();
    strcpy (mod_str, base_mod_str);
    char *option = strtok (mod_str, " ");
    while (option) {
        object_property (obj, option);
        option = strtok (NULL, " ");
    }
}

void
equip_by_list (shCreature *c, const EquipmentLine *table)
{
    bool do_or = false;
    bool do_and = false;
    //int gotweapon = 0;
    for (const EquipmentLine *item = table; item->ref.dummy; ++item)
    {

        if (!item->mod) {

        /* the pipe symbol indicates don't try to create
           the object unless the previous obj failed */
        } else if (item->mod[0] == '|' and !do_or) {
            do_or = true;
            do_and = false;
            continue;

        /* the comma symbol indicates create the
           object only if the previous obj was made */
        } else if (item->mod[0] == ',' and !do_and) {
            continue;
        }

        shObject *obj = NULL;

        bool rejected = false;
        const char *pos;
        if (item->mod and (pos = strchr (item->mod, '%'))) {
            while (pos > item->mod and isdigit (*(pos-1)))
                --pos;

            long chance = atoi (pos);
            if (RNG (100) >= chance)
                rejected = true;
        }

        if (rejected) {

        } else if (item->mod and item->mod[strlen (item->mod)-1] == 's') {
            obj = gen_obj_type (item->ref.set);
        } else {
            obj = new shObject (item->ref.id);
        }

        if (!obj) {
            do_or = true;
            do_and = false;
            continue;
        }
        do_or = false;
        do_and = true;
        modify_object (obj, item->mod);
        c->addObjectToInventory (obj, true);
#if 0
        if (!mWeapon and obj->isA (kWeapon)) {
            if (obj->myIlk ()->mMeleeSkill != kNoSkillCode) {
                gainRank (obj->myIlk ()->mMeleeSkill, 1 + mCLevel * 2/3);
            }
            if (obj->myIlk ()->mGunSkill != kNoSkillCode) {
                gainRank (obj->myIlk ()->mGunSkill, 1 + mCLevel * 2/3);
            }
            ++gotweapon;
            /* don't wield until hero is in sight so he can see message */
            //wield (obj, 1);
        } else if (obj->isA (kArmor) or obj->isA (kImplant)) {
            don (obj, 1);
        }
#endif
    }
}

void
equip_monster (shCreature *c)
{
    if (!stuff[c->mIlkId])
        return;

    equip_by_list (c, stuff[c->mIlkId]);
}

#define EQUIP(creature, ...) \
static EquipmentLine EQ##creature [] = \
{\
__VA_ARGS__, {0, NULL}\
};\
stuff[creature] = EQ##creature;

#define STUFF(creature, ...) \
static const char *ST##creature [] = \
{\
__VA_ARGS__, NULL\
};\
equipment[creature] = ST##creature;

const char **equipment[kMonNumberOf];

void
initializeEquipment ()
{
    memset (stuff, 0, kMonNumberOf * sizeof (EquipmentLine *));
    memset (equipment, 0, kMonNumberOf * sizeof (char *));

EQUIP(kMonLittleGreyAlien,
{kObjAnalProbe, NULL},
{kObjReticulanJumpsuit, "75%"},
{kObjBioArmor, "| 5%"},
{kObjZapGun, "25%"},
{kObjEnergyCell, ", 50% 3d3"}
);

EQUIP(kMonMutantHuman,
{kObjBadassTrenchcoat, "1%"},
{kObjLoserAnorak, "| 1%"},
{kObjNanoCola, "5%"},
{kObjTorc, "5%"}
);

EQUIP(kMonSpaceGoblin,
{kObjPeaShooter, "10%"},
{kObjClub,"| 15%"},
{kObjMoney, "80% 4d8"}
);

EQUIP(kMonWebCrawler,
{(uintptr_t)&prob::FloppyDisk, "25% s"}
);

EQUIP(kMonKamikazeGoblin,
{kObjPeaShooter, "10%"},
{(uintptr_t)&prob::Grenade, "1d3 s"},
{kObjMoney, "80% 5d10"}
);

EQUIP(kMonRedshirt,
{kObjPhaserPistol, NULL},
{kObjEnergyCell, "4d20"},
{kObjSturdyBoots, "5%"},
{kObjTricorder, "5%"}
);

EQUIP(kMonSpaceOrc,
{kObjConvPistol, "50%"},
{kObjBullet, ", 3d10"},
{kObjPeaShooter, "|"},
{kObjPowerClaw, "50%"},
{(uintptr_t)&prob::RayGun, "1% s"}
);

EQUIP(kMonCylonCenturion,
{kObjLaserPistol, NULL},
{kObjEnergyCell, "4d20"}
);

EQUIP(kMonScrubbot,
{kObjSquirtRayGun, "15%"}
);

EQUIP(kMonStormtrooper,
{kObjStormtrooperSuit, NULL},
{kObjStormtrooperHelmet, NULL},
{kObjStormtrooperBoots, NULL},
{kObjBlasterPistol, NULL},
{kObjEnergyCell, "4d20"}
);

EQUIP(kMonHighPingBastard,
{kObjBrassKnuckles, "15%"},
{kObjFlakJacket, NULL},
{kObjShotgun, "90%"},
{kObjShotgunShell, ", 1d12"},
{kObjPeaShooter, "|"},
{kObjBadassTrenchcoat, "1%"},
{kObjLoserAnorak, "| 3%"}
);

EQUIP(kMonSpaceOrcPyromaniac,
{kObjAsbestosJumpsuit, "5%"},
{kObjLightFlamer, "90%"},
{kObjFlamerFuel, ", 1"},
{kObjFlamer, "|"},
{kObjFlamerFuel, ", 2"},
{kObjPowerClaw, "50%"},
{(uintptr_t)&prob::RayGun, "1% s"}
);

EQUIP(kMonCreepingCredits,
{kObjMoney, "4d100"}
);

EQUIP(kMonMelnorme,
{kObjMoney, "3d50"},
{(uintptr_t)&prob::RayGun, "15% s"},
{kObjLaserRifle, "80%"},
{kObjPlasmaPistol, "|"},
{kObjEnergyCell, "8d10"}
);

EQUIP(kMonOrzScubaDiver,
{kObjSpaceHelmet, NULL},
{kObjLaserRifle, NULL},
{kObjEnergyCell, "5d20"}
);

EQUIP(kMonCylonCommandCenturion,
{kObjLaserRifle, NULL},
{kObjEnergyCell, "6d20"}
);

EQUIP(kMonKlingon,
{kObjFlakJacket, NULL},
{kObjBatleth, "20%"},
{kObjPhaserPistol, "|"},
{kObjEnergyCell, ", 3d20"}
);

EQUIP(kMonTroubleshooter,
{kObjReflecSuit, "75%"},
{kObjLaserPistol, "75% b=-1"},
{kObjLaserPistol, "|"},
{kObjStunGrenade, "5% 1d2"},
{kObjFragGrenade, "5% 1d2"},
{kObjConcussionGrenade, "5% 1d2"},
{kObjRadGrenade, "5% 1d2"},
{kObjEnergyCell, "6d20"},
{(uintptr_t)&prob::RayGun, "10% b=-1 s"},
{(uintptr_t)&prob::RayGun, "| 6% s"},
{kObjMotionTracker, "2% b=-1"},
{kObjB3, "40%"},
{kObjB3, "4%"}
);

EQUIP(kMonCheerleaderNinja,
{kObjKatana, "10%"},
{kObjShuriken, "2d3"}
);

EQUIP(kMonBorg,
{kObjPhaserPistol, NULL},
{kObjEnergyCell, "3d20"},
{(uintptr_t)&prob::Implant, "20% s"}
);

EQUIP(kMonSpaceOrcBoss,
{kObjFlakJacket, NULL},
{kObjShotgun, "30%"},
{kObjShotgunShell, ", 3d12"},
{kObjLaserRifle, "| 50%"},
{kObjEnergyCell, ", 4d20"},
{kObjPlasmaRifle, "|"},
{kObjEnergyCell, ", 5d20"},
{(uintptr_t)&prob::RayGun, "10% s"},
{kObjPowerClaw, NULL}
);

EQUIP(kMonKlingonCaptain,
{kObjFlakJacket, NULL},
{kObjBatleth, "70%"},
{kObjPhaserPistol, NULL},
{kObjEnergyCell, "3d20"},
{(uintptr_t)&prob::RayGun, "15% s"}
);

EQUIP(kMonMiGo,
{kObjBrain, "50%"}
);

EQUIP(kMonBeneGesserit,
{kObjGomJabbar, "50%"},
{kObjTorc, "30% b=0"},
{kObjTorc, "| 5% b=+1"}
);

EQUIP(kMonStormtrooperCommander,
{kObjStormtrooperSuit, "20% +1 f"},
{kObjStormtrooperSuit, "| +1"},
{kObjStormtrooperHelmet, "20% +1 f"},
{kObjStormtrooperHelmet, "| +1"},
{kObjStormtrooperBoots, "20% +1 f"},
{kObjStormtrooperBoots, "| +1"},
{kObjBlasterRifle, "b!-1"},
{kObjEnergyCell, "20d5"}
);

EQUIP(kMonKlingonCommander,
{kObjFlakJacket, "80% +2"},
{kObjFlakJacket, "| +4"},
{kObjBatleth, "40% +2 f"},
{kObjBatleth, "| b!-1 f"},
{kObjPhaserPistol, "80% +3 b!-1"},
{kObjPhaserPistol, "| +6 b!-1"},
{kObjEnergyCell, "3d20"},
{(uintptr_t)&prob::RayGun, "25% b!-1 s"}
);

EQUIP(kMonUsenetTroll,
{kObjAsbestosJumpsuit, "25%"},
{kObjAsbestosJacket, "10%"},
{(uintptr_t)&prob::Computer, "50% b=-1 s"},
{(uintptr_t)&prob::Computer, "| s"},
{(uintptr_t)&prob::FloppyDisk, "60% s"},
{(uintptr_t)&prob::FloppyDisk, "50% s"},
{(uintptr_t)&prob::FloppyDisk, "40% s"},
{(uintptr_t)&prob::FloppyDisk, "30% s"},
{(uintptr_t)&prob::FloppyDisk, "5% c s"}
);

EQUIP(kMonLowPingBastard,
{kObjSpikedKnuckles, "15%"},
{kObjKevlarJacket, "40% +2"},
{kObjKevlarJacket, "|"},
{kObjShotgun, NULL},
{kObjShotgunShell, ", 1d12"},
{kObjRailgun, "90%"},
{kObjRailgunSlug, ", 1d12"},
{kObjBadassTrenchcoat, "5%"}
);

EQUIP(kMonSpaceElf,
{kObjElvenJumpsuit, NULL},
{kObjElvenCH, "25%"},
{kObjElvenCA, "15%"},
{kObjElvenSword, "10%"},
{kObjElvenDagger, "| +1"},
{kObjLaserPistol, NULL},
{(uintptr_t)&prob::RayGun, "15% s"},
{kObjEnergyCell, "4d10"}
);

EQUIP(kMonCyborgNinja,
{kObjChameleonSuit, NULL},
{kObjKatana, "90%"},
{kObjLightSaber, "|"},
{(uintptr_t)&prob::Implant, "35% s"},
{(uintptr_t)&prob::Implant, "5% s"},
{kObjShuriken, "75% 3d2"},
{kObjBadassTrenchcoat, "10%"}
);

EQUIP(kMonMutantNinjaTurtle,
{kObjNunchaku, "50%"},
{kObjBoStaff, "|"},
{(uintptr_t)&prob::RayGun, "10% s"}
);

EQUIP(kMonTallGreyAlien,
{kObjAnalProbe, NULL},
{kObjEnergyCell, "50% 2d10"},
{kObjZapGun, ","},
{kObjReticulanJumpsuit, NULL},
{kObjEnergyDome, "25%"},
{kObjHarmonyDome, "| 5%"},
{kObjShieldBelt, "5%"}
);

EQUIP(kMonBrotherhoodPaladin,
{kObjBrotherhoodPA, NULL},
{kObjBrotherhoodPH, NULL},
{kObjOrdinaryJumpsuit, "10%"},
{(uintptr_t)&prob::Jumpsuit, "| 1% s"},
{kObjEnergyCell, "5d20"},
{kObjPlasmaPistol, "50%"},
{kObjPowerFist, ","},
{kObjPlasmaRifle, "|"},
{kObjHealingCanister, NULL},
{kObjRadAway, "35%"},
{(uintptr_t)&prob::Canister, "80% s"}
);

EQUIP(kMonSorceress,
{kObjTorc, "20% b=0"}
);

EQUIP(kMonSpaceElfLord,
{kObjElvenDagger, "+1"},
{kObjElvenJumpsuit, NULL},
{kObjElvenSpaceSuit, "50%"},
{kObjElvenEA, "| 50%"},
{kObjLaserRifle, "75%"},
{kObjPlasmaRifle, "|"},
{kObjLightSaber, "5%"},
{kObjElvenSword, "30%"},
{kObjKatana, "|"},
{kObjElvenCH, "60%"},
{kObjElvenEH, "|"},
{(uintptr_t)&prob::RayGun, "25% s"},
{kObjTorc, "50% b=+1"},
{kObjEnergyCell, "5d10"}
);

EQUIP(kMonClerkbot,
{kObjMoney, "20d10"},
{kObjBeer, "1d3"},
{kObjShieldBelt, "25%"},
{kObjEnergyCell, ", 4d50"}
);

EQUIP(kMonDocbot,
{kObjMoney, "1d400"},
{kObjBeer, "1d3"},
{kObjRadAway, "50% 1d2"},
{kObjHealingCanister, "50% 1d2"},
{kObjFullHealingCanister, "50%"},
{kObjRestorationCanister, "50% 1d2"},
{kObjGainAbilityCanister, "10%"}
);

EQUIP(kMonAMarine,
{kObjM56Smartgun, "5%"},
{kObjBullet, ", 6d10"},
{kObjSniperRifle, "| 5%"},
{kObjBullet, ", 3d5"},
{kObjChainsaw, ","},
{kObjPulseRifle, "| 20%"},
{kObjBullet, ", 6d10"},
{kObjAssaultPistol, "|"},
{kObjBullet, ", 2d10"},
{kObjChainsaw, ","},
{kObjEnergyCell, ", 5d4"},
{kObjAquamarinePA, NULL},
{kObjAquamarinePH, NULL},
{kObjHealingCanister, "10%"},
{kObjAdamantineCloak, "20%"}
);

EQUIP(kMonMGMarine,
{kObjPulseRifle, "50%"},
{kObjBullet, ", 8d10"},
{kObjAssaultPistol, "|"},
{kObjBullet, ", 2d10"},
{kObjPowerFist, ","},
{kObjMeanGreenPA, NULL},
{kObjMeanGreenPH, NULL},
{kObjEnergyCell, "40% 4d10"},
{kObjHealingCanister, "40%"},
{kObjRestorationCanister, "20%"},
{kObjAdamantineCloak, "3%"}
);

EQUIP(kMonReticulanDissident,
{kObjAnalProbe, "85%"},
{kObjZapGun, NULL},
{kObjEnergyCell, "1d100"},
{kObjBioArmor, NULL},
{kObjBioComputer, "80%"},
{(uintptr_t)&prob::FloppyDisk, "1 s"},
{(uintptr_t)&prob::FloppyDisk, "66% s"},
{(uintptr_t)&prob::FloppyDisk, "33% s"},
{kObjEnergyDome, "25%"},
{kObjHarmonyDome, "| 5%"},
{kObjShieldBelt, "50%"},
{kObjEnergyCell, ", 10d4"}
);

EQUIP(kMonSpaceElfQueen,
{kObjElvenJumpsuit, "50% b=+1"},
{kObjEnergyDome, "33%"},
{kObjElvenEH, "|"},
{kObjElvenSpaceSuit, "50%"},
{kObjElvenEA, "|"},
{kObjLightSaber, "20%"},
{kObjElvenSword, "| b=+1"},
{kObjEnergyCell, "2d10"},
{kObjTorc, "80% b=+1"}
);

EQUIP(kMonCCMarine,
{kObjChaosSword, "15%"},
{kObjPlasmaRifle, "| 50%"},
{kObjEnergyCell, ", 10d10"},
{kObjPulseRifle, "|"},
{kObjBullet, ", 8d10"},
{kObjPowerFist, "40%"},
{kObjChainsaw, "| 75%"},
{kObjEnergyCell, ", 10"},
{kObjPowerClaw, "|"},
{(uintptr_t)&prob::ChaosArmor, "s"},
{kObjEnergyCell, "40% 5d10"},
{kObjHealingCanister, "40%"},
{kObjRestorationCanister, "30%"},
{kObjMutagenCanister, "40%"},
{kObjAdamantineCloak, "1%"}
);

EQUIP(kMonZealot,
{kObjPsiBlades, NULL},
{kObjPowerSuit, NULL},
{kObjZealotLegEnhancements, "20%"},
{kObjKhaydarin, "10%"}
);

EQUIP(kMonHighTemplar,
{kObjPsiBlades, "15%"},
{(uintptr_t)&prob::HandEnergyGun, "s"},
{kObjEnergyCell, "5d10"},
{kObjKhaydarin, "50%"}
);

EQUIP(kMonDalek,
{kObjEnergyCell, "1d101+49"}
);

EQUIP(kMonDarkTemplar,
{kObjWarpScythe, "+0"},
{kObjKhaydarin, "5%"}
);

EQUIP(kMonWarbot,
{kObjLaserCannon, "50%"},
{kObjPlasmaCannon, "|"},
{kObjEnergyCell, "300"},
{kObjFragGrenade, "20% 1d3"},
{(uintptr_t)&prob::RayGun, "80% s"}
);

EQUIP(kMonSecuritron,
{kObjAutoCannon, NULL},
{kObjBullet, "1d61+59"},
{kObjFlashbang, "50%"},
{kObjStunGrenade, "|"},
{(uintptr_t)&prob::RayGun, "80% s"}
);

EQUIP(kMonGuardbot,
{kObjLaserCannon, "80%"},
{kObjPlasmaCannon, "|"},
{kObjEnergyCell, "200"},
{(uintptr_t)&prob::RayGun, "50% s"}
);

EQUIP(kMonKohrAh,
{kObjExcruciator, "85%"},
{kObjBrainShield, "15%"},
{kObjEnergyCell, "5d25"}
);

EQUIP(kMonBOFH,
{(uintptr_t)&prob::RayGun, "s"},
{kObjPeaShooter, NULL},
{kObjOrdinaryJumpsuit, "+2"},
{kObjReflecSuit, "+1"},
{kObjBOFHEye, NULL},
{(uintptr_t)&prob::Computer, "s"},
{(uintptr_t)&prob::FloppyDisk, "c s"},
{(uintptr_t)&prob::FloppyDisk, "75% s"},
{(uintptr_t)&prob::FloppyDisk, "75% s"}
);

EQUIP(kMonAgent,
{kObjBadassTrenchcoat, "+2 b=+1"},
{kObjSunglasses, "b=+1"}
);

EQUIP(kMonShodan,
{kObjTheOrgasmatron, NULL}
);
}
