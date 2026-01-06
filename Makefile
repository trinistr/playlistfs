#Compiler and Linker
CC          := gcc

#The Target Binary Program
TARGET      := playlistfs

#The Directories, Source, Includes, Objects and Binary
SRCDIR      := src
INCDIR      := include
BUILDDIR    := obj
DISTDIR     := dist
TARGETDIR   := $(DISTDIR)/bin
MANDIR      := $(DISTDIR)/share/man/man1
SCRIPTDIR   := scripts
SRCEXT      := c
DEPEXT      := d
OBJEXT      := o

#The installation prefix
PREFIX      ?= $(HOME)/.local

#man things
MANGEN      := $(SCRIPTDIR)/mangen
MAN_NAME    := playlist as a file system
MAN_SECTION := 1

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

INC         := -I$(INCDIR) -I/usr/include
INCDEP      := -I$(INCDIR)

# Compile binary (default target)
bin: $(TARGETDIR)/$(TARGET)

# Build compressed manpage
man: $(MANDIR)/$(TARGET).$(MAN_SECTION).gz

# Run tests
test: bin test-current

# Install everything
install-all: install install-mime install-mime-default

# Clean and recompile
remake: cleaner bin

install: install-bin install-man
install-bin: bin
	install --target-directory "$(PREFIX)/bin" -D -- $(TARGETDIR)/$(TARGET)
install-man: man
	install --target-directory "$(PREFIX)/share/man/man1" -D --mode=644 -- $(MANDIR)/$(TARGET).$(MAN_SECTION).gz
install-mime:
	install --target-directory "$(PREFIX)/share/applications" -D --mode=644 -- $(DISTDIR)/share/applications/*
	install --target-directory "$(PREFIX)/bin" -D -- $(TARGETDIR)/*
	xdg-mime install --novendor $(DISTDIR)/share/mime/packages/*
install-mime-default:
	xdg-mime default $(shell basename $(DISTDIR)/share/applications/*) $(shell grep -o 'type="[^"]*' $(DISTDIR)/share/mime/packages/* | cut -c 7-)

test-current:
	@total=0; failed=0; \
	for test_file in tests/test*.sh; do \
		if [ -f "$$test_file" ]; then \
			total=$$((total + 1)); \
			echo "=== $$test_file ==="; \
			chmod +x "$$test_file" 2>/dev/null; \
			$(RUNFLAGS) ./"$$test_file"; \
			if [ $$? -eq 0 ]; then \
				echo "✓ $$test_file passed"; \
			else \
				echo "✗ $$test_file failed"; \
				failed=$$((failed + 1)); \
			fi; \
			echo ""; \
		fi; \
	done; \
	if [ $$failed -eq 0 ]; then \
		echo "$$total test files executed, all tests nominal!"; \
		exit 0; \
	else \
		echo "$$total test files executed, $$failed failed!"; \
		exit 1; \
	fi

# Build manpage out of help
$(MANDIR)/$(TARGET).$(MAN_SECTION): $(TARGETDIR)/$(TARGET)
	@mkdir -p $(MANDIR)
	$(MANGEN) $(TARGETDIR)/$(TARGET) "$(MAN_NAME)" $(MAN_SECTION) > $(MANDIR)/$(TARGET).$(MAN_SECTION)
# Compress manpage
$(MANDIR)/$(TARGET).$(MAN_SECTION).gz: $(MANDIR)/$(TARGET).$(MAN_SECTION)
	gzip --force --keep $(MANDIR)/$(TARGET).$(MAN_SECTION)

directories:
	@mkdir -p $(TARGETDIR)
	@mkdir -p $(BUILDDIR)

# Delete intermediate files
clean:
	@$(RM) -rf $(BUILDDIR)
	@$(RM) -f $(MANDIR)/$(TARGET).$(MAN_SECTION)
# Delete intermediates and results
cleaner: clean
	@$(RM) $(TARGETDIR)/$(TARGET)
	@$(RM) $(MANDIR)/$(TARGET).$(MAN_SECTION).gz

#Non-File Targets
.PHONY: bin remake clean cleaner man install install-bin install-man install-mime install-mime-default test test-current directories

#---------------------------------------------------------------------------------
#DO NOT EDIT BELOW THIS LINE
#---------------------------------------------------------------------------------
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))

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
