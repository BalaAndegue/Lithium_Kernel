// ===========================================================================
// virtio.h - Interface du pilote disque VirtIO
// ===========================================================================

#ifndef KERNEL_FS_VIRTIO_H
#define KERNEL_FS_VIRTIO_H

struct buf;

// Initialiser le disque VirtIO (configurer les virtqueues)
void virtio_disk_init(void);

// Lire ou écrire un bloc disque
// write=0 : lire depuis le disque vers b->data
// write=1 : écrire b->data sur le disque
void virtio_disk_rw(struct buf *b, int write);

// Gérer une interruption VirtIO (appelée depuis plic / kerneltrap)
void virtio_disk_intr(void);

#endif
