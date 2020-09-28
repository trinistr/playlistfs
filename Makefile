#Compiler and Linker
CC          := gcc

#The Target Binary Program
TARGET      := playlistfs

#The Directories, Source, Includes, Objects and Binary
SRCDIR      := src
INCDIR      := include
BUILDDIR    := obj
TARGETDIR   := .
MANDIR      := .
SRCEXT      := c
DEPEXT      := d
OBJEXT      := o

#man things
MANGEN      := ./mangen
MAN_NAME    := playlist as a file system
MAN_SECTION := 1

#Flags, Libraries and Includes
DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS  := -Wall -ggdb -O0 --std=c11 $(shell pkg-config fuse --cflags) $(shell pkg-config glib-2.0 --cflags)
else
    CFLAGS  := -Wall -O3 --std=c11 $(shell pkg-config fuse --cflags) $(shell pkg-config glib-2.0 --cflags)
endif
LIB         := $(shell pkg-config fuse --libs) $(shell pkg-config glib-2.0 --libs)
INC         := -I$(INCDIR) -I/usr/include
INCDEP      := -I$(INCDIR)

#---------------------------------------------------------------------------------
#DO NOT EDIT BELOW THIS LINE
#---------------------------------------------------------------------------------
SOURCES     := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS     := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))

#Default Make
all: $(TARGET)

#Remake
remake: cleaner all

#Make the Directories
directories:
	@mkdir -p $(TARGETDIR)
	@mkdir -p $(BUILDDIR)

#Clean only Objecst
clean:
	@$(RM) -rf $(BUILDDIR)

#Full Clean, Objects and Binaries
cleaner: clean
	@$(RM) $(TARGETDIR)/$(TARGET)
	@$(RM) $(MANDIR)/$(TARGET).$(MAN_SECTION).gz

#Pull in dependency info for *existing* .o files
-include $(OBJECTS:.$(OBJEXT)=.$(DEPEXT))

#Link
$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGETDIR)/$(TARGET) $^ $(LIB)

#Compile
$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<
	@$(CC) $(CFLAGS) $(INCDEP) -MM $(SRCDIR)/$*.$(SRCEXT) > $(BUILDDIR)/$*.$(DEPEXT)
	@cp -f $(BUILDDIR)/$*.$(DEPEXT) $(BUILDDIR)/$*.$(DEPEXT).tmp
	@sed -e 's|.*:|$(BUILDDIR)/$*.$(OBJEXT):|' < $(BUILDDIR)/$*.$(DEPEXT).tmp > $(BUILDDIR)/$*.$(DEPEXT)
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILDDIR)/$*.$(DEPEXT).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILDDIR)/$*.$(DEPEXT)
	@rm -f $(BUILDDIR)/$*.$(DEPEXT).tmp

man: $(TARGET).$(MAN_SECTION).gz

$(TARGET).$(MAN_SECTION).gz: $(TARGET).$(MAN_SECTION)
	gzip -f $(MANDIR)/$(TARGET).$(MAN_SECTION)

$(TARGET).$(MAN_SECTION): $(TARGET)
	@mkdir -p $(MANDIR)
	$(MANGEN) $(TARGETDIR)/$(TARGET) "$(MAN_NAME)" $(MAN_SECTION) > $(MANDIR)/$(TARGET).$(MAN_SECTION)

#Non-File Targets
.PHONY: all remake clean cleaner man
