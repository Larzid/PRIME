#include "Scenario.h"
#include "Util.h"

scenario Scen;

void
generate_scenario (scenario &s)
{
    shObjId beverage_candidate[] =
    {
        kObjBeer,
        kObjNanoCola,
        kObjNukaCola,
        kObjB3,
        kObjCoffee
    };
    const int beverages = sizeof (beverage_candidate) / sizeof (shObjId);

    s.vat_bubbly_beverage = beverage_candidate[RNG (beverages)];
}
