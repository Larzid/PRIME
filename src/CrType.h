#ifndef CRTYPE_H
#define CRTYPE_H

enum shCreatureType {
    /* non-living: */
    kBot,
    kDroid,
    kProgram,
    kConstruct,
    /* living: */
    kEgg,
    kOoze,
    /* living, and has mind: */
    kCyborg,
    kAberration,
    kAnimal,
    kAlien,
    kBeast,
    kHumanoid,
    kMutant,      /* mutant humanoid */
    kInsect,
    kOutsider,
    kVermin,
    kZerg,
    kMaxCreatureType
};
#define HAS_MIND(_crtype) (_crtype > kOoze)
#define IS_ROBOT(_crtype) (_crtype <= kDroid)
#define IS_ALIVE(_crtype) (_crtype > kConstruct)

enum shCreatureSymbols {
    kSymBot = 'x',
    kSymCritter = 'c',
    kSymDroid = 'X',
    kSymGray = 'g',
    kSymHero = '@',
    kSymHumanoid = 'h',
    kSymInsect = 'i',
    kSymSlime = 's',
    kSymWorm = 'w',
    kSymZerg = 'Z'
};

#endif
