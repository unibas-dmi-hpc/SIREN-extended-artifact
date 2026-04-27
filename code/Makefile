# Makefile for building the siren.so shared object.
# -shared: Tells gcc to build this as a shared object ELF.
# -fPIC: Makes the binary compile as Position Independent Code (PIC), which stops it from making assumptions about jumps or calls to fixed addresses.
# -Wl,--version-script=version_script: The version script controls which symbols in the shared library are exported (visible outside the shared library) and which are not.
# -Wl,-Bsymbolic: Functions or variables within the shared library are resolved to the internal definitions in this library, rather than allowing the symbol to be overridden by an external definition.

CC = gcc
CFLAGS = -O2 -fPIC -Wall -Wextra -Werror -fstack-protector-all -std=gnu99
LDFLAGS = -shared -Wl,--version-script=version_script -Wl,-Bsymbolic
INCLUDES = -I../libraries/include -I../libraries/xxHash
LIBDIRS = -L../libraries/lib
LDLIBS = ../libraries/lib/libelf.a ../libraries/lib/libfuzzy.a ../libraries/xxHash/xxhash.c

SRCS = siren.c message_handler.c collect_info.c collect_modules.c collect_objects.c collect_compilers.c collect_maps.c collect_ssdeep_file.c collect_ssdeep_strings.c collect_ssdeep_symbols.c

all: siren.so

siren.so:
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(LIBDIRS) $(SRCS) -o siren.so $(LDLIBS)

clean: 
	rm -f siren.so