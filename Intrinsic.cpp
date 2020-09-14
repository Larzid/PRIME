#include "Intrinsic.h"
#include <string.h>
#include <assert.h>

const char *intrinsicName[kNumParamI] =
{
    "acidic blood",
    "air supply",
    "regeneration",
    "automatic searching",
    "blindness",
    "brain shield",
    "breathlessness",
    "bug sensing",
    "camouflage",
    "swimming",
    "electro-magnetic field vision",
    "flight",
    "health monitoring",
    "invisibility",
    "springy legs",
    "fertility",
    "narcolepsy",
    "night vision",
    "sense of peril",
    "scariness",
    "Holtzmann shield",
    "language translation",

    "light",
    "motion detection",
    "radiation processing",
    "reflection",
    "acute smell sense",
    "life sense",
    "telepathy",
    "tremorsense",
    "x-ray vision",
    "warp travel control"
};

bool
isIntrAdditive (shIntrinsic intr)
{
    switch (intr) {
    case kLightSource: case kScent:
        return false;
    default:
        return true;
    }
}

void
shIntrinsicData::clone (shIntrinsicData &base)
{
    memcpy (mInt, base.mInt, sizeof (mInt));
    memcpy (mBool, base.mBool, sizeof (mBool));
}

void
shIntrinsicData::clear (void)
{
    memset (mInt, 0, sizeof (mInt));
    memset (mBool, 0, sizeof (mBool));
}

int
shIntrinsicData::get (shIntrinsic intr)
{
    if (intr < kNumBoolI)
        return mBool[intr];
    return mInt[intr - kNumBoolI];
}

void
shIntrinsicData::set (shIntrinsic intr, int val)
{
    if (intr < kNumBoolI) {
        mBool[intr] = !!val;
        return;
    }
    mInt[intr - kNumBoolI] = val;
}

void
shIntrinsicData::toggle (shIntrinsic intr)
{
    if (intr >= kNumBoolI) {
        assert (false);
        return;
    }
    mBool[intr] = !mBool[intr];
}

void
shIntrinsicData::mod (shIntrinsic intr, int val)
{
    if (intr < kNumBoolI) {
        mBool[intr] += val;
        return;
    }
    mInt[intr - kNumBoolI] += val;
}
