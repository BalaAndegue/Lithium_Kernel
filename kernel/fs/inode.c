// ===========================================================================
// inode.c - Cache d'inodes et opérations sur les fichiers/répertoires
// ===========================================================================
//
// Chaque fichier est représenté par un inode qui contient :
//   - type (fichier, répertoire, périphérique)
//   - taille en octets
//   - adresses des blocs de données (12 directs + 1 indirect)
//
// Le cache d'inodes en mémoire évite les lectures disque répétées.
// Toute modification d'inode passe par inode_update() puis log_write().
// ===========================================================================

#include <stddef.h>
#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/fs/inode.h"
#include "kernel/fs/log.h"
#include "kernel/fs/block.h"
#include "kernel/fs/buffer.h"
#include "kernel/mem/spinlock.h"
#include "kernel/io/console.h"

// ---------------------------------------------------------------------------
// Cache d'inodes en mémoire
// ---------------------------------------------------------------------------
struct {
    struct spinlock lock;
    struct inode    inode[NINODE];
} icache;

// Superbloc chargé au boot
static struct superblock sb;

void inode_init(int dev)
{
    spinlock_init(&icache.lock, "icache");

    for (int i = 0; i < NINODE; i++)
        sleeplock_init(&icache.inode[i].lock, "inode");

    // Lire le superbloc (bloc 1)
    struct buf *bp = block_read(dev, 1);
    if (bp == NULL) {
        printf("inode_init: impossible de lire le superbloc\n");
        return;
    }

    for (int i = 0; i < (int)sizeof(sb); i++)
        ((uint8 *)&sb)[i] = bp->data[i];

    block_release(bp);

    if (sb.magic != FSMAGIC) {
        printf("inode_init: magic invalide (%x)\n", sb.magic);
        return;
    }

    printf("inode_init: %u inodes, %u blocs données\n", sb.ninodes, sb.nblocks);
}

// ---------------------------------------------------------------------------
// inode_alloc - Allouer un nouvel inode de type 'type' sur le disque
// ---------------------------------------------------------------------------
struct inode *inode_alloc(uint32 dev, short type)
{
    for (uint32 inum = 1; inum < sb.ninodes; inum++) {
        struct buf *bp = block_read(dev, IBLOCK(inum, sb));
        if (bp == NULL) return NULL;

        struct dinode *dip = (struct dinode *)bp->data + inum % IPB;

        if (dip->type == 0) {
            // Inode libre trouvé
            for (int i = 0; i < (int)sizeof(struct dinode); i++)
                ((uint8 *)dip)[i] = 0;
            dip->type = type;
            log_write(bp);
            block_release(bp);
            return inode_get(dev, inum);
        }

        block_release(bp);
    }

    printf("inode_alloc: plus d'inode disponible\n");
    return NULL;
}

// ---------------------------------------------------------------------------
// inode_get - Obtenir un inode du cache (sans le verrouiller)
// ---------------------------------------------------------------------------
struct inode *inode_get(uint32 dev, uint32 inum)
{
    struct inode *ip;
    struct inode *empty = NULL;

    spinlock_acquire(&icache.lock);

    // Chercher dans le cache
    for (ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++) {
        if (ip->ref > 0 && ip->dev == dev && ip->inum == inum) {
            ip->ref++;
            spinlock_release(&icache.lock);
            return ip;
        }
        if (empty == NULL && ip->ref == 0)
            empty = ip;
    }

    if (empty == NULL) {
        printf("inode_get: cache plein\n");
        spinlock_release(&icache.lock);
        return NULL;
    }

    ip = empty;
    ip->dev   = dev;
    ip->inum  = inum;
    ip->ref   = 1;
    ip->valid = 0;

    spinlock_release(&icache.lock);
    return ip;
}

// ---------------------------------------------------------------------------
// inode_dup - Incrémenter le compteur de références
// ---------------------------------------------------------------------------
struct inode *inode_dup(struct inode *ip)
{
    spinlock_acquire(&icache.lock);
    ip->ref++;
    spinlock_release(&icache.lock);
    return ip;
}

// ---------------------------------------------------------------------------
// inode_lock - Verrouiller l'inode et charger ses données du disque si besoin
// ---------------------------------------------------------------------------
void inode_lock(struct inode *ip)
{
    if (ip == NULL || ip->ref < 1) {
        printf("inode_lock: inode invalide\n");
        return;
    }

    sleeplock_acquire(&ip->lock);

    if (!ip->valid) {
        struct buf *bp = block_read(ip->dev, IBLOCK(ip->inum, sb));
        if (bp == NULL) return;

        struct dinode *dip = (struct dinode *)bp->data + ip->inum % IPB;

        ip->type  = dip->type;
        ip->major = dip->major;
        ip->minor = dip->minor;
        ip->nlink = dip->nlink;
        ip->size  = dip->size;

        for (int i = 0; i < NDIRECT + 1; i++)
            ip->addrs[i] = dip->addrs[i];

        block_release(bp);
        ip->valid = 1;

        if (ip->type == 0) {
            printf("inode_lock: inode %u sans type\n", ip->inum);
        }
    }
}

// ---------------------------------------------------------------------------
// inode_unlock - Relâcher le verrou
// ---------------------------------------------------------------------------
void inode_unlock(struct inode *ip)
{
    if (ip == NULL || !sleeplock_held(&ip->lock) || ip->ref < 1) {
        printf("inode_unlock: état invalide\n");
        return;
    }
    sleeplock_release(&ip->lock);
}

// ---------------------------------------------------------------------------
// inode_update - Écrire l'inode en mémoire sur le disque (via le log)
// ---------------------------------------------------------------------------
void inode_update(struct inode *ip)
{
    struct buf    *bp  = block_read(ip->dev, IBLOCK(ip->inum, sb));
    if (bp == NULL) return;

    struct dinode *dip = (struct dinode *)bp->data + ip->inum % IPB;

    dip->type  = ip->type;
    dip->major = ip->major;
    dip->minor = ip->minor;
    dip->nlink = ip->nlink;
    dip->size  = ip->size;

    for (int i = 0; i < NDIRECT + 1; i++)
        dip->addrs[i] = ip->addrs[i];

    log_write(bp);
    block_release(bp);
}

// ---------------------------------------------------------------------------
// inode_put - Décrémenter la référence ; libérer l'inode si ref == 0
// ---------------------------------------------------------------------------
void inode_put(struct inode *ip)
{
    spinlock_acquire(&icache.lock);

    if (ip->ref == 1 && ip->valid && ip->nlink == 0) {
        // Plus aucun répertoire ne pointe sur cet inode : le libérer
        spinlock_release(&icache.lock);

        inode_lock(ip);
        // Libération des blocs de données — simplifié : remise à zéro du type
        ip->type = 0;
        inode_update(ip);
        ip->valid = 0;
        inode_unlock(ip);

        spinlock_acquire(&icache.lock);
    }

    ip->ref--;
    spinlock_release(&icache.lock);
}

void inode_unlock_put(struct inode *ip)
{
    inode_unlock(ip);
    inode_put(ip);
}

// ---------------------------------------------------------------------------
// inode_stat - Remplir une structure stat depuis un inode verrouillé
// ---------------------------------------------------------------------------
void inode_stat(struct inode *ip, struct stat *st)
{
    st->dev   = (int)ip->dev;
    st->ino   = ip->inum;
    st->type  = ip->type;
    st->nlink = ip->nlink;
    st->size  = ip->size;
}

// ---------------------------------------------------------------------------
// inode_read - Lire n octets depuis l'offset off d'un inode
// ---------------------------------------------------------------------------
int inode_read(struct inode *ip, int user_dst, uint64 dst, uint32 off, uint32 n)
{
    (void)user_dst;

    if (off > ip->size || off + n < off) return -1;
    if (off + n > ip->size) n = ip->size - off;

    uint32 tot, m;
    for (tot = 0; tot < n; tot += m, off += m, dst += m) {
        uint32 addr = ip->addrs[off / BSIZE];
        if (addr == 0) break;

        struct buf *bp = block_read(ip->dev, addr);
        if (bp == NULL) break;

        m = BSIZE - off % BSIZE;
        if (m > n - tot) m = n - tot;

        uint8 *src = bp->data + (off % BSIZE);
        uint8 *d   = (uint8 *)dst;
        for (uint32 i = 0; i < m; i++) d[i] = src[i];

        block_release(bp);
    }

    return (int)tot;
}

// ---------------------------------------------------------------------------
// inode_write - Écrire n octets à l'offset off d'un inode
// ---------------------------------------------------------------------------
int inode_write(struct inode *ip, int user_src, uint64 src, uint32 off, uint32 n)
{
    (void)user_src;

    if (off > ip->size || off + n < off) return -1;
    if (off + n > MAXFILE * BSIZE) return -1;

    uint32 tot, m;
    for (tot = 0; tot < n; tot += m, off += m, src += m) {
        uint32 bn = off / BSIZE;

        // Allouer un nouveau bloc si nécessaire — simplifié (pas de bitmap)
        if (ip->addrs[bn] == 0) {
            printf("inode_write: allocation de bloc non implémentée\n");
            break;
        }

        struct buf *bp = block_read(ip->dev, ip->addrs[bn]);
        if (bp == NULL) break;

        m = BSIZE - off % BSIZE;
        if (m > n - tot) m = n - tot;

        uint8 *d   = bp->data + (off % BSIZE);
        uint8 *s   = (uint8 *)src;
        for (uint32 i = 0; i < m; i++) d[i] = s[i];

        log_write(bp);
        block_release(bp);
    }

    if (off > ip->size) ip->size = off;
    inode_update(ip);

    return (int)tot;
}
