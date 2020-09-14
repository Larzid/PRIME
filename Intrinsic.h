#ifndef INTRINSIC_H
#define INTRINSIC_H

enum shIntrinsic {
    /* Boolean intrinsics. */
    kAcidBlood,        /* Splash damage on death. */
    kAirSupply,        /* has air supply */
    kAutoRegeneration,
    kAutoSearching,
    kBlind,
    kBrainShielded,    /* Immunity to mental assaults. */
    kBreathless,       /* Does not need to breathe. */
    kCamouflaged,      /* Invisible next to obstacles. */
    kCanSwim,
    kEMFieldVision,    /* Like night vision but for aliens. */
    kFlying,
    kHealthMonitoring, /* Low hit points warning. */
    kInvisible,        /* Hidden by cloaking field. */
    kJumpy,            /* One-turn escape from pits. */
    kMultiplier,       /* Breeds explosively. */
    kNarcolepsy,       /* Periodical falling asleep. */
    kNightVision,
    kPerilSensing,
    kScary,            /* Attacking it may terrify. */
    kShielded,         /* Protected by a shield generator. */
    kTranslation,      /* Understand robots and aliens. */
    kNumBoolI,

    /* Parameterized. */
    kLightSource = kNumBoolI,
    kMotionDetection,
    kRadiationDetection, /* Number of active Geiger counters. */
    kRadiationProcessing,
    kReflection,       /* Some beams are reflected away. */
    kScent,            /* Sightless short range detection. */
    kSenseLife,        /* Xel'Naga and aliens' racial ability */
    kTelepathy,        /* Sense other minds. */
    kTremorsense,      /* Sensing of ground vibrations. */
    kXRayVision,       /* See through walls. */
    kWarpControl,      /* Chance to control transports. */
    kNumParamI,

    /* Dummy. */
    kNoIntrinsic = kNumParamI
};

struct shIntrinsicData
{
    void clone (shIntrinsicData &);
    void clear (void);
    int get (shIntrinsic);
    void set (shIntrinsic, int);
    void toggle (shIntrinsic);
    void mod (shIntrinsic, int);
 private:
    /* How many parameterized intrinsics there are. */
    static const int I_PARAM = kNumParamI - kNumBoolI + 1;

    int mInt[I_PARAM];
    int mBool[kNumBoolI];
};

extern const char *intrinsicName[kNumParamI];
#endif
