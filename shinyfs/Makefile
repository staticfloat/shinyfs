include ../Global.mk

export SRC=$(patsubst ./%.cpp, %.cpp, $(shell find . -name \*.cpp))
export OBJ=$(patsubst %.cpp, $(OBJDIR)/shinyfs/%.o,$(SRC)) #$(SRC:./%.cpp=$(OBJDIR)/%.o)

all: $(SHINYFS_EXE)


$(SHINYFS_EXE): $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(OBJ)

# This is the rule that matches every .cpp file
$(OBJDIR)/shinyfs/%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@
