#ifndef POINTERSTORE_H
#define POINTERSTORE_H

#include <stdio.h>

namespace sav {

struct _ptrstore_internal;

struct PointerStore
{
    PointerStore ();
    ~PointerStore ();
    void set_file (FILE *);
    void save_pointer (void *);
    bool load_pointer (void **);
    void save_target (void *);
    bool load_target (void *);

private:
    _ptrstore_internal *m;
};

enum SaveFileMagicNumber
{
    MAGIC_PTR = 1,
    MAGIC_WARP = 11
};

void save_magic (FILE *, long);
bool load_magic (FILE *, long);

}
#endif
