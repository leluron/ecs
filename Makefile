VARS_OLD := $(.VARIABLES)

DEPSDIR = .deps
OBJDIR = .obj
SRCDIR = src
DISTDIR = dist

FLAGS = -g -std=c++17

SRC = $(patsubst $(SRCDIR)/%.cpp, %, $(wildcard $(SRCDIR)/*.cpp))
OBJPATH = $(patsubst %, $(OBJDIR)/%.o, $(PARSER) $(SRC))

BINARY = $(DISTDIR)/ecs

COMP_HEADERS = $(wildcard $(SRCDIR)/components/*.h)
PROC_HEADERS = $(wildcard $(SRCDIR)/processes/*.h)

GEN_FILES = components.def components_include.h processes.def processes_include.h
GEN_FILES_FULL = $(patsubst %, $(SRCDIR)/generated/%, $(GEN_FILES))

ecs: $(BINARY)

$(BINARY): $(OBJPATH)
	g++ $^ -o $@ $(FLAGS) -llua -ldl

clean:
	rm -rf $(OBJDIR)
	rm -rf $(DEPSDIR)
	rm -rf $(SRCDIR)/generated

.PHONY: clean

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(DEPSDIR)/%.d $(GEN_FILES_FULL) | $(DEPSDIR) $(OBJDIR)
	g++ -MT $@ -MMD -MP -MF $(DEPSDIR)/$*.d -c -o $@ $< $(FLAGS)

$(DEPSDIR): ; mkdir -p $@
$(OBJDIR): ; mkdir -p $@
$(SRCDIR)/generated: ; mkdir -p $@

$(GEN_FILES_FULL) &: $(COMP_HEADERS) $(PROC_HEADERS) $(SRCDIR)/generated
	./gen

DEPSFILES := $(patsubst %,$(DEPSDIR)/%.d, $(SRC))
$(DEPSFILES):
include $(wildcard $(DEPSFILES))

vars:; $(foreach v, $(filter-out $(VARS_OLD) VARS_OLD,$(.VARIABLES)), $(info $(v) = $($(v)))) @#noop

