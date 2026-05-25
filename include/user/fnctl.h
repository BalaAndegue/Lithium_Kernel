// ===========================================================================
// fnctl.h (user) - Flags pour l'ouverture de fichiers
// ===========================================================================

#ifndef USER_FNCTL_H
#define USER_FNCTL_H

#define O_RDONLY   0x000   // Ouverture en lecture seule
#define O_WRONLY   0x001   // Ouverture en écriture seule
#define O_RDWR     0x002   // Ouverture en lecture-écriture
#define O_CREATE   0x200   // Créer le fichier s'il n'existe pas
#define O_TRUNC    0x400   // Tronquer à zéro si le fichier existe

// Descripteurs standard
#define STDIN_FILENO   0
#define STDOUT_FILENO  1
#define STDERR_FILENO  2

#endif
