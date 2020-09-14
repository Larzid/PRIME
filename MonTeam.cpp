#include <stdlib.h>
#include "Distribution.h"

prob::Table Barrels[] =
{
    {55, 1, 1, 'm', kMonFuelBarrel},
    {40, 2, 3, 'm', kMonFuelBarrel},
    { 5, 4, 6, 'm', kMonFuelBarrel},
    END_TOKEN
};


prob::Table Lvl0[] =
{
    {4, 1, 1, 's', (uintptr_t)&Barrels},
    {6, 1, 1, 'm', kMonWarpFungus},
    {5, 2, 4, 'm', kMonGridBug},
    {3, 1, 4, 'm', kMonBoreWorm},
    {1, 3, 4, 'm', kMonTribble},
    END_TOKEN
};

prob::Table Lvl1[] =
{
    {6, 1, 3, 'm', kMonGiantCockroach},
    {3, 1, 4, 'm', kMonMynock},
    {3, 1, 1, 'm', kMonManEatingPlant},
    {3, 1, 4, 'm', kMonLittleGreyAlien},
    {2, 2, 4, 'm', kMonFlyingToaster},
    {1, 1, 1, 'm', kMonNakedOrz},
    END_TOKEN
};


prob::Table Lvl2[] =
{
    {6, 1, 4, 'm', kMonSpaceGoblin},
    {5, 1, 1, 'm', kMonMutantHuman},
    {3, 1, 1, 'm', kMonTapiocaPudding},
    {3, 1, 1, 'm', kMonSmartBomb},
    {2, 1, 1, 'm', kMonWebCrawler},
    {2, 1, 1, 'm', kMonRatbot},
    {1, 1, 3, 'm', kMonGreenTomato},
    END_TOKEN
};


prob::Table Lvl3[] =
{
    {6, 1, 1, 'm', kMonKamikazeGoblin},
    {6, 1, 3, 'm', kMonRedshirt},
    {4, 1, 4, 'm', kMonSpaceOrc},
    {4, 1, 3, 'm', kMonCylonCenturion},
    {2, 1, 1, 'm', kMonCatbot},
    {2, 1, 1, 'm', kMonDogbot},
    {2, 1, 3, 'm', kMonRedTomato},
    {2, 1, 1, 'm', kMonVatSlime},
    {1, 4,12, 'm', kMonAlienEgg},
    END_TOKEN
};


prob::Table Lvl4[] =
{
    {6, 3, 7, 'm', kMonStormtrooper},
    {4, 1, 2, 'm', kMonHighPingBastard},
    {4, 1, 4, 'm', kMonSpaceOrcPyromaniac},
    {2, 2, 6, 'm', kMonCrite},
    {2, 1, 1, 'm', kMonMetalSlime},
    {2, 1, 1, 'm', kMonCreepingCredits},
    {1, 1, 1, 'm', kMonScrubbot},
    {1, 1, 1, 'm', kMonAstromechDroid},
    {1, 1, 1, 'm', kMonProtocolDroid},
    {1, 1, 1, 'm', kMonOrzScubaDiver},
    END_TOKEN
};

prob::Table OrcFireTeam[] =
{
    {100, 1, 2, 'm', kMonSpaceOrcBoss},
    {100, 2, 4, 'm', kMonSpaceOrcPyromaniac},
    END_TOKEN
};

prob::Table OrcGruntTeam[] =
{
    {100, 1, 2, 'm', kMonSpaceOrcBoss},
    {100, 3, 6, 'm', kMonSpaceOrc},
    { 25, 1, 3, 'm', kMonSpaceOrc},
    END_TOKEN
};

prob::Table OrcMixedTeam[] =
{
    {100, 1, 2, 'm', kMonSpaceOrcBoss},
    {100, 2, 4, 'm', kMonSpaceOrc},
    {100, 1, 2, 'm', kMonSpaceOrcPyromaniac},
    END_TOKEN
};

prob::Table OrcBossTeam[] =
{
    {100, 2, 3, 'm', kMonSpaceOrcBoss},
    { 25, 1, 1, 'm', kMonSpaceOrcBoss},
    END_TOKEN
};

prob::Table RandomOrcTeam[] =
{
    {1, 1, 1, 'M', (uintptr_t)&OrcFireTeam},
    {1, 1, 1, 'M', (uintptr_t)&OrcGruntTeam},
    {1, 1, 1, 'M', (uintptr_t)&OrcMixedTeam},
    {1, 1, 1, 'M', (uintptr_t)&OrcBossTeam},
    END_TOKEN
};

prob::Table CylonBlueTeam[] =
{
    {100, 1, 1, 'm', kMonCylonCommandCenturion},
    {100, 2, 2, 'm', kMonCylonCenturion},
    END_TOKEN
};

prob::Table RandomCylonTeam[] =
{
    {5, 1, 1, 'M', (uintptr_t)&CylonBlueTeam},
    {4, 2, 2, 'm', kMonCylonCommandCenturion},
    {1, 1, 1, 'm', kMonCylonCommandCenturion},
    END_TOKEN
};

prob::Table Lvl5[] =
{
    {6, 1, 3, 'm', kMonKlingon},
    {3, 1, 2, 'm', kMonLizard1},
    {3, 1, 3, 'm', kMonRadbug},
    {3, 1, 1, 's', (uintptr_t)&RandomOrcTeam},
    {2, 1, 1, 's', (uintptr_t)&RandomCylonTeam},
    {2, 2, 4, 'm', kMonTroubleshooter},
    {2, 2, 4, 'm', kMonBorg},
    {1, 1, 4, 'm', kMonCheerleaderNinja},
    {1, 1, 1, 'm', kMonFacehugger},
    {1, 1, 1, 'm', kMonChestburster},
    END_TOKEN
};

prob::Table ZerglingDuo[] =
{
    {100, 2, 2, 'm', kMonZergling},
    END_TOKEN
};

prob::Table KlingonTeam[] =
{
    {100, 1, 1, 'm', kMonKlingonCaptain},
    {100, 2, 2, 'm', kMonKlingon},
    END_TOKEN
};

prob::Table StormtrooperTeam[] =
{
    {6, 1, 1, 'm', kMonStormtrooperCommander},
    {6, 2, 4, 'm', kMonStormtrooper},
    END_TOKEN
};

prob::Table Lvl6[] =
{
    {6, 1, 1, 'm', kMonBrainMold},
    {5, 1, 3, 'M', (uintptr_t)&ZerglingDuo},
    {4, 1, 1, 'm', kMonRadspider},
    {4, 1, 1, 'M', (uintptr_t)&StormtrooperTeam},
    {3, 1, 1, 'M', (uintptr_t)&KlingonTeam},
    {3, 1, 3, 'm', kMonPinkHorror},
    {2, 1, 1, 'm', kMonBeneGesserit},
    {1, 1, 1, 'm', kMonAlienPrincess},
    {1, 1, 2, 'm', kMonRecognizer},
    {1, 1, 2, 'm', kMonMiGo},
    END_TOKEN
};

prob::Table KlingonCmdr4[] =
{
    {100, 1, 1, 'm', kMonKlingonCommander},
    {100, 4, 4, 'm', kMonKlingon},
    END_TOKEN
};

prob::Table KlingonCmdrCapt2[] =
{
    {2, 1, 1, 'm', kMonKlingonCommander},
    {3, 1, 1, 'm', kMonKlingonCaptain},
    {6, 2, 2, 'm', kMonKlingon},
    END_TOKEN
};

prob::Table HighKlingonTeam[] =
{
    {1, 1, 1, 'M', (uintptr_t)&KlingonCmdr4},
    {1, 1, 1, 'M', (uintptr_t)&KlingonCmdrCapt2},
    END_TOKEN
};

prob::Table Lvl7[] =
{
    {5, 1, 1, 'm', kMonHydralisk},
    {5, 1, 5, 'm', kMonSpaceElf},
    {4, 1, 2, 'm', kMonLowPingBastard},
    {4, 1, 3, 'm', kMonSewerWorm},
    {3, 1, 1, 'm', kMonSmartMissile},
    {3, 1, 2, 'm', kMonLizard2},
    {2, 1, 1, 's', (uintptr_t)&HighKlingonTeam},
    {1, 1, 1, 'm', kMonUsenetTroll},
    {3, 1, 1, 'm', kMonSorceress},
    END_TOKEN
};

prob::Table Lvl8[] =
{
    {6, 1, 4, 'm', kMonAlienWarrior},
    {4, 1, 3, 'm', kMonRadscorpion},
    {4, 2, 4, 'm', kMonBrotherhoodPaladin},
    {3, 1, 4, 'm', kMonTallGreyAlien},
    {2, 1, 4, 'm', kMonMutantNinjaTurtle},
    {2, 1, 3, 'm', kMonCyborgNinja},
    END_TOKEN
};

prob::Table ElfTeam[] =
{
    {100, 1, 1, 'm', kMonSpaceElfLord},
    {100, 1, 3, 'm', kMonSpaceElf},
    END_TOKEN
};

prob::Table Lvl9[] =
{
    {3, 1, 1, 's', (uintptr_t)&ElfTeam},
    {2, 3, 3, 'm', kMonSpiderMine},
    {1, 1, 1, 'm', kMonLawyer},
    END_TOKEN
};

prob::Table Lvl10[] =
{
    {4, 2, 5, 'm', kMonAMarine},
    {4, 2, 5, 'm', kMonMGMarine},
    END_TOKEN
};

prob::Table HighElfTeam[] =
{
    {100, 1, 1, 'm', kMonSpaceElfQueen},
    {100, 1, 1, 'm', kMonSpaceElfLord},
    {100, 1, 3, 'm', kMonSpaceElf},
    END_TOKEN
};

prob::Table Lvl11[] =
{
    {3, 1, 3, 'm', kMonReticulanDissident},
    {2, 1, 1, 's', (uintptr_t)&HighElfTeam},
    {1, 1, 1, 'm', kMonFiftyFootWoman},
    END_TOKEN
};

prob::Table Lvl12[] =
{
    {4, 2, 5, 'm', kMonCCMarine},
    {4, 1, 1, 'm', kMonRocketTurret},
    {2, 1, 3, 'm', kMonZealot},
    {3, 1, 1, 'm', kMonDefiler},
    {1, 1, 1, 'm', kMonHighTemplar},
    END_TOKEN
};

prob::Table Lvl13[] =
{
    {3, 1, 2, 'm', kMonLizard3},
    {2, 1, 1, 'm', kMonDarkTemplar},
    {2, 1, 3, 'm', kMonDalek},
    {1, 1, 1, 'm', kMonAlienQueen},
    END_TOKEN
};

prob::Table Lvl14[] =
{
    {2, 1, 1, 'm', kMonGiantApe},
    {1, 1, 1, 'm', kMonGoGo},
    {1, 1, 1, 'm', kMonWarbot},
    {1, 1, 1, 'm', kMonSecuritron},
    END_TOKEN
};

prob::Table Lvl15[] =
{
    {1, 1, 1, 'm', kMonKohrAh},
    END_TOKEN
};

prob::Table Lvl16[] =
{
    {1, 1, 1, 'm', kMonMailDaemon},
    {1, 1, 1, 'm', kMonBindDaemon},
    END_TOKEN
};

prob::Table Lvl17[] =
{
    {1, 1, 1, 'm', kMonNNTPDaemon},
    {1, 1, 1, 'm', kMonFTPDaemon},
    END_TOKEN
};

prob::Table *prob::MonByLevel[MAX_MON_LEV] =
{
    Lvl0,
    Lvl1,
    Lvl2,
    Lvl3,
    Lvl4,
    Lvl5,
    Lvl6,
    Lvl7,
    Lvl8,
    Lvl9,
    Lvl10,
    Lvl11,
    Lvl12,
    Lvl13,
    Lvl14,
    Lvl15,
    Lvl16,
    Lvl17,
    NULL,
    NULL
};

prob::Table TownCitizen[] =
{
    {4, 1, 1, 'm', kMonCylonCenturion},
    {3, 1, 1, 'm', kMonAstromechDroid},
    {2, 1, 1, 'm', kMonScrubbot},
    {2, 1, 1, 'm', kMonCatbot},
    {2, 1, 1, 'm', kMonDogbot},
    {1, 1, 1, 'm', kMonProtocolDroid},
    {1, 1, 1, 'm', kMonCylonCommandCenturion},
    {1, 1, 1, 'm', kMonRatbot},
    {1, 1, 1, 'm', kMonSpiderMine},
    {1, 1, 1, 'm', kMonSmartMissile},
    END_TOKEN
};

prob::Table UnixDaemon[] =
{
    {1, 1, 1, 'm', kMonNNTPDaemon},
    {1, 1, 1, 'm', kMonFTPDaemon},
    {1, 1, 1, 'm', kMonMailDaemon},
    {1, 1, 1, 'm', kMonBindDaemon},
    END_TOKEN
};

prob::Table WeakProgram[] =
{
    {1, 4, 6, 'm', kMonGridBug},
    {1, 2, 5, 'm', kMonFlyingToaster},
    {1, 2, 3, 'm', kMonWebCrawler},
    END_TOKEN
};

prob::Table Program[] =
{
    {50, 1, 1, 's', (uintptr_t)&WeakProgram},
    {25, 1, 1, 's', (uintptr_t)&UnixDaemon},
    {25, 1, 1, 'm', kMonRecognizer},
    END_TOKEN
};

prob::Table Zerg[] =
{
    {11, 1, 3, 'm', kMonHydralisk},
    { 7, 2, 3, 'M', (uintptr_t)&ZerglingDuo},
    { 2, 1, 1, 'm', kMonDefiler},
    END_TOKEN
};

prob::Table Alien[] =
{
    {30, 1, 2, 'm', kMonAlienWarrior},
    {25, 1, 1, 'm', kMonChestburster},
    {25, 1, 1, 'm', kMonAlienPrincess},
    {20, 1, 3, 'm', kMonFacehugger},
    END_TOKEN
};

prob::Table CaveMonster[] =
{
    {8, 1, 1, 's', (uintptr_t)&Alien},
    {6, 1, 1, 's', (uintptr_t)&Zerg},
    {6, 1, 1, 'm', kMonBrainMold},
    {4, 1, 3, 'm', kMonAlienEgg},
    {3, 1, 1, 'm', kMonAlienQueen},
    {3, 1, 1, 'm', kMonFiftyFootWoman},
    {3, 1, 1, 'm', kMonGiantApe},
    {3, 1, 1, 'm', kMonRadbug},
    {3, 1, 1, 'm', kMonRadspider},
    {3, 1, 1, 'm', kMonRadscorpion},
    {2, 1, 1, 'm', kMonRocketTurret},
    {2, 1, 1, 'm', kMonSecuritron},
    {2, 1, 1, 'm', kMonWarbot},
    {2, 1, 1, 'm', kMonMiGo},
    {2, 1, 1, 'm', kMonPinkHorror},
    {1, 1, 4, 'm', kMonLizard1},
    {1, 1, 3, 'm', kMonLizard2},
    {1, 1, 2, 'm', kMonLizard3},
    {1, 2, 3, 'm', kMonBrotherhoodPaladin},
    {1, 1, 1, 'm', kMonManEatingPlant},
    {1, 1, 1, 'm', kMonWarpFungus},
    END_TOKEN
};

prob::Table OozeGroup[] =
{
    {4, 1, 1, 'm', kMonWarpFungus},
    {3, 1, 3, 'm', kMonTapiocaPudding},
    {3, 1, 3, 'm', kMonVatSlime},
    {2, 1, 3, 'm', kMonMetalSlime},
    END_TOKEN
};

prob::Table SewerMonster[] =
{
    {4, 1, 1, 's', (uintptr_t)&Zerg},
    {3, 1, 3, 'm', kMonSewerWorm},
    {3, 1, 1, 'm', kMonBrainMold},
    {3, 1, 4, 'm', kMonMutantNinjaTurtle},
    {2, 1, 2, 'm', kMonMiGo},
    {2, 1, 1, 'm', kMonLizard1},
    {2, 1, 1, 'm', kMonLizard2},
    {2, 1, 1, 'm', kMonLizard3},
    {2, 1, 1, 's', (uintptr_t)&OozeGroup},
    {1, 1, 1, 'm', kMonFiftyFootWoman},
    {1, 1, 1, 'm', kMonGiantApe},
    {1, 1, 4, 'm', kMonCrite},
    {1, 1, 3, 'm', kMonRadbug},
    {1, 1, 3, 'm', kMonRadspider},
    {1, 1, 2, 'm', kMonRadscorpion},
    {1, 1, 1, 'm', kMonVatSlime},
    {1, 1, 6, 'm', kMonGiantCockroach},
    {1, 1, 1, 'm', kMonRatbot},
    END_TOKEN
};

prob::Table Shodan[] =
{
    {100, 1, 1, 'm', kMonShodan},
    { 90, 1, 1, 'm', kMonMailDaemon},
    { 90, 1, 1, 'm', kMonBindDaemon},
    { 90, 1, 1, 'm', kMonNNTPDaemon},
    { 90, 1, 1, 'm', kMonFTPDaemon},
    END_TOKEN
};

namespace prob {

Table RobotTown =
    {0, 1, 1, 's', (uintptr_t)&TownCitizen};
Table Mainframe =
    {0, 1, 1, 's', (uintptr_t)&Program};
Table Sewers =
    {0, 1, 1, 's', (uintptr_t)&SewerMonster};
Table GammaCaves =
    {0, 1, 1, 's', (uintptr_t)&CaveMonster};
Table ShodanGroup =
    {0, 1, 1, 'M', (uintptr_t)&Shodan};
}
