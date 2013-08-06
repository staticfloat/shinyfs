# Global defines
export OBJDIR=$(PWD)/.obj
export BUILDDIR=$(PWD)/build

DEFINES=LEVELDB
INCLUDES=$(shell echo ~)/Dropbox/coding/platform

CFLAGS=-std=c++0x $(addprefix -I,$(INCLUDES)) $(addprefix -D,$(DEFINES)) 

# These are the executables we'll build
export SHINYFS_EXE=$(BUILD)/shinyfs
