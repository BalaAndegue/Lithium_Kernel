// ===========================================================================
// file_system.c - Initialisation complète du système de fichiers
// ===========================================================================

#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/fs/file_system.h"
#include "kernel/fs/buffer.h"
#include "kernel/fs/log.h"
#include "kernel/fs/inode.h"
#include "kernel/fs/file.h"
#include "kernel/io/console.h"

// ---------------------------------------------------------------------------
// fs_init - Séquence d'initialisation du FS au démarrage du kernel
// ---------------------------------------------------------------------------
// Ordre obligatoire :
//   1. buf_cache_init — le cache de blocs (les autres en dépendent)
//   2. log_init       — le journal (utilise le cache)
//   3. inode_init     — le cache d'inodes (lit le superbloc via le cache)
//   4. file_init      — la table des fichiers ouverts
// ---------------------------------------------------------------------------
void fs_init(int dev)
{
    printf("fs_init: initialisation du système de fichiers (dev=%d)\n", dev);

    buf_cache_init();

    // Le log démarre au bloc 2, taille LOGSIZE
    // (le superbloc est en bloc 1, le log juste après)
    log_init(dev, 2, LOGSIZE);

    inode_init(dev);

    file_init();

    printf("fs_init: système de fichiers prêt\n");
}
