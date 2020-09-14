#include "Effect.h"
#include "AttackType.h"
#include "Global.h"
#include "Map.h"
#include "Hero.h"

namespace eff {

typedef type table[9];

/* 8-directional attacks */
table hydralisk =
    {hydra_n, hydra_ne, hydra_e, hydra_se, hydra_s,
     hydra_sw, hydra_w, hydra_nw, hydra};

table railgun =
    {rail_n, rail_ne, rail_e, rail_se, rail_s,
     rail_sw, rail_w, rail_nw, rail};

table psionic_vomit =
    {psi_vomit_n, psi_vomit_ne, psi_vomit_e, psi_vomit_se, psi_vomit_s,
     psi_vomit_sw, psi_vomit_w, psi_vomit_nw, psi_vomit};

table combi_head =
    {combi_n, combi_ne, combi_e, combi_se, combi_s,
     combi_sw, combi_w, combi_nw, none};

/* 4-directional attacks */
table laser_beam =
    {laser_vt, laser_fw, laser_hz, laser_bw, laser_vt,
     laser_fw, laser_hz, laser_bw, laser};

table optic_blast =
    {optic_vt, optic_fw, optic_hz, optic_bw, optic_vt,
     optic_fw, optic_hz, optic_bw, optic};

table particle_bolt =
    {bolt_vt, bolt_fw, bolt_hz, bolt_bw, bolt_vt,
     bolt_fw, bolt_hz, bolt_bw, bolt};

table binary_breath =
    {binary_vt, binary_fw, binary_hz, binary_bw, binary_vt,
     binary_fw, binary_hz, binary_bw, binary};

table combi_reach =
    {combi_vt, combi_fw, combi_hz, combi_bw, combi_vt,
     combi_fw, combi_hz, combi_bw, none};

table heat_ray =
    {heat_vt, heat_fw, heat_hz, heat_bw, heat_vt,
     heat_fw, heat_hz, heat_bw, heat};

table antimatter_ray =
    {antimatter_vt, antimatter_fw, antimatter_hz, antimatter_bw, antimatter_vt,
     antimatter_fw, antimatter_hz, antimatter_bw, antimatter};
}

eff::type
beam_effect (shAttack *attack, shDirection dir, bool head = true)
{
    /* Column number 9 contains glyph for directions up, down and origin. */
    if (dir == kOrigin or dir == kDown or dir == kNoDirection)
        dir = kUp;

    switch (attack->mType) {
    case shAttack::kSpit:              return eff::hydralisk[dir];
    case shAttack::kRail:              return eff::railgun[dir];
    case shAttack::kVomit:             return eff::psionic_vomit[dir];
    case shAttack::kLaserBeam:         return eff::laser_beam[dir];
    case shAttack::kOpticBlast:        return eff::optic_blast[dir];
    case shAttack::kBolt:              return eff::particle_bolt[dir];
    case shAttack::kBreatheTraffic:    return eff::binary_breath[dir];
    case shAttack::kHeatRay:           return eff::heat_ray[dir];
    case shAttack::kDisintegrationRay: return eff::antimatter_ray[dir];
    case shAttack::kFreezeRay:
    case shAttack::kCryolator:         return eff::cold;
    case shAttack::kPlague:            return eff::vomit;
    case shAttack::kPoisonRay:         return eff::poison;
    case shAttack::kLight:             return eff::blind;
    case shAttack::kIncendiary:        return eff::incendiary;
    case shAttack::kEnsnare:           return eff::web;
    case shAttack::kBreatheFire:
        return eff::type (int (eff::flame1) + (Clock / FULLTURN) % 8);
    case shAttack::kPsionicStorm:      return eff::psi_storm;
    case shAttack::kBreatheBugs:       return eff::bugs;
    case shAttack::kBreatheViruses:    return eff::viruses;
    case shAttack::kAcidSplash:        return eff::acid;
    case shAttack::kDisc:              return eff::smart_disc;
    case shAttack::kPea:               return eff::pea;
    case shAttack::kShot:              return eff::shrapnel;
    case shAttack::kCombi:  if (head)  return eff::combi_head[dir];
                            else       return eff::combi_reach[dir];
    case shAttack::kPlasmaGlob:
        if (dir == kOrigin)            return eff::plasma_hit;
        else                           return eff::plasma;
    case shAttack::kWaterRay:
        if (dir == kOrigin)            return eff::water;
        else                           return eff::none;
    case shAttack::kGammaRay:
        return Hero.cr ()->usesPower (kGammaSight)
                                            ? eff::rad
                                            : eff::none;
    case shAttack::kBlast:  /* Check primary damage. */
        switch (attack->mDamage[0].mEnergy) {
        case kBurning:                 return eff::incendiary;
        case kDisintegrating:          return eff::disintegration;
        case kFreezing:                return eff::cold;
        case kMagnetic:                return eff::none;
        case kRadiological:            return eff::rad;
        case kShrapnel:                return eff::shrapnel;
        //case kToxic:                   return eff::poison;
        case kWebbing:                 return eff::web;
        default:                       return eff::boom;
        }
    case shAttack::kExplode:
        if (attack->mDamage[0].mEnergy == kPsychic)
            return eff::psi_storm; /* Rossak sorceress. */
        return eff::boom;
    case shAttack::kFlash:
        switch (attack->mDamage[0].mEnergy) {
            case kBlinding:            return eff::blind;
            case kRadiological:        return eff::rad;
            default:                   return eff::boom;
        }
    case shAttack::kSpear:
        /* Speargun effct might look like railgun in
           ASCII, but when tiles get drawn it should get separate. */
    default:                           return eff::none;
    }
    return eff::none;
}

static bool
attack_has_corners (shAttack::Type type)
{
    if (type == shAttack::kBreatheTraffic or
        type == shAttack::kDisintegrationRay or
        type == shAttack::kHeatRay or
        type == shAttack::kLaserBeam or
        type == shAttack::kOpticBlast or
        type == shAttack::kVomit)
    {
        return true;
    }
    return false;
}

static int
attack_corner_offset (shAttack::Type type)
{
    switch (type) {
    case shAttack::kBreatheTraffic:
        return eff::binary_nwc - eff::laser_nwc;
    case shAttack::kDisintegrationRay:
        return eff::antimatter_nwc - eff::laser_nwc;
    case shAttack::kHeatRay:
        return eff::heat_nwc - eff::laser_nwc;
    case shAttack::kLaserBeam:
        return 0;
    case shAttack::kOpticBlast:
        return eff::optic_nwc - eff::laser_nwc;
    case shAttack::kVomit:
        return eff::psi_vomit_nwc - eff::laser_nwc;
    default: break;
    }
    return 0;
}

/* Returns true if any corners were drawn. */
bool
draw_corners (shMapLevel *l, shAttack *attack, shDirection dir, int x, int y)
{
    if (!isDiagonal (dir))
        return false;

    if (!attack_has_corners (attack->mType))
        return false;

    /* In NotEye mode this requires corners to be drawn. */
    int cr1x = x, cr1y = y, cr2x = x, cr2y = y;
    int cr1 = eff::none, cr2 = eff::none;
    switch (dir) {
    case kNorthWest:
        cr1y = y - 1; cr1 = eff::laser_swc;
        cr2x = x - 1; cr2 = eff::laser_nec;
        break;
    case kNorthEast:
        cr1y = y - 1; cr1 = eff::laser_sec;
        cr2x = x + 1; cr2 = eff::laser_nwc;
        break;
    case kSouthEast:
        cr1y = y + 1; cr1 = eff::laser_nec;
        cr2x = x + 1; cr2 = eff::laser_swc;
        break;
    case kSouthWest:
        cr1y = y + 1; cr1 = eff::laser_nwc;
        cr2x = x - 1; cr2 = eff::laser_sec;
        break;
    default: break; /* Eliminated by earlier guard clause. */
    }

    int offset = attack_corner_offset (attack->mType);
    cr1 += offset;
    cr2 += offset;

    if (cr1 and l->isFloor (cr1x, cr1y))
        l->setSpecialEffect (cr1x, cr1y, eff::type (cr1));
    if (cr2 and l->isFloor (cr2x, cr2y))
        l->setSpecialEffect (cr2x, cr2y, eff::type (cr2));

    return cr1 or cr2;
}
