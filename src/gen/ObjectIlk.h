#ifndef OBJECTILK_H
#define OBJECTILK_H
enum shObjId
{
kObjNothing,
kObjKevlarHelmet,
kObjPrussianHelmet,
kObjStormtrooperHelmet,
kObjSpaceHelmet,
kObjFootballHelmet,
kObjEnergyDome,
kObjHarmonyDome,
kObjBrainShield,
kObjAquamarinePH,
kObjMeanGreenPH,
kObjBrotherhoodPH,
kObjElvenCH,
kObjElvenEH,
kObjBioMask,
kObjSunglasses,
kObjPerilGlasses,
kObjBlindfold,
kObjTargeterGoggles,
kObjXRayGoggles,
kObjNVGoggles,
kObjScouterGoggles,
kObjShieldBelt,
kObjArmoredBelt,
kObjEnergyBelt,
kObjFashionBelt,
kObjCloakingBelt,
kObjStabilizerBelt,
kObjSuspensorBelt,
kObjAdamantineCloak,
kObjBadassTrenchcoat,
kObjLoserAnorak,
kObjSuperheroCape,
kObjTransparentJacket,
kObjSturdyBoots,
kObjJumpBoots,
kObjSpaceBoots,
kObjStormtrooperBoots,
kObjZealotLegEnhancements,
kObjOrdinaryJumpsuit,
kObjJanitorUniform,
kObjRadiationSuit,
kObjChameleonSuit,
kObjAsbestosJumpsuit,
kObjElvenJumpsuit,
kObjReticulanJumpsuit,
kObjReflecSuit,
kObjTeslaSuit,
kObjFootballPads,
kObjPlateArmor,
kObjFlakJacket,
kObjKevlarJacket,
kObjAsbestosJacket,
kObjStormtrooperSuit,
kObjAquamarinePA,
kObjMeanGreenPA,
kObjDeepBluePA,
kObjSpaceSuit,
kObjElvenSpaceSuit,
kObjElvenCA,
kObjElvenEA,
kObjPowerSuit,
kObjBrotherhoodPA,
kObjBioArmor,
kObjMBioArmor0,
kObjMBioArmor1,
kObjMBioArmor2,
kObjEBioArmor1,
kObjEBioArmor2,
kObjEBioArmor3,
kObjSBioArmor1,
kObjSBioArmor2,
kObjSBioArmor3,
kObjSBioArmor4,
kObjRedCPA,
kObjPinkCPA,
kObjBlueCPA,
kObjGreenCPA,
kObjShuriken,
kObjSmartDisc,
kObjFootball,
kObjElvenDagger,
kObjCombatKnife,
kObjAnalProbe,
kObjClub,
kObjStunRod,
kObjZapBaton,
kObjMop,
kObjNunchaku,
kObjRazorWhip,
kObjChainsaw,
kObjBoStaff,
kObjCopperStick,
kObjCombiStick,
kObjWarpScythe,
kObjBloodSword,
kObjKatana,
kObjElvenSword,
kObjBatleth,
kObjChaosSword,
kObjLightSaber,
kObjBrassKnuckles,
kObjSpikedKnuckles,
kObjWristBlade,
kObjPowerFist,
kObjPowerClaw,
kObjPhaseGloves,
kObjPsiBlades,
kObjGomJabbar,
kObjConcussionGrenade,
kObjFragGrenade,
kObjStunGrenade,
kObjFlashbang,
kObjIncendiaryGrenade,
kObjRadGrenade,
kObjEMPGrenade,
kObjWebbingGrenade,
kObjDispersalGrenade,
kObjFlare,
kObjBullet,
kObjShotgunShell,
kObjRailgunSlug,
kObjFlamerFuel,
kObjWebCasket,
kObjSpearheadDart,
kObjPeaShooter,
kObjConvPistol,
kObjAssaultPistol,
kObjKhanHeavyPistol,
kObjNetLauncher,
kObjShotgun,
kObjSawnoffShotgun,
kObjSniperRifle,
kObjLightFlamer,
kObjSpeargun,
kObjCryolator,
kObjPulseRifle,
kObjM56Smartgun,
kObjM57Smartgun,
kObjRailgun,
kObjAutoCannon,
kObjFlamer,
kObjLaserPistol,
kObjBlasterPistol,
kObjPhaserPistol,
kObjZapGun,
kObjPlasmaPistol,
kObjPlasmaCaster,
kObjCombatTranslocator,
kObjLaserRifle,
kObjBlasterRifle,
kObjPlasmaRifle,
kObjMiningLaser,
kObjLaserCannon,
kObjPlasmaCannon,
kObjSmallEnergyTank,
kObjEnergyTank=kObjSmallEnergyTank,
kObjLargeEnergyTank,
kObjFissionPlant,
kObjFusionPlant,
kObjDuctTape,
kObjMonkeyWrench,
kObjFlashlight,
kObjGeigerCounter,
kObjMotionTracker,
kObjDroidCaller,
kObjTricorder,
kObjRemoteControl,
kObjKeycard1,
kObjKeycard2,
kObjKeycard3,
kObjKeycard4,
kObjMasterKeycard,
kObjKeycard5=kObjMasterKeycard,
kObjLockPick,
kObjRestrainingBolt,
kObjMedicomp,
kObjMiniComputer,
kObjMegaComputer,
kObjBioComputer,
kObjSatCom,
kObjComputerOS=kObjSatCom,
kObjPortableHole,
kObjProximityMine,
kObjKhaydarin,
kObjRabbitFoot,
kObjBluePill,
kObjRedPill,
kObjTransferCable,
kObjJunk,
kObjMoney,
kObjEnergyCell,
kObjWreck,
kObjHealthMonitor,
kObjRadiationProcessor,
kObjPoisonResistor,
kObjBabelFish,
kObjCerebralCoprocessor,
kObjMotoricCoordinator,
kObjReflexCoordinator,
kObjAdrenalineGenerator,
kObjMyfilamentHypertensor,
kObjPsiAmp,
kObjTissueRegenerator,
kObjNarcoleptor,
kObjGloryDevice,
kObjExcruciator,
kObjShockCapacitor,
kObjMechaDendrites,
kObjTorc,
kObjSearchSkillsoft,
kObjRepairSkillsoft,
kObjSpotSkillsoft,
kObjLockPickSkillsoft,
kObjWarpStabilizer,
kObjUngooma,
kObjBOFHEye,
kObjEmptyRayGun,
kObjRayGunPreamble=kObjEmptyRayGun,
kObjUnknownRayGun=kObjEmptyRayGun,
kObjFreezeRayGun,
kObjHeatRayGun,
kObjGaussRayGun,
kObjPoisonRayGun,
kObjGammaRayGun,
kObjStasisRayGun,
kObjTransporterRayGun,
kObjHealingRayGun,
kObjRestorationRayGun,
kObjDisintegrationRayGun,
kObjAccelerationRayGun,
kObjDecelerationRayGun,
kObjDecontaminationRayGun,
kObjAugmentationRayGun,
kObjSquirtRayGun,
kObjNanoCola,
kObjNukaCola,
kObjB3,
kObjBeer,
kObjSuperGlue,
kObjCoffee,
kObjWater,
kObjRadAway,
kObjRestorationCanister,
kObjHealingCanister,
kObjLNO,
kObjNapalm,
kObjUniversalSolvent,
kObjSpeedCanister,
kObjPoisonCanister,
kObjPlasmaCanister,
kObjBuffout,
kObjMutagenCanister,
kObjFullHealingCanister,
kObjGainAbilityCanister,
kObjSpiceMelange,
kObjAntimatter,
kObjBrain,
kObjCannedEmbryo,
kObjSithGravy,
kObjCorruptDisk,
kObjBugDetectionDisk,
kObjIdentifyDisk,
kObjSpamDisk,
kObjBlankDisk,
kObjObjectDetectionDisk,
kObjLifeformDetectionDisk,
kObjMappingDisk,
kObjDiagnosticsDisk,
kObjWarpTracerDisk,
kObjLogicBombDisk,
kObjDebuggerDisk,
kObjEnhanceArmorDisk,
kObjEnhanceWeaponDisk,
kObjEnhanceImplantDisk,
kObjHypnosisDisk,
kObjTransportDisk,
kObjRecursiveLoopDisk,
kObjAntivirusDisk,
kObjReformatDisk,
kObjComputerVirus,
kObjRecallDisk,
kObjOperatingSystem,
kObjHackingDisk,
kObjResidentAntivirusDisk,
kObjMatterCompiler,
kObjTheOrgasmatron,
kObjFakeOrgasmatron1,
kObjFakeOrgasmatron2,
kObjFakeOrgasmatron3,
kObjFakeOrgasmatron4,
kObjFakeOrgasmatron5,
kObjNumIlks
};
#endif