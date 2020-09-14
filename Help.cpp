#include "Interface.h"
#include <ctype.h>

const char *
getCodename (const char *desc)
{   /* Create code name from string desc. */
    char *buf = GetBuf ();
    char *bufptr = buf;
    size_t len = strlen (desc);
    for (unsigned int i = 0; i < len; ++i)
        if (!isspace (desc[i])) {
            *bufptr = tolower (desc[i]);
            ++bufptr;
        }
    /* Used for pointing at description files for various skills. */
    strcpy (bufptr, ".txt");
    return buf;
}

static const int LINEMAX = 100;

/* Keybindings help window is in two columns format.  If desc1 is NULL the left
   column will be empty.  If desc2 is null the right column will be empty. */
static void
twoCommands (char *buf, shInterface::Command cmd1, const char *desc1,
                        shInterface::Command cmd2, const char *desc2)
{
    if (desc1 and desc2)
        snprintf (buf, LINEMAX, " @F%6s@H  %-30s @F%6s@H  %-30s",
            I->getKeyForCommand (cmd1), desc1,
            I->getKeyForCommand (cmd2), desc2);
    else if (desc1)
        snprintf (buf, LINEMAX, " @F%6s@H  %-30s",
            I->getKeyForCommand (cmd1), desc1);
    else if (desc2)
        snprintf (buf, LINEMAX, "         %-30s @F%6s@H  %-30s",
            "", I->getKeyForCommand (cmd2), desc2);
}

static void
processVector (shVector <const char *> *v, char *buf, int *l)
{
    for (int i = 0; i < v->count (); ++i) {
        if (i > 0 and i == v->count () - 1) { /* Last rest key. */
            *l += snprintf (buf + *l, LINEMAX - *l, "@H or @F%s", v->get (i));
        } else if (i) { /* Neither the first nor last rest key. */
            *l += snprintf (buf + *l, LINEMAX - *l, "@H, @F%s", v->get (i));
        } else { /* The first rest key. */
            *l += snprintf (buf + *l, LINEMAX - *l, "%s", v->get (i));
        }
    }
}

void
shInterface::showHelp ()
{
    shVector<char *> hlp;
    char *buf = GetBuf (), *buf0 = GetBuf ();
    static const char basic_movement[][LINEMAX] =
    {
"         @CMOVEMENT@H",
"",
"   @F7 8 9  home     up arrow    page up@H   Use the numeric keypad or arrow keys",
"    \\|/            \\   |   /             for movement.  Move into a monster",
"   @F4@H-@-@F6@H  @Fleft arrow@H - @ - @Fright arrow@H   to fight it in melee.",
"    /|\\            /   |   \\             Use @F_@H and @F_@H to select up and down",
"   @F1 2 3  delete  down arrow  page down@H  directions.  Climb stairs with these.",
""
    };
    const int movelin = sizeof (basic_movement) / sizeof (char [LINEMAX]);
    for (int i = 0; i < movelin; ++i) {
        hlp.add (strdup (basic_movement[i]));
    }
    /* Replace the two underscores. */
    hlp.get (5)[47] = getKeyForCommand (kMoveUp)[0];
    hlp.get (5)[57] = getKeyForCommand (kMoveDown)[0];

    static const char vi_keys[][LINEMAX] =
    {
"  @Fy k u  Y K U@H   Nethack/vi movement keys are also supported by selected",
"   \\|/    \\|/    keymap.  Shifted @FYUHJKLBN@H execute run with chosen direction.",
"  @Fh@H-@-@Fl@H  @FH@H-@-@FL@H",
"   /|\\    /|\\    ",
"  @Fb j n  B J N@H   ",
""
    };
    const int vilin = sizeof (vi_keys) / sizeof (char [LINEMAX]);
    static const char fire_vi[vilin][LINEMAX] =
    {
"",
"",
"",
"@FControl@H + @FYUHJKLBN@H shoots wielded weapon in the direction.",
"",
""
    };
    if (has_vi_keys ()) {
        for (int i = 0; i < vilin; ++i) {
            char *str = (char *) malloc (LINEMAX);
            strcpy (str, vi_keys[i]);
            hlp.add (str);
        }
        if (has_vi_fire_keys ()) {
            for (int i = 0; i < vilin; ++i) {
                char *str = hlp.get (hlp.count () - vilin + i);
                strcpy (&str[strlen (str)], fire_vi[i]);
            }
        }
    }

    shVector <const char *> *v;
    snprintf (buf, LINEMAX, "   Rest with: @F");
    int l = strlen (buf);
    v = getKeysForCommand (kRest);
    processVector (v, buf, &l);
    delete v;
    snprintf (buf + l, LINEMAX - l, "@H.   Glide with: @F");
    l = strlen (buf);
    v = getKeysForCommand (kGlide);
    processVector (v, buf, &l);
    delete v;
    snprintf (buf + l, LINEMAX - l, "@H.");
    buf[LINEMAX - 1] = 0;
    hlp.add (strdup (buf));
    hlp.add (strdup ("   You can instruct your character to continuously move in a direction"));
    hlp.add (strdup ("   until something interesting is encountered or the movement is disturbed."));
    hlp.add (strdup ("   This is called \"gliding\".  Press glide key followed by a direction to glide."));
    hlp.add (strdup (""));

    static const char meta_com[][LINEMAX] =
    {
"       @CMETA COMMANDS@H",
"",
"     @FF3@H  Access Necklace of the Eye options (only in graphical mode)",
"@Fleft alt@H Hold to show ASCII map (only in graphical mode)"
    };
    const int metalin = sizeof (meta_com) / sizeof (char [LINEMAX]);
    for (int i = 0; i < metalin; ++i) {
        hlp.add (strdup (meta_com[i]));
    }
    snprintf (buf, LINEMAX, " @F%6s@H  Access game menu to change options, save game or quit",
        getKeyForCommand (kMainMenu));
    hlp.add (strdup (buf));
    snprintf (buf, LINEMAX, " @F%6s@H  Get help on game topics",
        getKeyForCommand (kHelp));
    hlp.add (strdup (buf));
    snprintf (buf, LINEMAX, " @F%6s@H  Use Bastard Operator From Hell powers (only in BOFH mode)",
        getKeyForCommand (kBOFHPower));
    hlp.add (strdup (buf));
    hlp.add (strdup (""));

    hlp.add (strdup ("       @CFIGHTING@H                               @CINVENTORY@H"));
    hlp.add (strdup (""));
    twoCommands (buf, kFireWeapon, "Fire wielded weapon",
                      kListInventory, "Browse your inventory");
    hlp.add (strdup (buf));
    twoCommands (buf, kThrow, "Throw an object",
                      kPickup, "Pick up items from the floor");
    hlp.add (strdup (buf));
    twoCommands (buf, kZapRayGun, "Zap a ray gun",
                      kPickUpAll, "Pick up whole stack of items");
    hlp.add (strdup (buf));
    twoCommands (buf, kKick, "Kick something",
                      kDrop, "Drop an item");
    hlp.add (strdup (buf));
    twoCommands (buf, kMutantPower, "Use a mutant power",
                      kDropMany, "Drop several items");
    hlp.add (strdup (buf));
    twoCommands (buf, kNoCommand, NULL,
                      kExamine, "Examine object closely");
    hlp.add (strdup (buf));
    twoCommands (buf, kNoCommand, NULL,
                      kWield, "Hold something in hands (wield)");
    snprintf (buf0, LINEMAX, "       @CENVIRONMENT@H                      %s",
              buf + 40);
    hlp.add (strdup (buf0));
    twoCommands (buf, kNoCommand, NULL,
                      kWear, "Wear armor, install implants");
    hlp.add (strdup (buf));
    twoCommands (buf, kOpen, "Open a door, operate droids",
                      kTakeOff, "Remove armor, weapons, implants");
    hlp.add (strdup (buf));
    twoCommands (buf, kClose, "Close a door",
                      kUse, "Apply or activate (use)");
    hlp.add (strdup (buf));
    twoCommands (buf, kLook, "Look at a feature or monster",
                      kQuaff, "Drink object contents (quaff)");
    hlp.add (strdup (buf));
    twoCommands (buf, kPay, "Pay for an item or service",
                      kExecute, "Execute (run object's program)");
    hlp.add (strdup (buf));
    twoCommands (buf, kNoCommand, NULL,
                      kName, "Name an object or its class");
    hlp.add (strdup (buf));
    twoCommands (buf, kNoCommand, NULL,
                      kAdjust, "Adjust shortcut letter");
    snprintf (buf0, LINEMAX, "       @CCHARACTER@H                        %s",
              buf + 40);
    hlp.add (strdup (buf0));
    twoCommands (buf, kNoCommand, NULL,
                      kToggleAutopickup, "Toggle autopickup option");
    hlp.add (strdup (buf));
    twoCommands (buf, kHistory, "Show log message history",
                      kNoCommand, NULL);
    hlp.add (strdup (buf));
    twoCommands (buf, kDiscoveries, "Show recognized item types",
                      kNoCommand, NULL);
    hlp.add (strdup (buf));
    twoCommands (buf, kEditSkills, "Open skill screen",
                      kNoCommand, NULL);
    hlp.add (strdup (buf));
    hlp.add (strdup (""));

    char *lines = (char *) calloc (hlp.count () * LINEMAX, sizeof (char));
    for (int i = 0; i < hlp.count (); ++i)
        strcpy (&lines[i * LINEMAX], hlp.get (i));

    shTextViewer *viewer = new shTextViewer (lines, hlp.count (), LINEMAX);
    viewer->show ();
    delete viewer;

    for (int i = 0; i < hlp.count (); ++i)
        free (hlp.get (i));
}
