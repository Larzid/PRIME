#include <stdio.h>

#include "PointerStore.h"
#include "Util.h"

typedef shVector <void **> TargetList;

struct sav::_ptrstore_internal
{
    FILE *file;
    shVector <void *> *ptr_list;          /* maps pointers to integer identifiers */
    shVector <TargetList *> *upd_list;    /* keeps track of pointers for linking */
};

namespace sav {
    const int NULL_POINTER_INDEX = -1;
}

sav::PointerStore::PointerStore ()
{
    m = new _ptrstore_internal;
    m->file = NULL;
    m->ptr_list = new shVector <void *>;
    m->upd_list = new shVector <TargetList *>;
}

sav::PointerStore::~PointerStore ()
{
    delete m->ptr_list;
    delete m->upd_list;
    delete m;
}

void
sav::PointerStore::set_file (FILE *file)
{
    m->file = file;
}

void
sav::PointerStore::save_pointer (void *ptr)
{
    save_magic (m->file, MAGIC_PTR);

    if (ptr == NULL) {
        fprintf (m->file, " %d", NULL_POINTER_INDEX);
        return;
    }

    int id = m->ptr_list->find (ptr);

    if (id == -1)
        id = m->ptr_list->add (ptr);

    fprintf (m->file, " %d", id);
    debug.log ("save_pointer %3d %p", id, ptr);
}

bool
sav::PointerStore::load_pointer (void **ptr)
{
    if (!load_magic (m->file, MAGIC_PTR))
        return false;

    int id;
    if (fscanf (m->file, "%d", &id) != 1)
        return false;

    if (id == NULL_POINTER_INDEX) {
        *ptr = NULL;
        return true;
    }

    if (id < m->ptr_list->count ()) {
        *ptr = m->ptr_list->get (id); /* might set to NULL */

        if (*ptr != NULL) {
            debug.log ("load_pointer %3d %p <- %p %s", id, *ptr, ptr);
            return true;
        }
    }

    debug.log ("load_pointer %3d 0x???????? <- %p %s", id, ptr);
    if (id < m->upd_list->count ()) {
        TargetList *tgt = m->upd_list->get (id);
        if (tgt != NULL) {
            tgt->add (ptr);
            return true;
        }
    }

    TargetList *tgt = new TargetList;
    tgt->add (ptr);
    m->upd_list->set (id, tgt);
    return true;
}

bool
sav::PointerStore::load_target (void *ptr)
{
    if (!sav::load_magic (m->file, sav::MAGIC_PTR))
        return false;

    int id;
    if (fscanf (m->file, "%d", &id) != 1)
        return false;

    m->ptr_list->set (id, ptr);

    debug.log ("load_target  %3d %p", id, ptr);

    if (id < m->upd_list->count ()) {
        TargetList *tgt = m->upd_list->get (id);
        if (tgt == NULL)
            return true;

        for (int i = 0; i < tgt->count (); ++i) {
            *tgt->get (i) = ptr;
            debug.log ("                %p <- %p", ptr, tgt->get (i));
        }
        m->upd_list->set (id, NULL);
        delete tgt;
        return true;
    }
    return true;
}

void
sav::PointerStore::save_target (void *ptr)
{
    save_pointer (ptr);
}

void
sav::save_magic (FILE *file, long key)
{
    fprintf (file, " -0 %ld", key);
}

bool
sav::load_magic (FILE *file, long key)
{
    char str[10];
    fscanf (file, "%3s", str);
    if (strcmp(str, "-0") != 0)
        return false;

    long read_key;
    if (fscanf (file, "%ld", &read_key) != 1)
        return false;

    return read_key == key;
}
