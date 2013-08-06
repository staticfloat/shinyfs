EXE=MultiTest

# These are compile-time options
SHINY_DB = DB_LEVEL
#SHINY_DB = DB_KYOTO

# Build up our list of DEFINES
DEFINES = DEBUG SHINY_DB=$(SHINY_DB)


SRC = $(patsubst ./%.cpp, %.cpp, $(shell find . -name \*.cpp))
OBJ = $(patsubst %.cpp, ./.obj/%.o,$(SRC))
CPPFLAGS = -I $(shell echo ~)/Dropbox/coding/platform/ $(addprefix -D,$(DEFINES))
CFLAGS = -g -std=c++11 
LDFLAGS = -lrt

ifeq ($(SHINY_DB),DB_LEVEL)
LDFLAGS += -lleveldb
else ifeq ($(SHINY_DB),DB_KYOTO)
LDFLAGS += -lkyotocabinet
endif

CXX=g++

all: $(EXE) filecache
	@echo "Could it be?! Could serialization be done?! Let's write up tests"
	@echo "Then we'll get RootDir to create a DBWrapper and do read/write tests as well!"
	@echo
	@echo

clean:
	-rm -rf ./.obj
	-rm -f $(EXE)

filecache:
	@mkdir -p ./.filecache

$(EXE): $(OBJ)
	$(CXX) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

# This is the rule that matches every .cpp file
./.obj/%.o: %.cpp Nodes.h Makefile
	@mkdir -p $(shell dirname $@)
	$(CXX) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: filecache