#!/usr/bin/make -f

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com

# project name
TARGET_TAIL 	= tail
TARGET_GEN_TEXT = gen_text

CC		= gcc
TARGET_ARCH	=

# compiling flags
CPPFLAGS        =  -DPACKAGE_NAME=\"tail\" -DPACKAGE_TARNAME=\"tail\" -DPACKAGE_VERSION=\"1.0\" -DPACKAGE_STRING=\"tail\ 1.0\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STDLIB_H=1 -DHAVE_UNISTD_H=1 -DHAVE_SYS_PARAM_H=1 -DHAVE_GETPAGESIZE=1 -DHAVE_MMAP=1 -I.
CFLAGS		= -g -O2 -std=c99 -Wall -Werror

LINKER		= $(CC)
# liner flags here
LDFLAGS		= 

# directories
SRCDIR		= tail
OBJDIR		= obj
BINDIR		= bin

SOURCES 	:= $(wildcard $(SRCDIR)/*.c)
INCLUDES	:= $(wildcard $(SRCDIR)/*.h)
OBJECTS		:= $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

RM		= rm -f
MKDIR_P		= mkdir -p
RMDIR		= rm -rf

.PHONY: all
all:	$(BINDIR)/$(TARGET_TAIL) $(BINDIR)/$(TARGET_GEN_TEXT)

$(BINDIR)/$(TARGET_TAIL): $(BINDIR) $(OBJDIR) $(OBJECTS)
	@$(LINKER) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) -o $@ $(OBJECTS)
	@echo "LD "$@

$(BINDIR)/$(TARGET_GEN_TEXT): $(BINDIR) $(OBJDIR)/gen_text.o
	@$(LINKER) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(TARGET_ARCH) $(OBJDIR)/gen_text.o -o $@
	@echo "LD "$@

$(OBJDIR)/gen_text.o : tests/gen_text.c
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c $< -o $@
	@echo "CC "$<

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c $< -o $@
	@echo "CC "$<

$(BINDIR):
	@$(MKDIR_P) $(BINDIR)
	@echo "MKDIR "$(BINDIR)

$(OBJDIR):
	@$(MKDIR_P) $(OBJDIR)
	@echo "MKDIR "$(OBJDIR)

.PHONY: test
test:
	cd tests;
	@echo "Tests completed"

.PHONY: clean
clean:
	@$(RM) $(OBJECTS)
	@$(RM) config.log config.status
	@$(RMDIR) autom4te.cache
	@echo "Cleanup complete"

.PHONY: remove
remove: clean
	@$(RM) $(BINDIR)/$(TARGET_TAIL) $(BINDIR)/$(TARGET_GEN_TEXT)
	@$(RMDIR) $(BINDIR) $(OBJDIR)
	@echo "Executable removed"
