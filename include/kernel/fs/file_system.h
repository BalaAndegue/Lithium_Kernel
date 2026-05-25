// ===========================================================================
// file_system.h - Interface d'initialisation du système de fichiers
// ===========================================================================

#ifndef KERNEL_FS_FILE_SYSTEM_H
#define KERNEL_FS_FILE_SYSTEM_H

// Initialiser le système de fichiers complet
// (buffer cache → log → inodes → table de fichiers)
void fs_init(int dev);

#endif
