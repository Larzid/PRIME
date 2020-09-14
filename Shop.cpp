#include "Global.h"
#include "Util.h"
#include "Hero.h"
#include "Creature.h"
#include "Interface.h"

/******************************************************

    Shopping code and bribing lawyers.

******************************************************/

bool
shMapLevel::isInShop (int x, int y)
{
    switch (mRooms[mSquares[x][y].mRoomId].mType) {
    case shRoom::kGeneralStore:
    case shRoom::kHardwareStore:
    case shRoom::kSoftwareStore:
    case shRoom::kArmorStore:
    case shRoom::kWeaponStore:
    case shRoom::kImplantStore:
    case shRoom::kCanisterStore:
        return true;
    case shRoom::kNotRoom:
    case shRoom::kNormal:
    case shRoom::kCavern:
    case shRoom::kNest:
    case shRoom::kHospital:
    case shRoom::kSewer:
    case shRoom::kGarbageCompactor:
        return false;
    }
    return false;
}

/* Called to get shop name for spam program. */
const char *
shMapLevel::shopAd (int room)
{
    switch (mRooms[room].mType) {
    case shRoom::kGeneralStore:
        return "`All-Mart'";
    case shRoom::kHardwareStore:
        return "`Pod Depot'";
    case shRoom::kSoftwareStore:
        return "`MegaSoft'";
    case shRoom::kArmorStore:
        return "`Swashbuckler Outfitters'";
    case shRoom::kWeaponStore:
        return "`Bloodbath and Beyond'";
    case shRoom::kImplantStore:
        return "`Steve's Emporium of Previously Enjoyed Bionic implants'";
    case shRoom::kCanisterStore:
        return "`Imbibing Parlor'";
    default:
        return "`Unlicensed shop'";
    }
}

/* Called when the someone (usually the hero) enters a shop area. */
void
shCreature::enterShop ()
{
    if (!isHero ())  return;
    shCreature *shopkeeper = mLevel->getShopKeeper (mX, mY);

    if (!shopkeeper) {
        msg ("Hmm...  This store appears to be deserted.");
        return;
    }

    if (shopkeeper->isHostile ()) {
        return;
    }

    if (isHero ())  Hero.resetStoryFlag ("theft warning");

    if (!tryToTranslate (shopkeeper)) {
        msg (fmt ("%s beeps and bloops enthusiastically.", THE (shopkeeper)));
        return;
    }

    switch (mLevel->getRoom (mX, mY) -> mType) {
    case shRoom::kGeneralStore:
        I->p ("\"Welcome to 'All-Mart'!  How may I help you?\"");
        break;
    case shRoom::kHardwareStore:
        I->p ("\"Welcome to 'Pod Depot', "
              "the cheapest hardware store around!\"");
        break;
    case shRoom::kSoftwareStore:
        I->p ("\"Welcome to 'MegaSoft'!\""); break;
    case shRoom::kArmorStore:
        I->p ("\"Welcome to 'Swashbuckler Outfitters'!\""); break;
    case shRoom::kWeaponStore:
        /* this joke shamelessly stolen from "The Simpsons": */
        I->p ("\"Welcome to 'Bloodbath and Beyond'.\"");
        break;
    case shRoom::kImplantStore:
        I->p ("\"Welcome to Steve's Emporium of Previously Enjoyed"
              " Bionic implants!\""); break;
    case shRoom::kCanisterStore:
        I->p ("\"Welcome to 'Imbibing Parlor'!\""); break;
    default:
        I->p ("\"Uh... hi!\""); break;
    }
}


void
shCreature::leaveShop ()
{
    if (!isHero ())  return; /* NPCs do not shop. */
    shCreature *shopkeeper = mLevel->getShopKeeper (mX, mY);
    shObjectVector v;

    if (selectObjectsByFlag (&v, mInventory, obj::unpaid)) {
        /* The goods are no longer unpaid - they're stolen! */
        for (int i = 0; i < v.count (); ++i) {
            shObject *obj = v.get (i);
            obj->clear (obj::unpaid);
            if (shopkeeper)
                shopkeeper->mShopKeeper.mBill += obj->getCost ();
        }
    }
    if (!shopkeeper) {
        if (v.count ()) {
            /* Successfully robbed the shop! */
            I->p ("You make off with the loot.");
        }
        return;
    }
    if (shopkeeper->isHostile ()) {
        if (tryToTranslate (shopkeeper)) {
            I->p ("\"Not so fast!\"");
        } else {
            I->p ("%s is beeping frantically!", THE (shopkeeper));
        }
    } else if (!v.count () and !shopkeeper->mShopKeeper.mBill) {
        if (tryToTranslate (shopkeeper)) {
            I->p ("\"Come again soon!\"");
        } else {
            I->p ("%s twitters cheerfully.", THE (shopkeeper));
        }
    } else {
        if (v.count ()) {
            I->p ("You left with unpaid merchandise!");
        } else {
            I->p ("You left without paying your bill!");
        }
        shopkeeper->newEnemy (this);
    }
    Hero.resetStoryFlag ("theft warning");
}


static bool
not_in_same_room (shCreature *a, shCreature *b)
{
    return a->mLevel != b->mLevel or
        a->mLevel->getRoomID (a->mX, a->mY) !=
        a->mLevel->getRoomID (b->mX, b->mY);
}

void
shCreature::damagedShop (int x, int y)
{
    if (!isHero ())  return;
    shCreature *shopkeeper = mLevel->getShopKeeper (x, y);

    if (shopkeeper and !shopkeeper->isHostile ()) {
        if (tryToTranslate (shopkeeper)) {
            I->p ("\"Hey!  You can't do that here!\"");
        }
        shopkeeper->newEnemy (this);
    }
}


void
shCreature::dropIntoOwnedVat (shObject *obj)
{
    if (!isHero ())  return;
    int cnt = obj->mCount;
    int price = cnt * 10;
    shCreature *shopkeeper = mLevel->getShopKeeper (mX, mY);
    if (!shopkeeper)  return;

    if (not_in_same_room (this, shopkeeper)) {
        shopkeeper->newEnemy (this);
        return;
    }

    if (tryToTranslate (shopkeeper)) {
        I->p ("\"You owe %s $%d for polluting the vat.\"",
              THE (shopkeeper), price);
    } else {
        I->p ("%s testily beeps something about %d buckazoids.",
              THE (shopkeeper), price);
    }
    shopkeeper->mShopKeeper.mBill += price;
}


void
shCreature::quaffFromOwnedVat (shFeature *vat)
{
    if (!isHero ())  return;
    const char* vatAds[] =
    {
        "vat of delicious sludge",
        "vat of glorious sludge",
        "vat of magnificent sludge",
        "vat of royal sludge"
    };
    const int price = 80;
    shCreature *shopkeeper = mLevel->getShopKeeper (mX, mY);
    if (!shopkeeper) return;

    if (tryToTranslate (shopkeeper)) {
        /* Clerk might tell you what kind of ingredients vat contains. */
        if (!RNG (4)) {
            vat->mVat.mAnalyzed = 1;
        }
        I->p ("\"Only $%d for a sip from from this %s!\"", price,
            vat->mVat.mAnalyzed ? vat->getDescription () : vatAds[RNG (4)]);
    } else {
        I->p ("%s calmly beeps something about %d buckazoids.",
              THE (shopkeeper), price);
    }
    shopkeeper->mShopKeeper.mBill += price;
}

/* Vat was teleported but ended up in same room. */
void
shCreature::movedVat ()
{
    if (!isHero ())  return;
    shCreature *shopkeeper = mLevel->getShopKeeper (mX, mY);

    if (!shopkeeper->isHostile ()) {
        if (tryToTranslate (shopkeeper)) {
            I->p ("\"Feel free to rearrange furniture "
                  "but don't forget to pay.\"");
        } else {
            I->p ("%s chirps.", THE (shopkeeper));
        }
    }
}

/* Vat was teleported and clerkbot demands money for theft. */
void
shCreature::stolenVat ()
{
    if (!isHero ())  return;
    const int price = 300;
    shCreature *shopkeeper = mLevel->getShopKeeper (mX, mY);

    if (!shopkeeper->isHostile ()) {
        if (not_in_same_room (this, shopkeeper)) {
            shopkeeper->newEnemy (this);
            return;
        }
        if (tryToTranslate (shopkeeper)) {
            I->p ("\"You steal it, you bought it!\"");
            I->p ("\"You owe $%d for that vat.\"", price);
        } else {
            I->p ("%s testily beeps something about %d buckazoids.",
                  THE (shopkeeper), price);
        }
    } else {
        if (tryToTranslate (shopkeeper)) {
            I->p ("\"You will pay for this humilation too!\"");
        } else {
            I->p ("%s beeps something in fury.", THE (shopkeeper));
        }
    }
    shopkeeper->mShopKeeper.mBill += price;
}


void
shCreature::billableOffense (shObject *obj, const char *gerund, int fee)
{
    if (!isHero ())  return;
    if (obj->is (obj::unpaid)) {
        shCreature *shopkeeper = mLevel->getShopKeeper (mX, mY);
        if (!shopkeeper)
            return;
        if (not_in_same_room (this, shopkeeper)) {
            shopkeeper->newEnemy (this);
            return;
        }
        if (tryToTranslate (shopkeeper)) {
            I->p ("%s testily beeps something about %d buckazoids",
                  THE (shopkeeper), fee);
        } else {
            I->p ("\"You did $%d buckazoids worth of damage!\"", fee);
            I->p ("You owe compensation for %s %s.", gerund, THE (obj));
            shopkeeper->mShopKeeper.mBill += fee;
        }
    }
}

/* Hero may use unpaid tools for a fee. */
void
shCreature::employedItem (shObject *obj, int fee)
{
    if (!isHero ())  return;
    if (obj->is (obj::unpaid)) {
        shCreature *shopkeeper = mLevel->getShopKeeper (mX, mY);
        if (!shopkeeper)
            return;
        if (!fee) {
            if (tryToTranslate (shopkeeper)) {
                I->p ("%s seems to be confused about something.",
                    THE (shopkeeper));
            } else {
                I->p ("%s emits some strange chirps.", THE (shopkeeper));
            }
        } else {
            if (not_in_same_room (this, shopkeeper)) {
                shopkeeper->newEnemy (this);
                return;
            }
            shopkeeper->mShopKeeper.mBill += fee;
            if (tryToTranslate (shopkeeper)) {
                I->p ("\"Feel free to use this %s for only %d buckazoids.\"",
                    THE (obj), fee);
            } else {
                I->p ("%s calmly beeps something about %d buckazoids.",
                    THE (shopkeeper), fee);
            }
        }
    }
}

void
shCreature::usedUpItem (shObject *obj, int cnt, const char *action)
{
    if (!isHero ())  return;
    shCreature *shopkeeper = NULL;
    if (obj->mOwner) {
        shopkeeper = mLevel->getShopKeeper (mX, mY);
    } else {
        if (Level->isInBounds (obj->mX, obj->mY)) {
            shopkeeper = mLevel->getShopKeeper (obj->mX, obj->mY);
        }
    }

    if (shopkeeper and
        !shopkeeper->isHostile () and
        obj->is (obj::unpaid))
    {
        Hero.resetStoryFlag ("theft warning");
        if (not_in_same_room (this, shopkeeper)) {
            shopkeeper->newEnemy (this);
            return;
        }
        if (cnt > obj->mCount) {
            cnt = obj->mCount;
        }
        int price = obj->myIlk ()->mCost * cnt;

        if (tryToTranslate (shopkeeper)) {
            I->p ("\"You %s it, you bought it!\"", action);
            I->p ("You owe %s $%d for %s.",
                  THE (shopkeeper), price, obj->those ());
        } else {
            I->p ("%s testily beeps something about %d buckazoids.",
                  THE (shopkeeper), price);
        }
        shopkeeper->mShopKeeper.mBill += price;
    }
}


void
shCreature::pickedUpItem (shObject *obj)
{
    if (!isHero ())  return;
    shCreature *shopkeeper = mLevel->getShopKeeper (mX, mY);

    if (shopkeeper and
        obj->is (obj::unpaid) and
        !shopkeeper->isHostile () and
        shopkeeper->canSee (this))
    {
        Hero.resetStoryFlag ("theft warning");

        if (not_in_same_room (this, shopkeeper)) {
            shopkeeper->newEnemy (this);
            return;
        }
        int price = obj->myIlk ()->mCost * obj->mCount;
        if (tryToTranslate (shopkeeper)) {
            switch (RNG (5)) {
            case 0:
                I->p ("\"%s %s yours for only %d buckazoids!\"", THESE (obj),
                      obj->mCount > 1 ? "are" : "is", price);
                break;
            case 1:
                I->p ("\"Take %s home for %d buckazoids!\"",
                      THESE (obj), price);
                break;
            case 2:
                I->p ("\"Only $%d for %s!\"", price, THESE (obj));
                break;
            case 3:
                I->p ("\"%s, on sale for %d buckazoids!\"",
                      obj->anQuick (), price);
                break;
            case 4:
                I->p ("\"%s for $%d!  Such a deal!\"",
                      obj->anQuick (), price);
                break;
            }
        } else {
            I->p ("%s beeps something about %d %s.",
                  THE (shopkeeper), price,
                  price > 1 ? "buckazoids" : "buckazoid");
        }
    }
}


void
shCreature::maybeSellItem (shObject *obj)
{
    if (!isHero ())  return;
    shCreature *shopkeeper = mLevel->getShopKeeper (mX, mY);

    if (shopkeeper and
        !shopkeeper->isHostile () and
        !obj->is (obj::unpaid) and
        !obj->isA (kMoney) and
        shopkeeper->canSee (this))
    {
        int price = obj->myIlk ()->mCost * obj->mCount / 10;
        int quote = price;
        const char *the_sk = THE (shopkeeper);
        const char *an_obj = obj->anQuick ();

        Hero.resetStoryFlag ("theft warning");

        if (shopkeeper->countMoney () < quote) {
            quote = shopkeeper->countMoney ();
        }

        if (0 == quote) {
            if (tryToTranslate (shopkeeper)) {
                I->p ("%s is unable to buy %s from you.", the_sk, an_obj);
            } else {
                I->p ("%s whirs disappointedly.", the_sk, an_obj);
            }
            return;
        } else if (quote < price) {
            if (tryToTranslate (shopkeeper)) {
                I->p ("\"I can only offer you $%d for %s.\"", quote, an_obj);
            } else {
                I->p ("%s chirps and beeps something about %d buckazoids.",
                      the_sk, quote);
            }
        } else {
            if (tryToTranslate (shopkeeper)) {
                I->p ("%s offers to buy %s for %d buckazoids.",
                      the_sk, an_obj, quote);
            } else {
                I->p ("%s chirps and beeps something about %d buckazoids.",
                      the_sk, quote);
            }
        }
        if (I->yn ("Sell %s?", obj->mCount > 1 ? "them" : "it")) {
            obj->set (obj::unpaid);
            shopkeeper->loseMoney (quote);
            gainMoney (quote);
        }
        I->pageLog ();
    }
}

/* TODO: Split up! */
void
shCreature::payShopkeeper ()
{
    if (!isHero ())  return;
    shCreature *shopkeeper = mLevel->getShopKeeper (mX, mY);
    shCreature *guard = mLevel->getGuard ();
    shCreature *doc = mLevel->getDoctor (mX, mY);
    shCreature *trader = mLevel->getMelnorme ();
    shObject *obj;
    shObjectVector v;
    int price;

    if (trader) {
        int myRoom = mLevel->getRoomID (mX, mY);
        int traderRoom = mLevel->getRoomID (trader->mX, trader->mY);
        if (trader->isAdjacent (mX, mY) and
            /* It is possible to apologize in a corridor but not
               to trade afterwards until Melnorme relocates. */
            ((!myRoom or !traderRoom) and !trader->isHostile ()))
        {
            if (tryToTranslate (trader)) {
                I->p ("\"Doing business in a corridor is bad style.\"",
                      THE (trader));
            } else {
                I->p ("%s refuses to trade for some reason.", THE (trader));
            }
            return;
        } else if (myRoom == traderRoom) {
            payMelnorme (trader);
            return;
        }
        /* No return. Trader check is global instead only for current room. */
    }

    if (doc) {
        if (!isAdjacent (doc->mX, doc->mY)) {
            I->p ("You need to move next to %s first.", THE (doc));
        } else {
            payDoctor (doc);
        }
        return;
    }

    if (!shopkeeper and !guard) {
        I->p ("There's nobody around to pay.");
        return;
    }

    if (guard and !shopkeeper) {
        if (guard->isHostile ()) {
            I->p ("%s intends to collect payment from your dead body!",
                  THE (guard));
            return;
        }
        price = guard->mGuard.mToll;
        if (0 == price) {
            I->p ("You've already paid the toll.");
            return;
        } else if (2 == guard->mGuard.mChallengeIssued) {
            I->p ("As an on-duty janitor, you are exempt from the toll.");
            return;
        } else if (price > 0) {
            if (I->yn ("Pay %s %d buckazoids?", THE (guard), price)) {
                if (price > countMoney ()) {
                    I->p ("But you don't have that much money.");
                    return;
                } else {
                    loseMoney (price);
                    guard->gainMoney (price);
                    guard->mGuard.mToll = 0;
                    if (tryToTranslate (guard)) {
                        I->p ("\"You may pass.\"");
                    } else {
                        I->p ("%s beeps calmly.", THE (guard));
                    }
                }
            }
        }
        return;
    }

    if (shopkeeper->isHostile ()) {
        /* the record-keeping of the shopping bill is somewhat up in the air
           once the shopkeeper is angry, so the quick and dirty solution is
           not to allow pacification
         */
        I->p ("%s intends to collect payment from your dead body!",
              THE (shopkeeper));
        return;
    }

    price = shopkeeper->mShopKeeper.mBill;
    if (price) {
        I->p ("You owe %d buckazoids for the use of merchandise.", price);
        if (price > countMoney ()) {
            I->p ("But you don't have enough money to cover that bill.");
        } else {
            loseMoney (price);
            shopkeeper->gainMoney (price);
            shopkeeper->mShopKeeper.mBill = 0;
            I->p ("You pay that amount.");
        }
        I->drawSideWin (this);
        I->pause ();
    }

    if (0 == selectObjectsByFlag (&v, mInventory, obj::unpaid)) {
        I->p ("You don't have any merchandise to pay for.");
        return;
    }

    shMenu *menu = I->newMenu ("Pay for which items?",
        shMenu::kMultiPick | shMenu::kCategorizeObjects);
    for (int i = 0; i < v.count (); i++) {
        obj = v.get (i);
        menu->addPtrItem (obj->mLetter, obj->inv (), obj, obj->mCount);
    }
    while (menu->getPtrResult ((const void **) &obj)) {
        price = obj->mCount * obj->myIlk ()->mCost;
        if (price > countMoney ()) {
            I->p ("You can't afford to pay for %s.", THE (obj));
        } else {
            loseMoney (price);
            shopkeeper->gainMoney (price);
            if (tryToTranslate (shopkeeper)) {
                if (RNG (2)) {
                    /* sometimes, the clerk knows what he's selling, but
                       never if it's buggy or optimized. */
                    obj->set (obj::known_type);
                }
                I->p ("\"That'll be %d buckazoids for %s, thank you.\"",
                      price, obj->theQuick ());
            } else {
                I->p ("You buy %s for %d buckazoids.", THE (obj), price);
            }
            obj->clear (obj::unpaid);
        }
    }
    delete menu;
    Hero.resetStoryFlag ("theft warning");
    I->drawSideWin (this); /* Monetary status might have changed. */
}


/* shopkeeper strategy:
     stand in front of the shop doorway unless hero is there

   returns ms elapsed, -2 if the monster dies
*/
int
shCreature::doShopKeep ()
{
    shCreature *h = Hero.cr ();
    int elapsed;
    int res = -1;
    int retry = 3;

    while (-1 == res) {
        if (!retry--) {
            return 200;
        }

        switch (mTactic) {

        case kNewEnemy:
            mStrategy = kAngryShopKeep;
            mTactic = kReady;
            /* Alert guards. */
            for (int i = 0; i < mLevel->mCrList.count (); ++i) {
                shCreature *c = mLevel->mCrList.get (i);
                if (c->isRobot () and !c->isHostile () and !c->isPet ()) {
                    if (mLevel->isTownLevel () or
                        mLevel->getRoomID (c->mX, c->mY) == mShopKeeper.mShopId)
                    {
                        c->newEnemy (Hero.cr ());
                    }
                }
            }
            /* FIXME: at the moment monsters do not have the ability
                      to open hidden doors.  When they do get it this
                      action will no longer be needed. */
            if (mLevel->isTownLevel ()) {
                for (int i = 0; i < 3; ++i) {
                    shFeature *f = mLevel->getFeature (27 + 2*i, 3);
                    if (f and f->mType == shFeature::kDoorHiddenHoriz)
                        f->mType = shFeature::kDoorClosed;
                }
            }
            return doAngryShopKeep ();
        case kMoveTo:
            res = doMoveTo ();
            continue;
        case kReady:
            {
                shFeature *f = mLevel->getFeature (mX, mY);
                if (!mLevel->isInShop (mX, mY) or
                    /* Somehow, we're not in our shop! */
                    (f and f->isDoor ()))
                    /* We are blocking the door. Never good. */
                {
                    mDestX = mShopKeeper.mHomeX;
                    mDestY = mShopKeeper.mHomeY;

                    if (setupPath ()) {
                        mTactic = kMoveTo;
                        continue;
                    } else {
                        return LONGTURN;
                    }
                }
            }

            if (canSee (h) and
                mLevel->isInShop (h->mX, h->mY) and
                ( /* If hero is not in _our_ shop ignore it. */
                 mLevel->getRoomID (h->mX, h->mY) ==
                 mLevel->getRoomID (mX, mY)
                ))
            {
                if (mLevel->isInDoorWay (h->mX, h->mY)) {
                    shObjectVector v;
                    if (mShopKeeper.mBill or
                        selectObjectsByFlag (&v, h->mInventory, obj::unpaid))
                    {   /* Hero about to leave w/ unpaid merchandise */
                        if (!Hero.getStoryFlag ("theft warning")) {
                            if (h->tryToTranslate (this)) {
                                I->p ("\"Please don't "
                                      "leave without paying!\"");
                            } else {
                                I->p ("%s chitters urgently.", the ());
                            }
                            Hero.setStoryFlag ("theft warning", 1);
                        }
                        mDestX = mShopKeeper.mHomeX;
                        mDestY = mShopKeeper.mHomeY;
                        res = doQuickMoveTo ();
                        continue;
                    } else if (mX == mShopKeeper.mHomeX and
                               mY == mShopKeeper.mHomeY)
                    {   /* Hero is in the threshold of the shop doorway, and
                           we're in the way! */
                        mDestX = mX;
                        mDestY = mY;
                        if (!mLevel ->
                            findAdjacentUnoccupiedSquare (&mDestX, &mDestY))
                        {
                            elapsed = doQuickMoveTo ();
                            if (-1 == elapsed) elapsed = 800;
                            return elapsed;
                        }
                        return RNG (300, 1000); /* nowhere to go, just wait. */
                    }
                } else if (0 == RNG (3)) {
                    /* move to an empty square near the home square */
                    mDestX = mShopKeeper.mHomeX;
                    mDestY = mShopKeeper.mHomeY;
                    if (!mLevel ->
                        findAdjacentUnoccupiedSquare (&mDestX, &mDestY))
                    {
                        elapsed = doQuickMoveTo ();
                        if (-1 == elapsed) elapsed = 800;
                        return elapsed;
                    }
                    return RNG (300, 1000); /* nowhere to go, let's wait... */
                } else if (0 == RNG (50)) {
                    const char *quips[9] = {
                        "I'd give them away, but my wifebot won't let me!",
                        "All merchandise sold as-is.",
                        "Shoplifters will be vaporized!",
                        "Weyland-Yutani closed at -2.56 yesterday...",
                        "Goddamned warpspace connection bill.",
                        "What a crappy Z.O.T. round!",
                        "I sell FrungyTV subscriptions, too.",
                        "Hey, don't step on the merchandise!",
                        "All pickaxes are to be left outside."
                    };
                    if (Hero.cr ()->tryToTranslate (this)) {
                        I->p ("\"%s\"", quips[RNG(9)]);
                    }
                    return RNG (500, 1000);
                }
                return RNG (500, 1000);
            } else {
                return RNG (800, 1600);
            }
        case kFight:
        case kShoot:
            mTactic = kReady;
            debug.log ("Unexpected shopkeeper tactic!");
        }
    }

    return RNG (300, 1000); /* keep on lurking */
}




//returns ms elapsed, -2 if the monster dies
int
shCreature::doAngryShopKeep ()
{
    if (canSee (Hero.cr ()) and 0 == RNG (10)) {
        if (Hero.cr ()->tryToTranslate (this)) {
            I->p ("\"%s\"",
                  RNG (2) ? "Stop, thief!" : "You shoplifting scum!");
        } else {
            I->p ("%s beeps angrily at you.", the ());
        }
    }
    return doWander ();
}

void
shCreature::bribe (shCreature *m)
{
    if (m->isA (kMonLawyer)) {
        if (m->isHostile ()) {
            int bribe = 250 + RNG (501);
            if (m->is (kGenerous))  bribe = 250;
            if (I->yn ("Offer $%d to %s?", bribe, THE (m))) {
                if (countMoney () >= bribe) {
                    I->p ("%s takes your money.", THE (m));
                    loseMoney (bribe);
                    m->gainMoney (bribe);
                    m->mDisposition = kIndifferent;
                } else {
                    I->p ("You don't have that much money.");
                }
            }
        } else {
            I->p ("You have already paid off %s.", THE (m));
        }
    } else if (m->isA (kMonCreepingCredits)) {
        int cash = countMoney ();
        loseMoney (cash);
        m->gainMoney (cash);
        m->mMaxHP += cash / 8;
        m->mHP += cash / 4;
        if (m->mHP > m->mMaxHP) m->mHP = m->mMaxHP;
        I->p ("You give all your money to %s.", THE (m));
        if (m->isHostile () and cash >= 50) {
            m->mDisposition = kIndifferent;
        }
    } else if (m->isA (kMonMelnorme)) {
        payMelnorme (m);
    } else if (m->isA (kMonClerkbot) or m->isA (kMonDocbot) or
               m->isA (kMonGuardbot) or m->isA (kMonSecuritron) or
               m->isA (kMonWarbot))
    {
        if (tryToTranslate (m)) {
            I->p ("\"Sorry, dishonest deals protocol is not supported by this model.\"");
            I->p ("\"Please use official pay command.  It is '%s'.\"",
                I->getKeyForCommand (shInterface::kPay));
        } else {
            I->p ("%s beeps three times and ignores you.", THE (m));
        }
    } else {
        I->p ("%s does not seem interested in your money.", THE (m));
    }
}
