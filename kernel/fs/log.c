// ===========================================================================
// log.c - Journal d'écriture anticipée (write-ahead log)
// ===========================================================================
//
// Protocole de transaction :
//   log_begin_op()       — déclarer le début d'une opération FS
//   log_write(buf)       — enregistrer un bloc modifié dans le log
//   log_end_op()         — terminer ; si c'est la dernière op en cours,
//                          écrire le log sur disque puis le copier dans le FS
//
// Résistance aux pannes : au boot, log_init() relit le log et rejoue
// les transactions complètes (recover_from_log).
// ===========================================================================

#include <stddef.h>
#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/fs/log.h"
#include "kernel/fs/buffer.h"
#include "kernel/fs/block.h"
#include "kernel/mem/spinlock.h"
#include "kernel/proc/control.h"
#include "kernel/io/console.h"

// ---------------------------------------------------------------------------
// Structures internes du log
// ---------------------------------------------------------------------------

// En-tête du log sur le disque (premier bloc de la zone log)
struct logheader {
    int n;                  // Nombre de blocs enregistrés
    int block[LOGSIZE];     // Numéros des blocs dans le log
};

struct log {
    struct spinlock lock;
    int             start;      // Numéro du premier bloc log sur disque
    int             size;       // Taille max du log en blocs
    int             outstanding; // Nombre d'opérations FS en cours
    int             committing;  // 1 si commit en cours (attendre)
    int             dev;
    struct logheader lh;
};

static struct log log;

// Prototypes internes
static void commit(void);
static void recover_from_log(void);
static void write_log(void);
static void write_head(void);
static void read_head(void);
static void install_trans(int recovering);

// ---------------------------------------------------------------------------
// log_init - Lire le superbloc, initialiser le log et rejouer si crash
// ---------------------------------------------------------------------------
void log_init(int dev, int start, int size)
{
    if (sizeof(struct logheader) >= BSIZE) {
        printf("log_init: logheader trop grand\n");
        return;
    }

    spinlock_init(&log.lock, "log");
    log.start       = start;
    log.size        = size;
    log.outstanding = 0;
    log.committing  = 0;
    log.dev         = dev;

    read_head();
    recover_from_log();
    printf("log_init: log start=%d size=%d\n", start, size);
}

// ---------------------------------------------------------------------------
// log_begin_op - Déclarer le début d'une opération FS
// ---------------------------------------------------------------------------
void log_begin_op(void)
{
    spinlock_acquire(&log.lock);

    while (1) {
        if (log.committing) {
            // Un commit est en cours, attendre
            sleep(&log, &log.lock);
        } else if (log.lh.n + (log.outstanding + 1) * MAXOPBLOCKS > LOGSIZE) {
            // Pas assez de place dans le log, attendre un commit
            sleep(&log, &log.lock);
        } else {
            log.outstanding++;
            spinlock_release(&log.lock);
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// log_end_op - Terminer une opération ; déclencher le commit si on est le dernier
// ---------------------------------------------------------------------------
void log_end_op(void)
{
    int do_commit = 0;

    spinlock_acquire(&log.lock);
    log.outstanding--;

    if (log.committing) {
        printf("log_end_op: commit déjà en cours\n");
    }

    if (log.outstanding == 0) {
        do_commit = 1;
        log.committing = 1;
    } else {
        // Réveiller begin_op en attente de place
        wakeup(&log);
    }

    spinlock_release(&log.lock);

    if (do_commit) {
        commit();
        spinlock_acquire(&log.lock);
        log.committing = 0;
        wakeup(&log);
        spinlock_release(&log.lock);
    }
}

// ---------------------------------------------------------------------------
// log_write - Enregistrer un buffer dans le log (entre begin_op et end_op)
// ---------------------------------------------------------------------------
void log_write(struct buf *b)
{
    spinlock_acquire(&log.lock);

    if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1) {
        printf("log_write: log plein\n");
        spinlock_release(&log.lock);
        return;
    }

    if (log.outstanding < 1) {
        printf("log_write: hors transaction\n");
        spinlock_release(&log.lock);
        return;
    }

    // Absorption : si ce bloc est déjà dans le log, réutiliser la même case
    int i;
    for (i = 0; i < log.lh.n; i++) {
        if (log.lh.block[i] == (int)b->blockno)
            break;
    }
    log.lh.block[i] = (int)b->blockno;

    if (i == log.lh.n) {
        buf_pin(b);
        log.lh.n++;
    }

    spinlock_release(&log.lock);
}

// ---------------------------------------------------------------------------
// Fonctions internes
// ---------------------------------------------------------------------------

static void read_head(void)
{
    struct buf *buf = block_read(log.dev, log.start);
    if (buf == NULL) return;

    struct logheader *lh = (struct logheader *)buf->data;
    log.lh.n = lh->n;
    for (int i = 0; i < log.lh.n; i++)
        log.lh.block[i] = lh->block[i];

    block_release(buf);
}

static void write_head(void)
{
    struct buf *buf = block_read(log.dev, log.start);
    if (buf == NULL) return;

    struct logheader *hb = (struct logheader *)buf->data;
    hb->n = log.lh.n;
    for (int i = 0; i < log.lh.n; i++)
        hb->block[i] = log.lh.block[i];

    block_write(buf);
    block_release(buf);
}

// Copier les blocs modifiés dans la zone log du disque
static void write_log(void)
{
    for (int tail = 0; tail < log.lh.n; tail++) {
        struct buf *to   = block_read(log.dev, log.start + tail + 1);
        struct buf *from = block_read(log.dev, log.lh.block[tail]);
        if (to == NULL || from == NULL) continue;

        for (int i = 0; i < BSIZE; i++)
            to->data[i] = from->data[i];

        block_write(to);
        block_release(from);
        block_release(to);
    }
}

// Copier le log vers les blocs finaux du FS
static void install_trans(int recovering)
{
    for (int tail = 0; tail < log.lh.n; tail++) {
        struct buf *lbuf = block_read(log.dev, log.start + tail + 1);
        struct buf *dbuf = block_read(log.dev, log.lh.block[tail]);
        if (lbuf == NULL || dbuf == NULL) continue;

        for (int i = 0; i < BSIZE; i++)
            dbuf->data[i] = lbuf->data[i];

        block_write(dbuf);

        if (recovering == 0)
            buf_unpin(dbuf);

        block_release(lbuf);
        block_release(dbuf);
    }
}

static void commit(void)
{
    if (log.lh.n > 0) {
        write_log();       // Écrire les blocs modifiés dans le log
        write_head();      // Écrire l'en-tête (rend la transaction durable)
        install_trans(0);  // Appliquer le log dans le FS
        log.lh.n = 0;
        write_head();      // Effacer le log (n=0)
    }
}

static void recover_from_log(void)
{
    read_head();
    install_trans(1);  // Rejouer les transactions incomplètes
    log.lh.n = 0;
    write_head();
}
