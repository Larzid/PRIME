#include "Scenario.h"
#include "Util.h"

Scenario Scen;

struct _scen_internal
{
    shObjId vat_bubbly_beverage;
};

void
Scenario::save_state (FILE *f)
{
    fprintf (f, "%u\n", m->vat_bubbly_beverage);
}

bool
Scenario::load_state (FILE *f)
{
    int n_read;
    n_read = fscanf (f, "%u", (unsigned int *) &m->vat_bubbly_beverage);
    return n_read == 1;
}

void
Scenario::generate ()
{
    m = new _scen_internal;

    shObjId beverage_candidate[] =
    {
        kObjBeer,
        kObjNanoCola,
        kObjNukaCola,
        kObjB3,
        kObjCoffee
    };
    const int beverages = sizeof (beverage_candidate) / sizeof (shObjId);

    m->vat_bubbly_beverage = beverage_candidate[RNG (beverages)];
}

void
Scenario::clear ()
{
    delete m;
}

shObjId
Scenario::bubbly_beverage ()
{
    return m->vat_bubbly_beverage;
}
