# Description
This is an extended code artifact accompanying the paper: "SIREN: Software Identification and Recognition in HPC Systems". It contains source code for the collector, receiver, database, and post-processing. SIREN can collect data on the process-level, including process identifiers and metadata, loaded modules, shared libraries, compiler information, memory-mapped files, Python imported packages, and fuzzy hashes of the executable. For a detailed explanation please see the paper.

# Intended use and data collected
SIREN is a research artifact for HPC software fingerprinting. Under `LD_PRELOAD`, the collector runs in the observed process and emits UDP messages per process containing: the executable path, PPID/UID/GID, inode/size/permissions/timestamps from `stat`, environment modules (`LOADEDMODULES`), the shared-object list from `dl_iterate_phdr`, ELF `.comment` (compiler strings), `/proc/<pid>/maps` entries (post-processed into Python imported packages), and optional ssdeep fuzzy hashes over the file, printable strings, and symbols. Slurm identifiers (`SLURM_JOB_ID`, `SLURM_STEP_ID`) are read when present.

# Prerequisites
gcc, make, go, git, sqlite3, python3. Python packages are listed in `analysis/requirements.txt` (`pip install -r analysis/requirements.txt`).

# Setup and execution
- bash setup.sh
- bash test.sh
- bash analyze.sh

# Output
- output/statistics.txt — human-readable summary of captured processes
- output/siren.db — SQLite database of raw captures (one row per UDP message)
- output/captures.txt — flat formatted dump of the database

# Configuration
- Toggle individual collectors in code/config.h.
- Sender UDP target (IP/port) is set at compile time in code/common.h (`UDP_IP`, `UDP_PORT`).
- Receiver listen address is passed via `--port` to `listener/siren_listener`.
- Rebuild after changes.

# Directory layout
- code/       LD_PRELOAD collector (C, builds to code/siren.so)
- listener/   UDP receiver and SQLite writer (Go)
- analysis/   pandas-based post-processing and stats
- demo/       small C and Python binaries exercised by test.sh

# License
The source code in this repository is released under the MIT License (see `LICENSE`).

# Reference
Jakobsche, T., Robertsén, F., Jones, J. R., Haus, U. U., & Ciorba, F. M. (2025, November).
SIREN: Software Identification and Recognition in HPC Systems. In *Proceedings of the
International Conference for High Performance Computing, Networking, Storage and Analysis*
(pp. 1742–1754). [doi:10.1145/3712285.3759873](https://doi.org/10.1145/3712285.3759873)

Archived artifact on Zenodo: [zenodo.org/records/15301754](https://zenodo.org/records/15301754)
