#include <stdlib.h>
#include "Distribution.h"

/* NeverRandomlyGenerated
    kObjNothing
    kObjTheOrgasmatron
    kObjFakeOrgasmatron1
    kObjFakeOrgasmatron2
    kObjFakeOrgasmatron3
    kObjFakeOrgasmatron4
    kObjFakeOrgasmatron5
    kObjBOFHEye
    kObjBluePill
    kObjRedPill
    kObjWreck
    kObjNumIlks
*/

namespace prob {

Table Money[] =
{
    {20,   1,  24, 'i', kObjMoney},
    {50,  25,  50, 'i', kObjMoney},
    {20,  51,  99, 'i', kObjMoney},
    { 9, 100, 250, 'i', kObjMoney},
    { 1, 251, 500, 'i', kObjMoney},
    END_TOKEN
};

Table RayGun[] =
{
    {6, 1, 1, 'i', kObjTransporterRayGun},
    {6, 1, 1, 'i', kObjHeatRayGun},
    {4, 1, 1, 'i', kObjEmptyRayGun},
    {4, 1, 1, 'i', kObjFreezeRayGun},
    {4, 1, 1, 'i', kObjGaussRayGun},
    {4, 1, 1, 'i', kObjPoisonRayGun},
    {4, 1, 1, 'i', kObjGammaRayGun},
    {3, 1, 1, 'i', kObjStasisRayGun},
    {3, 1, 1, 'i', kObjHealingRayGun},
    {3, 1, 1, 'i', kObjAccelerationRayGun},
    {3, 1, 1, 'i', kObjDecelerationRayGun},
    {2, 1, 1, 'i', kObjRestorationRayGun},
    {2, 1, 1, 'i', kObjSquirtRayGun},
    {2, 1, 1, 'i', kObjDecontaminationRayGun},
    {1, 1, 1, 'i', kObjDisintegrationRayGun},
    {1, 1, 1, 'i', kObjAugmentationRayGun},
    END_TOKEN
};


Table Ammunition[] =
{
    {60, 10, 30, 'i', kObjBullet},
    {20, 15, 25, 'i', kObjShotgunShell},
    { 5,  3, 10, 'i', kObjRailgunSlug},
    { 5,  1,  1, 'i', kObjFlamerFuel},
    { 5,  6, 20, 'i', kObjWebCasket},
    { 5, 10, 30, 'i', kObjSpearheadDart},
    END_TOKEN
};


Table Grenade[] =
{
    {8, 2, 6, 'i', kObjConcussionGrenade},
    {6, 2, 6, 'i', kObjFragGrenade},
    {6, 2, 6, 'i', kObjStunGrenade},
    {6, 2, 6, 'i', kObjFlashbang},
    {4, 2, 4, 'i', kObjIncendiaryGrenade},
    {4, 2, 4, 'i', kObjRadGrenade},
    {4, 2, 4, 'i', kObjEMPGrenade},
    {4, 2, 4, 'i', kObjWebbingGrenade},
    {4, 2, 4, 'i', kObjDispersalGrenade},
    {2, 2,12, 'i', kObjFlare},
    {1, 2, 4, 'i', kObjFootball},
    END_TOKEN
};

Table Skillsoft[] =
{
    {1, 1, 1, 'i', kObjLockPickSkillsoft},
    {1, 1, 1, 'i', kObjSearchSkillsoft},
    {1, 1, 1, 'i', kObjRepairSkillsoft},
    {1, 1, 1, 'i', kObjSpotSkillsoft},
    END_TOKEN
};

Table AbilModImplant[] =
{
    {1, 1, 1, 'i', kObjAdrenalineGenerator},
    {1, 1, 1, 'i', kObjMyfilamentHypertensor},
    {1, 1, 1, 'i', kObjReflexCoordinator},
    {1, 1, 1, 'i', kObjMotoricCoordinator},
    {1, 1, 1, 'i', kObjCerebralCoprocessor},
    {1, 1, 1, 'i', kObjPsiAmp},
    END_TOKEN
};

Table OtherImplant[] =
{
    {1, 1, 1, 'i', kObjHealthMonitor},
    {1, 1, 1, 'i', kObjRadiationProcessor},
    {1, 1, 1, 'i', kObjPoisonResistor},
    {1, 1, 1, 'i', kObjTissueRegenerator},
    {1, 1, 1, 'i', kObjShockCapacitor},
    {1, 1, 1, 'i', kObjMechaDendrites},
    {1, 1, 1, 'i', kObjWarpStabilizer},
    {1, 1, 1, 'i', kObjTorc},
    {1, 1, 1, 'i', kObjBabelFish},
    {1, 1, 1, 'i', kObjUngooma},
    {1, 1, 1, 'i', kObjNarcoleptor},
    {1, 1, 1, 'i', kObjGloryDevice},
    {1, 1, 1, 'i', kObjExcruciator},
    END_TOKEN
};

Table Implant[] =
{
    {65, 1, 1, 's', (uintptr_t)&OtherImplant},
    {25, 1, 1, 's', (uintptr_t)&AbilModImplant},
    {10, 1, 1, 's', (uintptr_t)&Skillsoft},
    END_TOKEN
};

Table CommonDisk[] =
{
    {1, 1, 1, 'i', kObjBugDetectionDisk},
    {1, 1, 1, 'i', kObjIdentifyDisk},
    {1, 1, 1, 'i', kObjSpamDisk},
    {1, 1, 1, 'i', kObjDebuggerDisk},
    {1, 1, 1, 'i', kObjLogicBombDisk},
    {1, 1, 1, 'i', kObjAntivirusDisk},
    END_TOKEN
};

Table RareDisk[] =
{
    {1, 1, 1, 'i', kObjHackingDisk},
    {1, 1, 1, 'i', kObjMatterCompiler},
    {1, 1, 1, 'i', kObjOperatingSystem},
    {1, 1, 1, 'i', kObjResidentAntivirusDisk},
    END_TOKEN
};

Table OtherDisk[] =
{
    {1, 1, 1, 'i', kObjBlankDisk},
    {1, 1, 1, 'i', kObjComputerVirus},
    {1, 1, 1, 'i', kObjCorruptDisk},
    {1, 1, 1, 'i', kObjDiagnosticsDisk},
    {1, 1, 1, 'i', kObjEnhanceArmorDisk},
    {1, 1, 1, 'i', kObjEnhanceImplantDisk},
    {1, 1, 1, 'i', kObjEnhanceWeaponDisk},
    {1, 1, 1, 'i', kObjHypnosisDisk},
    {1, 1, 1, 'i', kObjLifeformDetectionDisk},
    {1, 1, 1, 'i', kObjMappingDisk},
    {1, 1, 1, 'i', kObjObjectDetectionDisk},
    {1, 1, 1, 'i', kObjRecallDisk},
    {1, 1, 1, 'i', kObjRecursiveLoopDisk},
    {1, 1, 1, 'i', kObjReformatDisk},
    {1, 1, 1, 'i', kObjTransportDisk},
    {1, 1, 1, 'i', kObjWarpTracerDisk},
    END_TOKEN
};

Table FloppyDisk[] =
{
    {55, 1, 1, 's', (uintptr_t)&OtherDisk},
    {35, 1, 1, 's', (uintptr_t)&CommonDisk},
    {10, 1, 1, 's', (uintptr_t)&RareDisk},
    END_TOKEN
};

Table Can5[] =
{
    {11, 1, 1, 'i', kObjBeer},
    { 5, 1, 1, 'i', kObjNanoCola},
    { 5, 1, 1, 'i', kObjNukaCola},
    { 5, 1, 1, 'i', kObjCoffee},
    { 5, 1, 1, 'i', kObjSuperGlue},
    { 3, 1, 1, 'i', kObjB3},
    { 1, 1, 1, 'i', kObjWater},
    END_TOKEN
};

Table Can50[] =
{
    {5, 1, 1, 'i', kObjHealingCanister},
    {3, 1, 1, 'i', kObjRadAway},
    {2, 1, 1, 'i', kObjRestorationCanister},
    END_TOKEN
};

Table Can75[] =
{
    {1, 1, 1, 'i', kObjLNO},
    {1, 1, 1, 'i', kObjNapalm},
    END_TOKEN
};

Table Can100[] =
{
    {2, 1, 1, 'i', kObjPlasmaCanister},
    {2, 1, 1, 'i', kObjSpeedCanister},
    {2, 1, 1, 'i', kObjPoisonCanister},
    {1, 1, 1, 'i', kObjBuffout},
    {1, 1, 1, 'i', kObjUniversalSolvent},
    END_TOKEN
};

Table Can200[] =
{
    {2, 1, 1, 'i', kObjMutagenCanister},
    {2, 1, 1, 'i', kObjFullHealingCanister},
    {2, 1, 1, 'i', kObjSpiceMelange},
    {1, 1, 1, 'i', kObjGainAbilityCanister},
    END_TOKEN
};

Table Can1000[] =
{
    {2, 1, 1, 'i', kObjAntimatter},
    {2, 1, 1, 'i', kObjBrain},
    {1, 1, 1, 'i', kObjCannedEmbryo},
    END_TOKEN
};

Table Canister[] =
{
    {5, 1, 1, 's', (uintptr_t)&Can5},
    {4, 1, 1, 's', (uintptr_t)&Can50},
    {3, 1, 1, 's', (uintptr_t)&Can75},
    {4, 1, 1, 's', (uintptr_t)&Can100},
    {4, 1, 1, 's', (uintptr_t)&Can200},
    {2, 1, 1, 's', (uintptr_t)&Can1000},
    END_TOKEN
};

Table Beverage[] =
{
    {9, 1, 1, 'i', kObjBeer},
    {4, 1, 1, 'i', kObjNanoCola},
    {4, 1, 1, 'i', kObjNukaCola},
    {4, 1, 1, 'i', kObjB3},
    {3, 1, 1, 'i', kObjCoffee},
    {1, 1, 1, 'i', kObjWater},
    END_TOKEN
};


Table VeryRareTool[] =
{
    {2, 1, 1, 'i', kObjLargeEnergyTank},
    {2, 1, 1, 'i', kObjFissionPlant},
    {1, 1, 1, 'i', kObjFusionPlant},
    {1, 1, 1, 'i', kObjMedicomp},
    {1, 1, 1, 'i', kObjKhaydarin},
    END_TOKEN
};

Table RareTool[] =
{
    {3, 1, 1, 'i', kObjRemoteControl},
    {3, 1, 1, 'i', kObjMasterKeycard},
    {3, 1, 1, 'i', kObjTransferCable},
    {1, 1, 1, 'i', kObjMegaComputer},
    {1, 1, 1, 'i', kObjBioComputer},
    {1, 1, 1, 'i', kObjSatCom},
    END_TOKEN
};

Table UncommonTool[] =
{
    {1, 1, 1, 'i', kObjTricorder},
    {1, 1, 1, 'i', kObjDroidCaller},
    {1, 1, 1, 'i', kObjGeigerCounter},
    {1, 1, 1, 'i', kObjMotionTracker},
    {1, 1, 1, 'i', kObjLockPick},
    {1, 1, 1, 'i', kObjSmallEnergyTank},
    {1, 1, 1, 'i', kObjRestrainingBolt},
    {1, 1, 1, 'i', kObjMiniComputer},
    END_TOKEN
};

Table CommonTool[] =
{
    {5, 1, 1, 'i', kObjJunk},
    {5, 1, 1, 'i', kObjDuctTape},
    {4, 1, 1, 'i', kObjFlashlight},
    {3, 1, 1, 'i', kObjProximityMine},
    {3, 1, 1, 'i', kObjPortableHole},
    {3, 1, 1, 'i', kObjMonkeyWrench},
    {1, 1, 1, 'i', kObjKeycard1},
    {1, 1, 1, 'i', kObjKeycard2},
    {1, 1, 1, 'i', kObjKeycard3},
    {1, 1, 1, 'i', kObjKeycard4},
    END_TOKEN
};

Table Tool[] =
{
    { 5, 1, 1, 's', (uintptr_t)&VeryRareTool},
    {15, 1, 1, 's', (uintptr_t)&RareTool},
    {25, 1, 1, 's', (uintptr_t)&UncommonTool},
    {55, 1, 1, 's', (uintptr_t)&CommonTool},
    END_TOKEN
};

Table Gizmo[] =
{
    {1, 1, 1, 'i', kObjRemoteControl},
    {1, 1, 1, 'i', kObjTricorder},
    {1, 1, 1, 'i', kObjDroidCaller},
    {1, 1, 1, 'i', kObjGeigerCounter},
    {1, 1, 1, 'i', kObjMotionTracker},
    END_TOKEN
};

Table Keycard[] =
{
    {1, 1, 1, 'i', kObjKeycard1},
    {1, 1, 1, 'i', kObjKeycard2},
    {1, 1, 1, 'i', kObjKeycard3},
    {1, 1, 1, 'i', kObjKeycard4},
    {1, 1, 1, 'i', kObjMasterKeycard},
    END_TOKEN
};

Table PowerPlant[] =
{
    {2, 1, 1, 'i', kObjFissionPlant},
    {1, 1, 1, 'i', kObjFusionPlant},
    END_TOKEN
};

Table Computer[] =
{
    {3, 1, 1, 'i', kObjMiniComputer},
    {2, 1, 1, 'i', kObjMegaComputer},
    {2, 1, 1, 'i', kObjSatCom},
    {1, 1, 1, 'i', kObjBioComputer},
    END_TOKEN
};

Table Jumpsuit[] =
{
    {10, 1, 1, 'i', kObjOrdinaryJumpsuit},
    { 5, 1, 1, 'i', kObjAsbestosJumpsuit},
    { 3, 1, 1, 'i', kObjJanitorUniform},
    { 3, 1, 1, 'i', kObjElvenJumpsuit},
    { 2, 1, 1, 'i', kObjRadiationSuit},
    { 1, 1, 1, 'i', kObjChameleonSuit},
    { 1, 1, 1, 'i', kObjReticulanJumpsuit},
    END_TOKEN
};

Table Belt[] =
{
    {4, 1, 1, 'i', kObjFashionBelt},
    {3, 1, 1, 'i', kObjArmoredBelt},
    {2, 1, 1, 'i', kObjShieldBelt},
    {2, 1, 1, 'i', kObjEnergyBelt},
    {2, 1, 1, 'i', kObjSuspensorBelt},
    {1, 1, 1, 'i', kObjCloakingBelt},
    {1, 1, 1, 'i', kObjStabilizerBelt},
    END_TOKEN
};

Table Goggles[] =
{
    { 5, 1, 1, 'i', kObjSunglasses},
    { 5, 1, 1, 'i', kObjPerilGlasses},
    { 2, 1, 1, 'i', kObjBlindfold},
    { 2, 1, 1, 'i', kObjTargeterGoggles},
    { 2, 1, 1, 'i', kObjNVGoggles},
    { 2, 1, 1, 'i', kObjXRayGoggles},
    { 2, 1, 1, 'i', kObjScouterGoggles},
    END_TOKEN
};

Table Boots[] =
{
    {5, 1, 1, 'i', kObjSturdyBoots},
    {3, 1, 1, 'i', kObjSpaceBoots},
    {3, 1, 1, 'i', kObjStormtrooperBoots},
    {2, 1, 1, 'i', kObjJumpBoots},
    {1, 1, 1, 'i', kObjZealotLegEnhancements},
    END_TOKEN
};

Table Cloak[] =
{
    {10, 1, 1, 'i', kObjBadassTrenchcoat},
    {10, 1, 1, 'i', kObjLoserAnorak},
    { 5, 1, 1, 'i', kObjTransparentJacket},
    { 3, 1, 1, 'i', kObjAdamantineCloak},
    { 2, 1, 1, 'i', kObjSuperheroCape},
    END_TOKEN
};

Table Helmet[] =
{
    {5, 1, 1, 'i', kObjKevlarHelmet},
    {5, 1, 1, 'i', kObjPrussianHelmet},
    {5, 1, 1, 'i', kObjStormtrooperHelmet},
    {5, 1, 1, 'i', kObjElvenCH},
    {3, 1, 1, 'i', kObjSpaceHelmet},
    {3, 1, 1, 'i', kObjFootballHelmet},
    {2, 1, 1, 'i', kObjEnergyDome},
    {2, 1, 1, 'i', kObjHarmonyDome},
    {2, 1, 1, 'i', kObjBrainShield},
    {2, 1, 1, 'i', kObjElvenEH},
    {1, 1, 1, 'i', kObjBioMask},
    END_TOKEN
};

Table BodyArmor[] =
{
    {5, 1, 1, 'i', kObjReflecSuit},
    {3, 1, 1, 'i', kObjKevlarJacket},
    {3, 1, 1, 'i', kObjFlakJacket},
    {3, 1, 1, 'i', kObjAsbestosJacket},
    {3, 1, 1, 'i', kObjTeslaSuit},
    {2, 1, 1, 'i', kObjPlateArmor},
    {2, 1, 1, 'i', kObjFootballPads},
    {2, 1, 1, 'i', kObjStormtrooperSuit},
    {2, 1, 1, 'i', kObjSpaceSuit},
    {2, 1, 1, 'i', kObjElvenSpaceSuit},
    {2, 1, 1, 'i', kObjElvenCA},
    {2, 1, 1, 'i', kObjElvenEA},
    END_TOKEN
};

Table BioArmor[] =
{
    {70, 1, 1, 'i', kObjBioArmor},
    { 9, 1, 1, 'i', kObjMBioArmor0},
    { 5, 1, 1, 'i', kObjEBioArmor1},
    { 5, 1, 1, 'i', kObjEBioArmor2},
    { 5, 1, 1, 'i', kObjEBioArmor3},
    { 1, 1, 1, 'i', kObjSBioArmor1},
    { 1, 1, 1, 'i', kObjSBioArmor2},
    { 1, 1, 1, 'i', kObjSBioArmor3},
    { 1, 1, 1, 'i', kObjSBioArmor4},
    { 1, 1, 1, 'i', kObjMBioArmor1},
    { 1, 1, 1, 'i', kObjMBioArmor2},
    END_TOKEN
};

Table ChaosArmor[] =
{
    {1, 1, 1, 'i', kObjRedCPA},
    {1, 1, 1, 'i', kObjPinkCPA},
    {1, 1, 1, 'i', kObjBlueCPA},
    {1, 1, 1, 'i', kObjGreenCPA},
    END_TOKEN
};

Table PowerArmor[] =
{
    {1, 1, 1, 'i', kObjPowerSuit},
    {1, 1, 1, 'i', kObjAquamarinePA},
    {1, 1, 1, 'i', kObjMeanGreenPA},
    {1, 1, 1, 'i', kObjBrotherhoodPA},
    END_TOKEN
};

Table PowerHelmet[] =
{
    {1, 1, 1, 'i', kObjAquamarinePH},
    {1, 1, 1, 'i', kObjMeanGreenPH},
    {1, 1, 1, 'i', kObjBrotherhoodPH},
    END_TOKEN
};

Table Armor[] =
{
    {25, 1, 1, 's', (uintptr_t)&BodyArmor},
    {18, 1, 1, 's', (uintptr_t)&Jumpsuit},
    {12, 1, 1, 's', (uintptr_t)&Helmet},
    { 9, 1, 1, 's', (uintptr_t)&Goggles},
    { 9, 1, 1, 's', (uintptr_t)&Belt},
    { 6, 1, 1, 's', (uintptr_t)&Boots},
    { 6, 1, 1, 's', (uintptr_t)&Cloak},
    { 5, 1, 1, 's', (uintptr_t)&BioArmor},
    { 4, 1, 1, 's', (uintptr_t)&PowerArmor},
    { 4, 1, 1, 's', (uintptr_t)&PowerHelmet},
    { 2, 1, 1, 's', (uintptr_t)&ChaosArmor},
    END_TOKEN
};


Table HeavyConvGun[] =
{
    {2, 1, 1, 'i', kObjFlamer},
    {2, 1, 1, 'i', kObjRailgun},
    {2, 1, 1, 'i', kObjAutoCannon},
    {2, 1, 1, 'i', kObjPulseRifle},
    {1, 1, 1, 'i', kObjM56Smartgun},
    {1, 1, 1, 'i', kObjM57Smartgun},
    END_TOKEN
};

Table HeavyEnergyGun[] =
{
    {1, 1, 1, 'i', kObjLaserCannon},
    {1, 1, 1, 'i', kObjPlasmaCannon},
    {1, 1, 1, 'i', kObjMiningLaser},
    END_TOKEN
};

Table LightConvGun[] =
{
    {3, 1, 1, 'i', kObjShotgun},
    {3, 1, 1, 'i', kObjSawnoffShotgun},
    {2, 1, 1, 'i', kObjSniperRifle},
    {2, 1, 1, 'i', kObjLightFlamer},
    {1, 1, 1, 'i', kObjSpeargun},
    {1, 1, 1, 'i', kObjCryolator},
    END_TOKEN
};

Table LightEnergyGun[] =
{
    {3, 1, 1, 'i', kObjLaserRifle},
    {1, 1, 1, 'i', kObjBlasterRifle},
    {1, 1, 1, 'i', kObjPlasmaRifle},
    END_TOKEN
};

Table HandConvGun[] =
{
    {2, 1, 1, 'i', kObjPeaShooter},
    {3, 1, 1, 'i', kObjConvPistol},
    {3, 1, 1, 'i', kObjAssaultPistol},
    {2, 1, 1, 'i', kObjKhanHeavyPistol},
    {1, 1, 1, 'i', kObjNetLauncher},
    END_TOKEN
};

Table HandEnergyGun[] =
{
    {3, 1, 1, 'i', kObjZapGun},
    {3, 1, 1, 'i', kObjLaserPistol},
    {2, 1, 1, 'i', kObjBlasterPistol},
    {2, 1, 1, 'i', kObjPlasmaPistol},
    {2, 1, 1, 'i', kObjPhaserPistol},
    {1, 1, 1, 'i', kObjCombatTranslocator},
    {1, 1, 1, 'i', kObjPlasmaCaster},
    END_TOKEN
};

Table EnergyGun[] =
{
    {2, 1, 1, 's', (uintptr_t)&HeavyEnergyGun},
    {3, 1, 1, 's', (uintptr_t)&LightEnergyGun},
    {4, 1, 1, 's', (uintptr_t)&HandEnergyGun},
    END_TOKEN
};

Table ConvGun[] =
{
    {2, 1, 1, 's', (uintptr_t)&HeavyConvGun},
    {3, 1, 1, 's', (uintptr_t)&LightConvGun},
    {4, 1, 1, 's', (uintptr_t)&HandConvGun},
    END_TOKEN
};

Table Cannon[] =
{
    {1, 1, 1, 'i', kObjAutoCannon},
    {1, 1, 1, 'i', kObjLaserCannon},
    {1, 1, 1, 'i', kObjPlasmaCannon},
    END_TOKEN
};

Table LightRifle[] =
{
    {1, 1, 1, 'i', kObjLaserRifle},
    {1, 1, 1, 'i', kObjBlasterRifle},
    {1, 1, 1, 'i', kObjPlasmaRifle},
    {1, 1, 1, 'i', kObjSniperRifle},
    END_TOKEN
};

Table HeavyRifle[] =
{
    {1, 1, 1, 'i', kObjPulseRifle},
    {1, 1, 1, 'i', kObjM56Smartgun},
    {1, 1, 1, 'i', kObjM57Smartgun},
    END_TOKEN
};

Table Gun[] =
{
    {10, 1, 1, 's', (uintptr_t)&HeavyConvGun},
    {10, 1, 1, 's', (uintptr_t)&HeavyEnergyGun},
    {25, 1, 1, 's', (uintptr_t)&LightConvGun},
    {25, 1, 1, 's', (uintptr_t)&LightEnergyGun},
    {15, 1, 1, 's', (uintptr_t)&HandConvGun},
    {15, 1, 1, 's', (uintptr_t)&HandEnergyGun},
    END_TOKEN
};

Table CommonMeleeWeapon[] =
{
    {1, 1, 1, 'i', kObjBoStaff},
    {1, 1, 1, 'i', kObjCopperStick},
    {1, 1, 1, 'i', kObjNunchaku},
    {1, 2, 6, 'i', kObjShuriken},
    {1, 1, 3, 'i', kObjFootball},
    {1, 1, 2, 'i', kObjElvenDagger},
    {1, 1, 1, 'i', kObjCombatKnife},
    {1, 1, 1, 'i', kObjAnalProbe},
    {1, 1, 1, 'i', kObjClub},
    {1, 1, 1, 'i', kObjStunRod},
    {1, 1, 1, 'i', kObjZapBaton},
    {1, 1, 1, 'i', kObjMop},
    {1, 1, 1, 'i', kObjChainsaw},
    {1, 1, 1, 'i', kObjPowerClaw},
    {1, 1, 1, 'i', kObjBrassKnuckles},
    {1, 1, 1, 'i', kObjSpikedKnuckles},
    END_TOKEN
};

Table RareMeleeWeapon[] =
{
    {2, 1, 1, 'i', kObjPowerFist},
    {2, 1, 1, 'i', kObjRazorWhip},
    {2, 1, 1, 'i', kObjCombiStick},
    {2, 1, 1, 'i', kObjWristBlade},
    {2, 1, 1, 'i', kObjGomJabbar},
    {1, 1, 1, 'i', kObjPhaseGloves},
    {1, 1, 1, 'i', kObjPsiBlades},
    {1, 1, 1, 'i', kObjWarpScythe},
    {1, 1, 1, 'i', kObjSmartDisc},
    END_TOKEN
};

Table MeleeWeapon[] =
{
    {90, 1, 1, 's', (uintptr_t)&CommonMeleeWeapon},
    {10, 1, 1, 's', (uintptr_t)&RareMeleeWeapon},
    END_TOKEN
};

Table Sword[] =
{
    {6, 1, 1, 'i', kObjKatana},
    {5, 1, 1, 'i', kObjBatleth},
    {4, 1, 1, 'i', kObjElvenSword},
    {3, 1, 1, 'i', kObjBloodSword},
    {2, 1, 1, 'i', kObjChaosSword},
    {1, 1, 1, 'i', kObjLightSaber},
    END_TOKEN
};

Table Weapon[] =
{
    {35, 1, 1, 's', (uintptr_t)&Gun},
    {20, 1, 1, 's', (uintptr_t)&Ammunition},
    {15, 1, 1, 's', (uintptr_t)&Grenade},
    {21, 1, 1, 's', (uintptr_t)&MeleeWeapon},
    { 9, 1, 1, 's', (uintptr_t)&Sword},
    END_TOKEN
};

Table NotMoney[] =
{
    {18,  1,  1, 's', (uintptr_t)&FloppyDisk},
    {15,  1,  1, 's', (uintptr_t)&Weapon},
    {15,  1,  1, 's', (uintptr_t)&Canister},
    {12,  1,  1, 's', (uintptr_t)&Tool},
    {10,  1,  1, 's', (uintptr_t)&Armor},
    { 5,  1,  1, 's', (uintptr_t)&Implant},
    { 3, 20, 60, 'i', kObjEnergyCell},
    { 2,  1,  1, 's', (uintptr_t)&RayGun},
    END_TOKEN
};

Table RandomObject[] =
{
    {20,  1,  1, 's', (uintptr_t)&Money},
    {80,  1,  1, 's', (uintptr_t)&NotMoney},
    END_TOKEN
};

}
