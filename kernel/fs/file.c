// ===========================================================================
// file.c - Table globale des fichiers ouverts et opérations read/write/close
// ===========================================================================

#include <stddef.h>
#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/fs/file.h"
#include "kernel/fs/pipe.h"
#include "kernel/fs/inode.h"
#include "kernel/fs/log.h"
#include "kernel/mem/spinlock.h"
#include "kernel/io/console.h"

struct ftable ftable;

void file_init(void)
{
    spinlock_init(&ftable.lock, "ftable");
    printf("file_init: table de %d fichiers ouverts\n", NFILE);
}

// ---------------------------------------------------------------------------
// file_alloc - Allouer une entrée libre dans la table globale
// ---------------------------------------------------------------------------
struct file *file_alloc(void)
{
    struct file *f;

    spinlock_acquire(&ftable.lock);
    for (f = ftable.file; f < ftable.file + NFILE; f++) {
        if (f->ref == 0) {
            f->ref = 1;
            spinlock_release(&ftable.lock);
            return f;
        }
    }
    spinlock_release(&ftable.lock);
    printf("file_alloc: table pleine\n");
    return NULL;
}

// ---------------------------------------------------------------------------
// file_dup - Dupliquer une référence sur un fichier (ex: héritage fork)
// ---------------------------------------------------------------------------
struct file *file_dup(struct file *f)
{
    spinlock_acquire(&ftable.lock);
    if (f->ref < 1) {
        printf("file_dup: ref invalide\n");
        spinlock_release(&ftable.lock);
        return NULL;
    }
    f->ref++;
    spinlock_release(&ftable.lock);
    return f;
}

// ---------------------------------------------------------------------------
// file_close - Décrémenter la référence ; libérer si plus personne ne l'utilise
// ---------------------------------------------------------------------------
void file_close(struct file *f)
{
    struct file ff;

    spinlock_acquire(&ftable.lock);
    if (f->ref < 1) {
        printf("file_close: ref invalide\n");
        spinlock_release(&ftable.lock);
        return;
    }

    if (--f->ref > 0) {
        spinlock_release(&ftable.lock);
        return;
    }

    // Copier et réinitialiser avant de relâcher le lock
    ff = *f;
    f->ref  = 0;
    f->type = FD_NONE;
    spinlock_release(&ftable.lock);

    if (ff.type == FD_PIPE) {
        pipe_close(ff.pipe, ff.writable);
    } else if (ff.type == FD_INODE || ff.type == FD_DEVICE) {
        log_begin_op();
        inode_put(ff.ip);
        log_end_op();
    }
}

// ---------------------------------------------------------------------------
// file_read - Lire n octets depuis un fichier vers l'adresse addr
// ---------------------------------------------------------------------------
int file_read(struct file *f, uint64 addr, int n)
{
    if (!f->readable) return -1;

    if (f->type == FD_PIPE) {
        return pipe_read(f->pipe, addr, n);
    }

    if (f->type == FD_INODE || f->type == FD_DEVICE) {
        inode_lock(f->ip);
        int r = inode_read(f->ip, 1, addr, f->off, (uint32)n);
        if (r > 0) f->off += (uint32)r;
        inode_unlock(f->ip);
        return r;
    }

    return -1;
}

// ---------------------------------------------------------------------------
// file_write - Écrire n octets depuis addr vers un fichier
// ---------------------------------------------------------------------------
int file_write(struct file *f, uint64 addr, int n)
{
    if (!f->writable) return -1;

    if (f->type == FD_PIPE) {
        return pipe_write(f->pipe, addr, n);
    }

    if (f->type == FD_INODE) {
        // Découper en petites transactions pour ne pas dépasser MAXOPBLOCKS
        int max = ((MAXOPBLOCKS - 1 - 1 - 2) / 2) * BSIZE;
        int i = 0;
        while (i < n) {
            int n1 = n - i;
            if (n1 > max) n1 = max;

            log_begin_op();
            inode_lock(f->ip);
            int r = inode_write(f->ip, 1, addr + (uint32)i, f->off, (uint32)n1);
            if (r > 0) f->off += (uint32)r;
            inode_unlock(f->ip);
            log_end_op();

            if (r != n1) break;
            i += r;
        }
        return i == n ? n : -1;
    }

    return -1;
}

// ---------------------------------------------------------------------------
// file_stat - Remplir une structure stat depuis un descripteur
// ---------------------------------------------------------------------------
int file_stat(struct file *f, uint64 addr)
{
    if (f->type == FD_INODE || f->type == FD_DEVICE) {
        inode_lock(f->ip);
        struct stat *st = (struct stat *)addr;
        inode_stat(f->ip, st);
        inode_unlock(f->ip);
        return 0;
    }
    return -1;
}
