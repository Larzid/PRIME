/* Hero inventory functions */

#include "Global.h"
#include "Util.h"
#include "Creature.h"
#include "Object.h"
#include "Interface.h"
#include <ctype.h>

void
shCreature::reorganizeInventory ()
{
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *obj1 = mInventory->get (i);
        innate_knowledge (obj1);
        if (!intr (kBlind))
            obj1->set (obj::known_appearance);

        for (int j = 0; j < i; ++j) {
            shObject *obj2 = mInventory->get (j);
            if (obj2->canMerge (obj1)) {
                mInventory->remove (obj1);
                obj2->merge (obj1);
                --i;
            }
        }
    }
    mInventory->sort (&compareObjects);
}


int
shCreature::addObjectToInventory (shObject *obj, bool quiet /* = false */)
{
    if (!isHero ())  quiet = true;
    int spot;
    char let, spots[52];

    innate_knowledge (obj);

    const char *what = AN (obj);

    for (int i = 0; i < 52; spots[i++] = 0)
        ;
    /* Try merging. */
    for (int i = 0; i < mInventory->count (); ++i) {
        shObject *iobj = mInventory->get (i);

        if (iobj->canMerge (obj)) {
            /* The getMass shuffle is required for objects that
               upon merging may discard unneeded containers. */
            mWeight -= iobj->getMass ();
            iobj->merge (obj);
            mWeight += iobj->getMass ();
            if (!quiet and isHero ()) {
                iobj->announce ();
            }
            computeIntrinsics (quiet);
            return 1;
        }
        let = iobj->mLetter;
        spot =
            (let >= 'a' and let <= 'z') ? let - 'a' :
            (let >= 'A' and let <= 'Z') ? let - 'A' + 26 :
            -1;

        assert (-1 != spot);
        assert (0 == spots[spot]);
        spots[spot] = 1;
    }
    /* Try to place item in exactly the spot it was before. */
    let = obj->mLetter;
    spot =
        (let >= 'a' and let <= 'z') ? let - 'a' :
        (let >= 'A' and let <= 'Z') ? let - 'A' + 26 :
        -1;
    int i;
    if (-1 != spot and 0 == spots[spot]) {
        i = spot; /* Place item under its old letter. */
    } else { /* Look for free spot. */
        for (i = 0; i < 52 and 0 != spots[i]; ++i)
            ;
    }
    /* Put item in free slot. */
    if (i < 52) {
        mInventory->add (obj);
        mWeight += obj->getMass ();
        obj->mLetter = i < 26 ? i + 'a' : i + 'A' - 26;
        obj->mLocation = shObject::kInventory;
        obj->mOwner = this;
        if (!quiet)  obj->announce ();
        mInventory->sort (&compareObjects);
        computeIntrinsics (quiet);
        return 1;
    }

    if (!quiet)
        msg (fmt ("You don't have any room in your pack for %s.", what));
    return 0;
}


int
selectObjects (shObjectVector *dest, shObjectVector *src, shObjId ilk)
{
    int n = 0;
    for (int i = 0; i < src->count (); ++i) {
        if (src->get (i) -> isA (ilk)) {
            dest->add (src->get (i));
            n++;
        }
    }
    return n;
}

int
selectObjects (shObjectVector *dest, shObjectVector *src, subtype_t st)
{
    int n = 0;
    for (int i = 0; i < src->count (); ++i)
        if (src->get (i) -> has_subtype (st)) {
            dest->add (src->get (i));
            ++n;
        }

    return n;
}

int
selectObjects (shObjectVector *dest, shObjectVector *src,
               shObjectType type)
{
    int n = 0;
    for (int i = 0; i < src->count (); ++i) {
        if ((kMaxObjectType == type) or (src->get (i) -> isA (type))) {
            dest->add (src->get (i));
            n++;
        }
    }
    return n;
}


int
selectObjectsByFlag (shObjectVector *dest, shObjectVector *src, unsigned int flag, bool invert)
{
    int count = 0;
    for (int i = 0; i < src->count (); ++i) {
        if (src->get (i)->is (flag) ? !invert : invert) {
            dest->add (src->get (i));
            ++count;
        }
    }
    return count;
}

int
deselectObjectsByFlag (shObjectVector *dest, shObjectVector *src,
                       unsigned int flag)
{
    return selectObjectsByFlag (dest, src, flag, true);
}


int
selectObjectsByFunction (shObjectVector *dest, shObjectVector *src,
                         bool (shObject::*idfunc) (), bool invert)
{
    int n = 0;
    for (int i = 0; i < src->count (); ++i) {
        if ((src->get (i)->*idfunc) () ? !invert : invert)  {
            dest->add (src->get (i));
            n++;
        }
    }
    return n;
}

int
deselectObjectsByFunction (shObjectVector *dest, shObjectVector *src,
                           bool (shObject::*idfunc) ())
{
    return selectObjectsByFunction (dest, src, idfunc, true);
}

int
selectObjectsByCallback (shObjectVector *dest, shObjectVector *src,
                         bool (*objfunc) (shObject *), bool invert)
{
    int n = 0;
    for (int i = 0; i < src->count (); ++i) {
        if ((*objfunc) (src->get (i)) ? !invert : invert)  {
            dest->add (src->get (i));
            n++;
        }
    }
    return n;
}

int
deselectObjectsByCallback (shObjectVector *dest, shObjectVector *src,
                           bool (*objfunc) (shObject *))
{
    return selectObjectsByCallback (dest, src, objfunc, true);
}



int
compare_letters (const void *a, const void *b)
{
    if (*(char*) a < *(char*)b)
        return -1;
    else if (*(char*)a > *(char*)b)
        return 1;
    else
        return 0;
}

// EFFECTS: prompts the user to select an item
//          if type is kMaxObjectType, no type restriction

shObject *
shCreature::quickPickItem (shObjectVector *v, const char *action, int flags,
                           int *count /* = NULL */ )
{
    if (0 == v->count () and !(flags & shMenu::kFiltered)) {
        I->p ("You don't have anything to %s.", action);
        return NULL;
    }
    char *actionbuf = GetBuf ();
    strncpy (actionbuf, action, SHBUFLEN);
    actionbuf[0] = toupper (actionbuf[0]);
    if (v->count ()) {
        shMenu *menu = I->newMenu (actionbuf, flags);
        shObject *obj;
        for (int i = 0; i < v->count (); ++i) {
            obj = v->get (i);
            menu->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
        }
        int res = menu->getPtrResult ((const void **) &obj, count);
        delete menu;
        if (res == DELETE_FILTER_SIGNAL) {
            /* Fall below to choice from whole inventory. */
        } else if (!obj) {
            I->pageLog ();
            I->nevermind ();
            return NULL;
        } else {
            return obj;
        }
    }
    if (flags & shMenu::kFiltered) {
        flags &= ~shMenu::kFiltered;
        shMenu *menu = I->newMenu (actionbuf, flags);
        shObject *obj;
        for (int i = 0; i < mInventory->count (); ++i) {
            obj = mInventory->get (i);
            menu->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
        }
        menu->getPtrResult ((const void **) &obj, count);
        delete menu;
        if (!obj) {
            I->pageLog ();
            I->nevermind ();
        }
        return obj;
    }
    return NULL;
}
