#include "Global.h"
#include "Interface.h"
#include <dirent.h>

struct shFlags Flags;

static const struct shOption {
    const char *mName;
    int *mValue;
    int mDefault;
} Options[] = {
    {"fadelog", &Flags.mFadeLog, 0 },
    {"show line of sight", &Flags.mShowLOS, 0 },
    {"autopickup", &Flags.mAutopickup, 0 },
    {NULL, NULL, 0 }
}, Autopickup[] = {
    {": implants", &Flags.mAutopickupTypes[kImplant], 1 },
    {"? floppy disks", &Flags.mAutopickupTypes[kFloppyDisk], 1 },
    {"! canisters", &Flags.mAutopickupTypes[kCanister], 1 },
    {"( tools", &Flags.mAutopickupTypes[kTool], 0 },
    {"] armor", &Flags.mAutopickupTypes[kArmor], 0 },
    {") weapons", &Flags.mAutopickupTypes[kWeapon], 0 },
    {"= ammunition", &Flags.mAutopickupTypes[kProjectile], 1 },
    {"& other", &Flags.mAutopickupTypes[kOther], 0 },
    {"/ ray guns", &Flags.mAutopickupTypes[kRayGun], 1 },
    {"* energy cells", &Flags.mAutopickupTypes[kEnergyCell], 1 },
    {NULL, NULL, 0 }
};

void
miscOptions (const shOption opts[], const char *header)
{
    int i;
    shMenu *menu = I->newMenu (header, shMenu::kMultiPick);
    char letter = 'a';

    for (i = 0; opts[i].mName; ++i) {
        if (opts[i].mValue) {
            menu->addIntItem (letter++, opts[i].mName, i, 1, *opts[i].mValue);
        } else {
            menu->addHeader (opts[i].mName);
        }
    }

    for (i = 0; opts[i].mName; ++i) {
        if (opts[i].mValue)
            *opts[i].mValue = 0;
    }
    while (menu->getIntResult (&i)) {
        *opts[i].mValue = 1;
    }
    delete menu;
}

void
shInterface::editOptions ()
{
    shMenu *menu = newMenu ("Options", 0);
    menu->addIntItem ('a', "Switch keymap", 1);
    menu->addIntItem ('b', "Miscellaneous options", 2);
    menu->addIntItem ('c', "Autopickup types", 3);

    int i;
    menu->getIntResult (&i);
    delete menu;
    switch (i) {
        case 1:
            keymapChoice ();
            break;
        case 2:
            miscOptions (Options, "Miscellaneous");
            break;
        case 3:
            miscOptions (Autopickup, "Autopickup types");
            break;
    }
}

enum shConfigOption
{
    kKeySet,
    kFadeLog,
    kShowLOS,
    kAutopickup,
    kAutopickupTypes
};

static const char *OptionNames[] =
{
    "keymap",
    "fadelog",
    "showsight",
    "autopickup",
    "autopickup_types"
};
const int n_options = sizeof (OptionNames) / sizeof (char *);

void
defaultOptions ()
{
    for (int i = 0; Options[i].mName; i++) {
        if (Options[i].mValue) {
            *Options[i].mValue = Options[i].mDefault;
        }
    }
    Flags.mKeySet[0] = '\0'; /* No keymap loaded. Prompt user. */
}

void
shInterface::loadOptions ()
{
    char confname[PRIME_PATH_LENGTH];
    snprintf (&confname[0], sizeof (confname) - 1, "%s/config.txt", UserDir);
    FILE *config = fopen (confname, "r");
    if (config == NULL) {
        debug.log ("Could not read configuration file %s. Using defaults.", confname);
        defaultOptions ();
        return;
    } /* Ok, read options but use defaults where not specified. */
    defaultOptions ();
    /* Assume unspecified pickup types to be not wanted. */
    for (int i = kMoney; i <= kEnergyCell; ++i) {
        Flags.mAutopickupTypes[i] = 0;
    }
    char buf[200], key[31], value[31];
    while (fgets (&buf[0], sizeof (buf)-1, config)) {
        int scan = sscanf (&buf[0], "%30s%*s%30s", &key[0], &value[0]);
        if (scan != 2) {
            debug.log ("Could not parse configuration line:");
            debug.log ("%s", buf);
            continue;
        }
        int n = 0; /* Find key. */
        while (n < n_options and strcmp (OptionNames[n], key))
            ++n;
        if (n == n_options) {
            debug.log ("Unrecognized option name '%s' on line:", key);
            debug.log ("%s", buf);
            continue;
        }
        int numval = 0; /* For numeric values. */
        if (n >= kFadeLog and n <= kAutopickup) {
            if (!sscanf (&value[0], "%d", &numval)) {
                debug.log ("Incorrect integral value '%s' on line:", value);
                debug.log ("%s", buf);
                continue;
            }
        }
        switch ((shConfigOption) n) {
            case kKeySet:
                strncpy (&Flags.mKeySet[0], value, sizeof (Flags.mKeySet)-1);
                readKeybindings (value);
                break;
            case kFadeLog: /* Booleans. */
            case kShowLOS:
            case kAutopickup:
                *(Options[n - kFadeLog].mValue) = numval != 0;
                break;
            case kAutopickupTypes:
                debug.log ("Autopickup: %s", value);
                for (int i = 0, len = strlen (key); i < len; ++i) {
                    int index;
                    for (int j = kImplant; j <= kEnergyCell; ++j) {
                        index = j - kImplant;
                        if (value[i] == Autopickup[index].mName[0]) {
                            *(Autopickup[index].mValue) = 1;
                            break;
                        }
                    }
                    if (index > kEnergyCell) {
                        debug.log ("Unrecognized autopickup character '%c'", value[i]);
                    }
                }
                break;
        }
    }
    fclose (config);
}

void
shInterface::saveOptions ()
{
    char confname[PRIME_PATH_LENGTH];
    snprintf (&confname[0], sizeof (confname) - 1, "%s/config.txt", UserDir);
    FILE *config = fopen (confname, "w");
    if (config == NULL) {
        pageLog ();
        p ("Could not write configuration file %s.", confname);
        p ("Changes to options are lost.");
        pause ();
        return;
    }
    for (int i = kKeySet; i <= kAutopickupTypes; ++i) {
        switch ((shConfigOption) i) {
            case kKeySet:
                fprintf (config, "%s = %s\n", OptionNames[i], &Flags.mKeySet[0]);
                break;
            case kFadeLog: /* Booleans. */
            case kShowLOS:
            case kAutopickup:
                fprintf (config, "%s = %d\n", OptionNames[i],
                         *(Options[i - kFadeLog].mValue));
                break;
            case kAutopickupTypes: {
                char types[20];
                int n_types = 0;
                for (int j = kImplant; j <= kEnergyCell; ++j) {
                    int index = j - kImplant;
                    if (*(Autopickup[index].mValue)) {
                        types[n_types++] = Autopickup[index].mName[0];
                    }
                }
                if (n_types == 0) break; /* Not set. */
                types[n_types] = '\0'; /* Terminate autopickup string. */
                fprintf (config, "%s = %s\n", OptionNames[i], &types[0]);
                break;
            }
        }
    }
    fclose (config);
}

void
shInterface::keymapChoice ()
{
    shMenu *keymaps = newMenu ("Choose keymap:", 0);
    DIR *dir = opendir (DATADIR);
    if (dir == NULL) {
        p ("Error opening directory '%s'.", DATADIR);
        return;
    }
    dirent *data;
    char letter = 'a';
    while ((data = readdir (dir))) {
        char *dot = strrchr (data->d_name, '.');
        if (dot == NULL) continue; /* File name must be *.cfg. */
        if (strncmp (dot, ".cfg", 4) != 0) continue;
        /* Save names in buffers. */
        char *buf = GetBuf ();
        strcpy (buf, &data->d_name[0]);
        /* Try to open file beforehand. */
        char path[PRIME_PATH_LENGTH];
        snprintf (&path[0], sizeof (path)-1, "%s/%s", DATADIR, data->d_name);
        FILE *candidate = fopen (path, "r");
        /* Do not offer inaccessible keymaps. */
        if (candidate == NULL) continue;
        /* Read caption from file. */
        char title[51];
        /* TODO: skip this file (and log as corrupt) if title is not present. */
        (void) fscanf (candidate, "%*2c%50[^\n]", &title[0]);
        keymaps->addPtrItem (letter++, &title[0], buf);
        fclose (candidate);
    }
    char *fname = NULL;
    while (!keymaps->getPtrResult ((const void **) &fname)) {
        keymaps->dropResults ();
    }
    delete keymaps;
    if (fname != NULL) {
        readKeybindings (fname);
    }
}
