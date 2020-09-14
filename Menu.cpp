#include <stdio.h>
#include <ctype.h>
#include "Interface.h"


shMenuChoice::shMenuChoice (char letter, char alt, const char *text,
    const void *value, int count, int selected)
{
    mLetter = letter;
    mAltLetter = alt;
    strncpy (mText, text, 255); mText[255] = 0;
    mValue.mPtr = value;
    mSelected = selected;
    mCount = count;
}

shMenuChoice::shMenuChoice (char letter, char alt, const char *text,
    int value, int count, int selected)
{
    mLetter = letter;
    mAltLetter = alt;
    strncpy (mText, text, 255); mText[255] = 0;
    mValue.mInt = value;
    mSelected = selected;
    mCount = count;
}


shMenu::shMenu (const char *prompt, int flags)
    : mChoices ()
{
    strncpy (mPrompt, prompt, 79); mPrompt[79] = 0;
    mFlags = flags;
    mResultIterator = 0;
    mOffset = 0;
    mDone = 0;
    mNum = 0;
    mObjTypeHack = kMaxObjectType;
    mLastLet = 'a'-1;
    mHelpFileName = NULL;
    mHelpHandlerP = NULL;
    mHelpHandlerI = NULL;
    mHelpMode = false;
    mHeight = I->getMaxLines ();
    mWidth = I->getMaxColumns ();
    if (!(mFlags & kNoPick)) {
        /* KLUDGE: avoid repeating a letter on the same page */
        mHeight = mini (52, mHeight);
    }
}

shMenu::~shMenu (void)
{
    for (int i = 0; i < mChoices.count (); ++i)
        delete mChoices.get (i);
}

char
shMenu::nextLet (void)
{
    ++mLastLet;
    if (mLastLet == 'z' + 1)  mLastLet = 'A';
    return mLastLet;
}

void
shMenu::addIntItem (char letter, const char *text, int value,
                    int count /* = 1 */, int selected /* = 0 */)
{
    if (0 == letter)
        letter = ' ';

    char alt = letter >= 0 ? nextLet () : ' ';
    mChoices.add (new shMenuChoice (letter, alt, text, value, count, selected));
}

void
shMenu::addPtrItem (char letter, const char *text, const void *value,
                    int count /* = 1 */, int selected /* = 0 */)
{
    const char *typeHeader[kMaxObjectType] =
    {
        "UNINITIALIZED       (! please report !)",
        "Money               (toggle all with $)",
        "Bionic Implants     (toggle all with :)",
        "Floppy Disks        (toggle all with ?)",
        "Canisters           (toggle all with !)",
        "Tools               (toggle all with ()",
        "Armor               (toggle all with [)",
        "Weapons             (toggle all with ))",
        "Ammunition          (toggle all with =)",
        "Other               (toggle all with &)",
        "Ray Guns            (toggle all with /)",
        "Energy Cells        (toggle all with *)"
    };

    if (0 == letter)
        letter = ' ';

    if (mFlags & kCategorizeObjects and value) {
        shObjectType t = ((shObject *) value) -> apparent ()->mType;
        if (t != mObjTypeHack) {
            if (t >= kUninitialized and t <= kEnergyCell) {
                if (mFlags & kMultiPick) { /* Show toggle key. */
                    addHeader (typeHeader[t]);   /* From above table. */
                } else {
                    addHeader (objectTypeHeader[t]); /* Default. */
                }
            } else {
                addHeader ("------");
            }
            mObjTypeHack = t;
        }
    }
    char alt = letter >= 0 ? nextLet () : ' ';
    mChoices.add (new shMenuChoice (letter, alt, text, value, count, selected));
}

void
shMenu::addHeader (const char *text)
{   /* prints out the header */
    addPtrItem (-1, text, NULL, -1);
}

void
shMenu::addPageBreak (void)
{   /* Just a prettifier. */
    addPtrItem (0, "", NULL, -1);
    /* -2 is signal to adjust space on both sides of text. */
    addPtrItem (-2, "---..oo..--=oOo=--..oo..---", NULL, -1);
    addPtrItem (0, "", NULL, -1);
}

void
shMenu::addText (const char *text)
{
    addPtrItem (' ', text, NULL);
}

void
shMenu::finish () {
    assert (kNoPick & mFlags);
    accumulateResults ();
}

shMenuChoice *
shMenu::getResultChoice (void)
{
    if (!mDone)
        accumulateResults ();

    while (mResultIterator < mChoices.count ()) {
        shMenuChoice *choice = mChoices.get (mResultIterator++);
        if (choice->mSelected)
            return choice;
    }
    return NULL;
}

/* call this repeatedly to store the selected results into value and count.
  RETURNS: 0 if there are no (more) results, 1 o/w
*/

int
shMenu::getIntResult (int *value, int *count /* = NULL */)
{
    shMenuChoice *choice = getResultChoice ();

    if (choice) {
        *value = choice->mValue.mInt;
        if (NULL != count) {
            *count = choice->mSelected;
        }
        return 1;
    }
    *value = 0;
    return 0;
}

int
shMenu::getPtrResult (const void **value, int *count /* = NULL */)
{
    shMenuChoice *choice = getResultChoice ();
    if (mDone == DELETE_FILTER_SIGNAL)
        return DELETE_FILTER_SIGNAL;

    if (choice) {
        *value = choice->mValue.mPtr;
        if (NULL != count) {
            *count = choice->mSelected;
        }
        return 1;
    }
    *value = NULL;
    return 0;
}

void
shMenu::getRandIntResult (int *value, int *count /* = NULL */)
{
    shMenuChoice *choice = mChoices.get (RNG (mChoices.count ()));

    *value = choice->mValue.mInt;
    if (NULL != count) {
        *count = choice->mSelected;
    }
}

void
shMenu::getRandPtrResult (const void **value, int *count /* = NULL */)
{
    shMenuChoice *choice = mChoices.get (RNG (mChoices.count ()));

    *value = choice->mValue.mPtr;
    if (NULL != count) {
        *count = choice->mSelected;
    }
}

int
shMenu::getNumChoices (void)
{
    int num = 0;
    for (int i = 0; i < mChoices.count (); ++i)
        if (mChoices.get (i)->mSelected)
            ++num;
    return num;
}

void
shMenu::attachHelp (const char *fname)
{
    mHelpFileName = fname;
}

void
shMenu::attachHelp (shHelpFuncPtr handler)
{
    mHelpHandlerP = handler;
}

void
shMenu::attachHelp (shHelpFuncInt handler)
{
    mHelpHandlerI = handler;
}

void
shMenu::showHelp ()
{
    int lines;
    const char **text = prepareHelp (&lines);
    I->clearWin (shInterface::kMenuHelp);
    for (int i = 0; i < lines; ++i) {
        int len = strlen (text[i]);
        I->winGoToYX (shInterface::kMenuHelp, i, 0);
        for (int j = 0; j < len; ++j) {
            if (islower (text[i][j]) or text[i][j] == ':' or
                (j == 0 and (text[i][j] == 'I' or text[i][j] == 'Q')))
            {
                I->setWinColor (shInterface::kMenuHelp, kGray);
            } else {
                I->setWinColor (shInterface::kMenuHelp, kWhite);
            }
            I->winPutchar (shInterface::kMenuHelp, text[i][j]);
        }
    }
    free (text);
}

void
shMenu::select (int i1, int i2,   /* mChoices[i1..i2) */
                int action,       /* 0 unselect, 1 select, 2 toggle */
                shObjectType t)   /* = kUninitialized */
{
    if (!(mFlags & kMultiPick))
        return;

    for (int i = i1; i < i2; ++i) {
        shMenuChoice *choice = mChoices.get (i);
        if (choice->mCount < 0) {
            continue;
        }
        if (t and
            mFlags & kCategorizeObjects and
            choice->mValue.mPtr and
            t != ((shObject *) choice->mValue.mPtr) -> apparent ()->mType)
        {
            continue;
        }
        if (0 == action) {
            choice->mSelected = 0;
        } else if (1 == action) {
            choice->mSelected = choice->mCount;
        } else if (2 == action) {
            if (!choice->mSelected)
                choice->mSelected = choice->mCount;
            else
                choice->mSelected = 0;
        }
    }
}

const char **
shMenu::prepareHelp (int *lines)
{   /* Building blocks for help. */
    const char *navigation =
"ARROWS, PAGE UP, PAGE DOWN  navigate    SPACE, ENTER, ESCAPE  finish";
    static char count[] =
"Quantity: [     ]   press NUMBERS to change quantity of selected items";
    const char *multipick =
"Items:  , select all  - deselect all  @ toggle all  LETTER toggle single";

    const char *help    = "TAB  show help file   ";
    const char *helpoff = "TAB  enter help mode  ";
    const char *helpon  = "TAB  exit help mode   ";
    const char *filter = "BACKSPACE  disable filter   ";
    const char *singlepick = "LETTER  choose and accept";
    const char *helppick = "LETTER  show help about item";

    static char lastline[81];

    /* Choose appropriate blocks. */
    *lines = 0;
    const char **text = (const char **) calloc (4, sizeof (char *));
    if (mFlags & kNoHelp)  return text;

    text[(*lines)++] = navigation;

    if ((mFlags & kCountAllowed) and !mHelpMode) {
        text[(*lines)++] = count;
        if (mNum == 0) {
            strncpy (count + 11, " all ", 5); /* Does not place \0 anywhere. */
        } else {
            sprintf (count + 11, "%5d", mNum < 99999 ? mNum : 99999);
            count[16] = ']'; /* Replace \0 added by sprintf. */
        }
    }

    if ((mFlags & kMultiPick) and !mHelpMode)
        text[(*lines)++] = multipick;

    const char *helpline = NULL;
    if (mHelpFileName)  helpline = help;
    if (mHelpHandlerP or mHelpHandlerI)
        helpline = mHelpMode ? helpon : helpoff;
    if (helpline or (mFlags & kFiltered) or !(mFlags & kMultiPick)) {
        snprintf (lastline, 81, "%s%s%s",
            helpline ? helpline : "",
            mFlags & kFiltered ? filter : "",
            mFlags & kMultiPick ? "" : (mHelpMode ? helppick : singlepick));
        text[(*lines)++] = lastline;
    }

    return text;
}

void
shMenu::dropResults (void)
{
    mDone = 0;
    mResultIterator = 0;
    for (int i = 0; i < mChoices.count (); ++i)
        mChoices.get (i) -> mSelected = 0;
}

/* Returns true when key pressed was valid. */
bool
shMenu::interpretKey (int key, shInterface::Command cmd)
{
    if (cmd) switch (cmd) {
    case shInterface::kDrop: /* Drop item filter. */
        if (kFiltered & mFlags) {
            mDone = DELETE_FILTER_SIGNAL;
            return true;
        }
        return false;
    case shInterface::kMoveNW: /* Home */
        mLast -= mOffset;
        mOffset = 0;
        return true;
    case shInterface::kMoveSW: /* End */
        mOffset += mChoices.count () - mLast;
        mLast = mChoices.count ();
        return true;
    case shInterface::kMoveUp:
        if (mOffset > 0) {
            --mOffset;
            --mLast;
        }
        return true;
    case shInterface::kMoveDown:
        if (mLast < mChoices.count ()) {
            ++mOffset;
            ++mLast;
        }
        return true;
    case shInterface::kMoveNE: /* Page up */
        mOffset -= mItemHeight / 2;
        mLast -= mItemHeight / 2;
        if (mOffset < 0) {
            mLast -= mOffset;
            mOffset = 0;
        }
        return true;
    case shInterface::kMoveSE: /* Page down */
        mLast += mItemHeight / 2;
        mOffset += mItemHeight / 2;
        if (mLast >= mChoices.count ()) {
            mOffset -= mLast - mChoices.count ();
            mLast = mChoices.count ();
        }
        return true;
    case shInterface::kHelp:
        if (mHelpFileName) {
            shTextViewer *viewer = new shTextViewer (mHelpFileName);
            viewer->show ();
            delete viewer;
        } else if (mHelpHandlerP or mHelpHandlerI) {
            mHelpMode = !mHelpMode;
        } else {
            return false;
        }
        return true;
    default: break;
    }
    switch (key) {
    case '@': /* toggle all */
        select (0, mChoices.count(), 2);
        return true;
    case '-': /* deselect all */
        select (0, mChoices.count(), 0);
        return true;
    case ',': /* select all */
        select (0, mChoices.count(), 1);
        return true;
    case '$': /* toggle all cash */
        select (0, mChoices.count(), 2, kMoney);
        return true;
    case ':': /* toggle all implants */
        select (0, mChoices.count(), 2, kImplant);
        return true;
    case '?': /* toggle all floppies */
        select (0, mChoices.count(), 2, kFloppyDisk);
        return true;
    case '!': /* toggle all cans */
        select (0, mChoices.count(), 2, kCanister);
        return true;
    case '(': /* toggle all tools */
        select (0, mChoices.count(), 2, kTool);
        return true;
    case '[': /* toggle all armor */
        select (0, mChoices.count(), 2, kArmor);
        return true;
    case ')': /* toggle all weapons */
        select (0, mChoices.count(), 2, kWeapon);
        return true;
    case '=': /* toggle all ammo */
        select (0, mChoices.count(), 2, kProjectile);
        return true;
    case '/': /* toggle all ray guns */
        select (0, mChoices.count(), 2, kRayGun);
        return true;
    case '*': /* toggle all energy */
        select (0, mChoices.count(), 2, kEnergyCell);
        return true;
    case '&': /* toggle all other things */
        select (0, mChoices.count(), 2, kOther);
        return true;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        if (mFlags & kCountAllowed) {
            mNum = mNum * 10 + key - '0';
            if (mNum >= 1000000) {
                mNum = 1000000;
            }
            return true;
        } else {
            return false;
        }
    default:
        if (!mHelpMode and (mFlags & kNoPick))
            return false;
        for (int i = 0; i < mChoices.count (); ++i) {
            shMenuChoice *item = mChoices.get (i);
            if (mHelpMode and item->mAltLetter == key) {
                if (mHelpHandlerP)
                    mHelpHandlerP (item->mValue.mPtr);
                else
                    mHelpHandlerI (item->mValue.mInt);
                return true;
            } else if (item->mLetter == key) {
                if ((mFlags & kCountAllowed) and mNum) { /* Set exact number. */
                    if (mNum > item->mCount) {
                        /* TODO: Warn user not that many items are available. */
                        mNum = item->mCount;
                    }
                } else if (mFlags & kSelectIsPlusOne) {
                    mNum = item->mSelected == item->mCount ? 0 :
                           item->mSelected + 1;
                } else { /* Toggle all or none. */
                    mNum = item->mSelected ? 0 : item->mCount;
                }
                item->mSelected = mNum;
                mNum = 0;
                if (!(mFlags & kMultiPick)) {
                    mDone = 1;
                }
                return true;
            }
        }
        return false;
    }
    return false;
}

/* Build status bar featuring numbers of lines shown and scroll information. */
const char *
shMenu::bottomLine (int curpos)
{
    const char *indicator = NULL;
    bool lineinfo = true;
    if (mLast == mChoices.count () and mOffset == 0) {
        indicator = "--All shown--";
        lineinfo = false;
    } else if (curpos == mChoices.count ()) {
        indicator = "--End--";
    } else if (0 == mOffset) {
        indicator = "--Top--";
    } else {
        indicator = "--More--";
    }
    /* Show weight for object menus. */
    char wgtbuf[19] = "                  ";
    if (mFlags & kCategorizeObjects) {
        int total = 0;
        for (int i = 0; i < mChoices.count (); ++i) {
            shMenuChoice *choice = mChoices.get (i);
            if (choice->mValue.mPtr == 0)
                continue;
            total += ((shObject *) choice->mValue.mPtr)->getMass ();
        }
        sprintf (wgtbuf, "%5d weight total", total);
    }
    /* Indicator, optional weight and line number information. */
    static char buf[60];
    sprintf (buf, "%-8s   %s", indicator, wgtbuf);
    if (lineinfo)
        sprintf (buf + strlen (buf), "   displaying lines %d - %d of %d   ",
                 mOffset + 1, curpos + 1, mChoices.count () + 1);
    return buf;
}


shTextViewer::shTextViewer (const char *fname)
{   /* Try opening target file. */
    assert (fname);
    mFree = true;
    mLines = NULL;
    mNumLines = 0;
    mMaxWidth = 0;
    char path[PRIME_PATH_LENGTH];
    snprintf (path, sizeof (path)-1, "%s/%s", DATADIR, fname);
    FILE *text = fopen (path, "r");
    if (!text) return;
    /* Count lines and check max width. */
    char *buf = GetBuf ();
    while (fgets (buf, SHBUFLEN, text)) {
        mMaxWidth = maxi (mMaxWidth, strlen (buf));
        ++mNumLines;
    }
    ++mNumLines; /* For adding " --End-- " at the bottom. */
    mMaxWidth = maxi (mMaxWidth, 9 /* --End-- */); /* Ok, this is paranoidal. */
    ++mMaxWidth; /* Compensate for \0. */
    /* Copy file to memory. */
    mLines = (char *) calloc (mNumLines * mMaxWidth, sizeof (char));
    rewind (text);
    int line = 0;
    while (fgets (mLines + line * mMaxWidth, mMaxWidth, text)) {
        ++line;
    }
    strcpy (mLines + line * mMaxWidth, " --End-- ");
    /* Done! Now clean up. */
    fclose (text);
}

shTextViewer::shTextViewer (char *lines, int num, int max)
{   /* Import text from some other procedure. */
    mLines = lines;
    mNumLines = num;
    mMaxWidth = max;
    mFree = false;
}

shTextViewer::~shTextViewer (void)
{
    if (mFree)  free (mLines);
}

void /* Color change sequences: @X where X stands for a color code. */
shTextViewer::print (int winline, int fileline)
{
    int control = 0, idx = 0;
    char *line = mLines + fileline * mMaxWidth;
    I->winPrint (shInterface::kTemp, winline, 0, ""); /* Just to move cursor. */
    while (line[idx]) { /* Parse line. */
        if (control and !isupper (line[idx])) {
            I->winPutchar (shInterface::kTemp, '@');
            control = 0; /* Avoid choking on our email addresses. */
        }
        if (control) {
            I->setWinColor (shInterface::kTemp, shColor (line[idx] - 'A'));
            control = 0;
        } else if (line[idx] == '@') {
            control = 1;
        } else {
            I->winPutchar (shInterface::kTemp, line[idx]);
        }
        ++idx;
    }
}
     /* Displays loaded file to the screen. This procedure */
void /* should have no reason to modify member variables.  */
shTextViewer::show (bool bottom)
{
    int height = I->getMaxLines () - 1;
    int first = 0;
    if (bottom)  first = maxi (mNumLines - height, 0);
    I->newWin (shInterface::kTemp);

    shInterface::SpecialKey sp;
    while (sp != shInterface::kEscape) {
        /* Show content. */
        I->clearWin (shInterface::kTemp);
        I->setWinColor (shInterface::kTemp, kGray);
        int lines_drawn = mini (mNumLines - first, height);
        for (int i = first; i < first + lines_drawn; ++i) {
            print (i - first, i);
        }
        /* Show help at bottom. */
        I->setWinColor (shInterface::kTemp, kWhite);
        I->winPrint (shInterface::kTemp, height, 0,
"    SPACE  ENTER  ESCAPE          ARROWS  PAGE UP  PAGE DOWN");
        I->setWinColor (shInterface::kTemp, kGray);
        I->winPrint (shInterface::kTemp, height, 9, ",");
        I->winPrint (shInterface::kTemp, height, 16, ",");
        I->winPrint (shInterface::kTemp, height, 24, " - exit");
        I->winPrint (shInterface::kTemp, height, 40, ",");
        I->winPrint (shInterface::kTemp, height, 49, ",");
        I->winPrint (shInterface::kTemp, height, 60, " - navigation");
        I->refreshWin (shInterface::kTemp);
        I->getSpecialChar (&sp);
        /* Navigation. */
        switch (sp) {
        case shInterface::kHome:
            first = 0;
            break;
        case shInterface::kEnd:
            first = mNumLines - height;
            break;
        case shInterface::kUpArrow:
            if (first > 0) --first;
            break;
        case shInterface::kDownArrow:
            if (first + height < mNumLines) ++first;
            break;
        case shInterface::kPgUp: case shInterface::kLeftArrow:
            first -= height / 2;
            if (first < 0) first = 0;
            break;
        case shInterface::kPgDn: case shInterface::kRightArrow:
            first += height / 2;
            if (first + height > mNumLines)  first = mNumLines - height;
            break;
        case shInterface::kEnter: case shInterface::kSpace:
            sp = shInterface::kEscape;
            break;
        default: break;
        }
    }
    I->delWin (shInterface::kTemp);
}


void
shMenu::accumulateResults ()
{
    const int map_end = 64; /* column map ends */
    extern bool mapOn;
    mapOn = false; /* Dim the map in graphical modes while menu is active. */
    shInterface::Window menuwin = shInterface::kMenu;

    int helplines;
    /* Merely to extract number of help lines needed. */
    free (prepareHelp (&helplines));

    /* Two additional lines besides all the choices are the menu title
       and --End-- or --More-- at the bottom. */
    mItemHeight = mini (mHeight - helplines, mChoices.count () + 2);
    int width = 10;
    int gap = 0; /* Used to position the menu. */
    for (int i = 0; i < mChoices.count (); ++i) {
        width = maxi (width, strlen (mChoices.get (i)->mText) + 1);
    }
    if (!(mFlags & kNoPick)) width += 10; /* Adjust for "( ) x - " prompts. */
    /* Main header might be still longer. */
    width = maxi (width, strlen (mPrompt) + 2);
    if (mFlags & kCategorizeObjects) { /* Do not center item lists. */
        /* Determine whether window can leave sidebar unobscured. */
        if (width <= map_end) { /* Yes! */
            width = map_end;
        } else {                /* No, so hide it whole. */
            width = mWidth;
        }
    } else {
        width = mini (mWidth, width);
        if (width > map_end) { /* Would obscure side bar window? */
            width = mWidth;    /* Then cover it whole. */
        } else { /* Place small gap between sidebar and menu if possible. */
            gap = mini (10, (map_end - width) / 2);
        }
    }

    int begin = maxi (0, map_end - width) - gap;
    I->newWin (menuwin);
    I->moveWin (menuwin, begin, 0, begin + width, mItemHeight);
    if (helplines) {
        I->newWin (shInterface::kMenuHelp);
        I->moveWin (shInterface::kMenuHelp, 0, mHeight - helplines, 80, mHeight);
        showHelp ();
    }

    /* -2 lines to make space for header and --End-- or similar. */
    mLast = mini (mOffset + mItemHeight - 2, mChoices.count ());
    while (1) { /* Menu loop. */
        I->clearWin (menuwin);
        /* Menu header: */
        I->setWinColor (menuwin, kWhite, kBlack);
        I->winOutXY (menuwin, 0, 0, mPrompt);

        I->setWinColor (menuwin, kGray, kBlack);
        int i;
        for (i = mOffset; i < mLast; ++i) {
            /* Draw each menu line. */
            char buf[100];
            shMenuChoice *item = mChoices.get (i);

            if (item->mLetter >= 0 and (kCategorizeObjects & mFlags)
                and ((shObject *) item->mValue.mPtr)->isKnownRadioactive ())
            {
                I->setWinColor (menuwin, kGreen, kBlack);
            }
            if (-2 == item->mLetter) { /* This is a pretty delimiter. */
                int len = strlen (item->mText);
                int j = (width - len) / 2;
                char *spaces = GetBuf ();
                for (int i = 0; i < j; ++i) {
                    spaces[i] = ' ';
                }
                spaces[j] = '\0';
                snprintf (buf, 100, "%s%s%s", spaces, item->mText, spaces);
            } else if (-1 == item->mLetter) { /* This is a header entry. */
                if (kCategorizeObjects & mFlags and mFlags & kMultiPick) {
                    /* Get category header. */
                    char part[50];
                    snprintf (part, 50, " %s ", item->mText);
                    part[49] = 0;
                    char *gap = strstr (part, "  "); /* Find gap. */
                    gap[1] = 0; /* Truncate. */
                    I->setWinColor (menuwin, kBlack, kGray);
                    I->winOutXY (menuwin, 0, 1 + i - mOffset, part);
                    /* Get (toggle all with X) part. */
                    char *p2 = strstr (item->mText, "(t");
                    int len = strlen (p2);
                    snprintf (part, len-2, "%s", item->mText);
                    I->setWinColor (menuwin, kBlue, kBlack);
                    I->winOutXY (menuwin, width-22, 1 + i - mOffset, p2);
                    /* The toggle key should stand out. */
                    snprintf (buf, len - 1, "%c", p2[len - 2]);
                    I->winGoToYX (menuwin, 1 + i - mOffset, width-22+len-2);
                    I->setWinColor (menuwin, kWhite, kBlack);
                    I->winPutchar (menuwin, p2[len - 2]);
                    buf[0] = 0; /* Printing is done. */
                } else {
                    I->setWinColor (menuwin, kBlack, kGray);
                    snprintf (buf, 100, " %s ", item->mText);
                }
            } else if (mHelpMode) {
                snprintf (buf, 100, "    %c - %s", item->mAltLetter, item->mText);
            } else if (mFlags & kNoPick) {
                if (mHelpHandlerP or mHelpHandlerI) {
                    if (' ' == item->mLetter) {
                        snprintf (buf, 100, "        %s", item->mText);
                    } else {
                        snprintf (buf, 100, "    %c - %s", item->mLetter, item->mText);
                    }
                } else {
                    if (' ' == item->mLetter) {
                        snprintf (buf, 100, "%s", item->mText);
                    } else {
                        snprintf (buf, 100, "%c - %s", item->mLetter, item->mText);
                    }
                }
            } else {
                if (' ' == item->mLetter) {
                    snprintf (buf, 100, "        %s", item->mText);
                } else if (mFlags & kShowCount) {
                    if (item->mSelected) {
                        snprintf (buf, 100, "(%d) %c - %s",
                            item->mSelected, item->mLetter, item->mText);
                    } else {
                        snprintf (buf, 100, "( ) %c - %s",
                            item->mLetter, item->mText);
                    }
                } else {
                    snprintf (buf, 100, "(%c) %c - %s",
                              0 == item->mSelected ? ' ' :
                              item->mCount == item->mSelected ? 'X' : '#',
                              item->mLetter, item->mText);
                }
            }
            I->winOutXY (menuwin, 0, 1 + i - mOffset, "%s", buf);
            I->setWinColor (menuwin, kGray, kBlack);
        }
        I->winOutXY (menuwin, 0, 1 + i - mOffset, bottomLine (i));

        while (1) {
            I->refreshWin (menuwin);
            I->refreshWin (shInterface::kMenuHelp);
            if (helplines)  showHelp ();
            shInterface::SpecialKey spk;
            int key = I->getSpecialChar (&spk);

            if (27 == key or 13 == key or 10 == key or ' ' == key) { /* done */
                mDone = 1; break;
            } else if ('\t' == key) { /* invoke help */
                interpretKey (0, shInterface::kHelp); break;
            } else if (spk == shInterface::kBackSpace) {
                interpretKey (0, shInterface::kDrop); break;
            } else if (spk == shInterface::kHome) {
                interpretKey (0, shInterface::kMoveNW); break;
            } else if (spk == shInterface::kEnd) {
                interpretKey (0, shInterface::kMoveSW); break;
            } else if (spk == shInterface::kUpArrow) {
                interpretKey (0, shInterface::kMoveUp); break;
            } else if (spk == shInterface::kDownArrow) {
                interpretKey (0, shInterface::kMoveDown); break;
            } else if (spk == shInterface::kLeftArrow or
                       spk == shInterface::kPgUp)
            {
                interpretKey (0, shInterface::kMoveNE); break;
            } else if (spk == shInterface::kRightArrow or
                       spk == shInterface::kPgDn)
            {
                interpretKey (0, shInterface::kMoveSE); break;
            } else {
                if (interpretKey (key)) break;
            }
            if (mDone) break;
        }
        if (mDone) break;
    }
    /* Clean up. */
    I->delWin (menuwin);
    if (helplines)  I->delWin (shInterface::kMenuHelp);
    mapOn = true;
    I->drawScreen ();
}
