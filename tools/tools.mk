
#CROSS_COMPILE = mipsel-linux-
CROSS_COMPILE = 


# Define make variables
AS      = $(CROSS_COMPILE)as
LD      = $(CROSS_COMPILE)ld
CC      = $(CROSS_COMPILE)gcc
CXX     = $(CROSS_COMPILE)g++
AR      = $(CROSS_COMPILE)ar
NM      = $(CROSS_COMPILE)nm
STRIP   = $(CROSS_COMPILE)strip
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
RANLIB  = $(CROSS_COMPILE)ranlib

# tools
MKDIR   = mkdir -p
PWD     = pwd
MV      = mv
CHMOD	= chmod 755
CD	= cd
UMASK  = umask 077
CP      = cp -af
RM      = rm
SORT    = sort
CAT			= cat
SED     = sed
TOUCH   = touch
TR		= tr
LCASE	= tr A-Z a-z
ECHO	= echo
TAR		= tar
GZIP		= gzip
LN		= ln -sf
FIND 	= $(shell find $(1) -name $(2))
	FIND0 	= $(shell find $(1) -maxdepth 1 -name $(2))


# $(call make-depend, src, obj, dep)
define make-depend
	$(CC) -MM -MF $3 -MP -MT $2 $(CFLAGS) $1
endef

