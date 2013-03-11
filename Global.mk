# Global defines
export OBJDIR=$(PWD)/.obj
export BUILDDIR=$(PWD)/build

export DEFINES=LEVELDB

export CFLAGS=-I$(shell echo ~)/Dropbox/coding/platform $(addprefix -D,$(DEFINES))

# These are the executables we'll build
export SHINYFS_EXE=$(BUILD)/shinyfs