include Global.mk

all: $(SHINYFS_EXE)


# We sub all the hard work off onto SHINYFS. :P
$(SHINYFS_EXE): | $(BUILDDIR) $(OBJDIR)
	@make -e -C shinyfs


# Creates the BUILDDIR directory
$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

# Creates the OBJDIR directory
$(OBJDIR):
	@mkdir -p $(OBJDIR)

# Cleans up by killing BUILDDIR and OBJDIR
clean:
	rm -rf $(BUILDDIR) $(OBJDIR)