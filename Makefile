#Compiler and Linker
CC          ::= gcc

#The Target Binary Program
TARGET      ::= playlistfs

#The Directories, Source, Includes, Objects and Binary
SRCDIR      ::= src
INCDIR      ::= include
BUILDDIR    ::= obj
DISTDIR     ::= dist
TARGETDIR   ::= $(DISTDIR)/bin
MANDIR      ::= $(DISTDIR)/share/man/man1
SCRIPTDIR   ::= scripts
SRCEXT      ::= c
DEPEXT      ::= d
OBJEXT      ::= o

ifeq ($(strip $(PREFIX)),)
    # The installation prefix
    PREFIX ::= $(HOME)/.local
    ifneq ($(strip $(XDG_DATA_HOME)),)
        # Use XDG_DATA_HOME if set
        DATA_HOME ::= $(XDG_DATA_HOME)
    else
        DATA_HOME ::= $(PREFIX)/share
    endif
else
    DATA_HOME ::= $(PREFIX)/share
endif

#man things
MANGEN      ::= $(SCRIPTDIR)/mangen
MAN_NAME    ::= playlist-like FUSE file system

#Flags, Libraries and Includes
FUSE ?= 3 # Set to 2 to use FUSE 2
DEBUG ?= 0 # Set to 1 to deoptimize and enable gdb support
SANITIZER ?= 0 # Set to 1 to enable ASan and extra diagnostics in tests

CFLAGS += -Wall -O3 --std=c11 -DBUILD_DATE=\"$(shell date +%Y-%m-%d)\" $(shell pkg-config glib-2.0 --cflags)
LDFLAGS += $(shell pkg-config glib-2.0 --libs)
RUNFLAGS +=

ifeq ($(FUSE), 2)
    FUSE_VERSION ?= 29
    CFLAGS += -DFUSE_USE_VERSION=$(FUSE_VERSION) $(shell pkg-config fuse --cflags) -DFUSE_LIB_VERSION=\"$(shell pkg-config fuse --modversion)\"
    LDFLAGS += $(shell pkg-config fuse --libs)
else
    FUSE_VERSION ?= 35
    CFLAGS += -DFUSE_USE_VERSION=$(FUSE_VERSION) $(shell pkg-config fuse3 --cflags) -DFUSE_LIB_VERSION=\"$(shell pkg-config fuse3 --modversion)\"
    LDFLAGS += $(shell pkg-config fuse3 --libs)
endif

ifeq ($(DEBUG), 1)
    CFLAGS += -ggdb -O0
endif
ifeq ($(SANITIZER), 1)
    CFLAGS += -fsanitize=address -fsanitize-address-use-after-scope
    LDFLAGS += -fsanitize=address
    RUNFLAGS += ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1
endif

# Compile binary (default target)
bin: $(TARGETDIR)/$(TARGET)

# Build compressed manpage
man: $(MANDIR)/$(TARGET).1.gz

# Run tests
test: bin test-current
test-current:
	$(MAKE) -C tests

# Install everything
install-full: install install-supplementary install-set-default
# Uninstall everything
uninstall-full: uninstall uninstall-supplementary

install: install-bin install-man
install-bin: bin
	install --target-directory "$(PREFIX)/bin" -D --strip -- $(TARGETDIR)/$(TARGET)
install-man: man
	install --target-directory "$(DATA_HOME)/man/man1" -D --mode=644 -- $(MANDIR)/$(TARGET).1.gz
install-supplementary: install-mime-package
	install --target-directory "$(PREFIX)/bin" -D -- $(TARGETDIR)/playlistfs_mount
	install --target-directory "$(DATA_HOME)/applications" -D --mode=644 -- $(DISTDIR)/share/applications/playlistfs-mount.desktop
install-mime-package:
	xdg-mime install --novendor $(DISTDIR)/share/mime/packages/playlistfs.xml
install-set-default:
	xdg-mime default playlistfs-mount.desktop $(shell grep -o 'type="[^"]*' $(DISTDIR)/share/mime/packages/playlistfs.xml | cut -c 7-)

uninstall: uninstall-bin uninstall-man
uninstall-bin:
	-$(RM) "$(PREFIX)/bin/$(TARGET)"
uninstall-man:
	-$(RM) "$(DATA_HOME)/man/man1/$(TARGET).1.gz"
uninstall-supplementary: uninstall-mime-package
	-$(RM) "$(PREFIX)/bin/playlistfs_mount"
	-$(RM) "$(DATA_HOME)/applications/playlistfs-mount.desktop"
uninstall-mime-package:
	-xdg-mime uninstall $(DISTDIR)/share/mime/packages/playlistfs.xml

# Build manpage out of help
$(MANDIR)/$(TARGET).1: $(TARGETDIR)/$(TARGET)
	@mkdir -p $(MANDIR)
	$(MANGEN) $(TARGETDIR)/$(TARGET) "$(MAN_NAME)" 1 > $(MANDIR)/$(TARGET).1
# Compress manpage
$(MANDIR)/$(TARGET).1.gz: $(MANDIR)/$(TARGET).1
	gzip --force --keep $(MANDIR)/$(TARGET).1

# Clean and recompile
remake: cleaner bin

# Delete intermediate files
clean:
	@$(RM) -rf $(BUILDDIR)
	@$(RM) -f $(MANDIR)/$(TARGET).1
# Delete intermediates and results
cleaner: clean
	@$(RM) $(TARGETDIR)/$(TARGET)
	@$(RM) $(MANDIR)/$(TARGET).1.gz

version.major:
	rake -f $(SCRIPTDIR)/rake.rb version:major
version.minor:
	rake -f $(SCRIPTDIR)/rake.rb version:minor
version.patch:
	rake -f $(SCRIPTDIR)/rake.rb version:patch

#Non-File Targets
.PHONY: \
bin remake clean cleaner man include \
test test-current \
install-full install install-bin install-man install-supplementary install-mime-package install-set-default \
uninstall-full uninstall uninstall-bin uninstall-man uninstall-supplementary uninstall-mime-package \
version.major version.minor version.patch

#---------------------------------------------------------------------------------
#DO NOT EDIT BELOW THIS LINE
#---------------------------------------------------------------------------------
INC         ::= -I$(INCDIR) -I/usr/include
INCDEP      ::= -I$(INCDIR)

SOURCES ::= $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS ::= $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))

#Pull in dependency info for *existing* .o files
-include $(OBJECTS:.$(OBJEXT)=.$(DEPEXT))

#Link
$(TARGETDIR)/$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGETDIR)/$(TARGET) $^ $(LDFLAGS)

#Compile
$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<
	@$(CC) $(CFLAGS) $(INCDEP) -MM $(SRCDIR)/$*.$(SRCEXT) > $(BUILDDIR)/$*.$(DEPEXT)
	@cp -f $(BUILDDIR)/$*.$(DEPEXT) $(BUILDDIR)/$*.$(DEPEXT).tmp
	@sed -e 's|.*:|$(BUILDDIR)/$*.$(OBJEXT):|' < $(BUILDDIR)/$*.$(DEPEXT).tmp > $(BUILDDIR)/$*.$(DEPEXT)
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILDDIR)/$*.$(DEPEXT).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILDDIR)/$*.$(DEPEXT)
	@rm -f $(BUILDDIR)/$*.$(DEPEXT).tmp
