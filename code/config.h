#ifndef CONFIG_H
#define CONFIG_H

// Set to 1 to enable, 0 to disable. Rebuild after edits.
//
// INFO is not listed here, siren_prep.py assumes every row has INFO (dropna(subset=['INFO'])), so disabling it breaks the analysis pipeline. Python SCRIPT INFO is always on when the executable is python-like.
//
// MODULES       user-dir only; environment modules loaded
// OBJECTS       any-dir; dl_iterate_phdr shared-object list
// COMPILERS     user-dir only; ELF .comment section
// MAPS          user-dir and python only; powers PYTHON_IMPORTS in siren_prep.py; off = no imports column
//
// FILE_H        applies to SELF (user-dir) and python SCRIPT; ssdeep file hash
// STRINGS_H     user-dir only; ssdeep hash of printable ELF strings
// SYMBOLS_H     user-dir only; ssdeep hash of ELF symbol names
// CHUNK_H       ssdeep prefix on OBJECTS/MODULES/COMPILERS/MAPS chunks

#define COLLECT_MODULES   1
#define COLLECT_OBJECTS   1
#define COLLECT_COMPILERS 1
#define COLLECT_MAPS      1

#define COLLECT_FILE_H    0
#define COLLECT_STRINGS_H 0
#define COLLECT_SYMBOLS_H 0
#define COLLECT_CHUNK_H   0

#endif
