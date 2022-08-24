CFLAGS:=-Wall -Wno-unused-result -std=gnu17
NONO_MAIN_EXE=NonoMain.exe

# Check is debug enabled
DEBUG ?= 1
ifeq ($(DEBUG),1)
  CFLAGS+= -g -D_DEBUG_
  O=.debug
else
  CFLAGS+= -O3 -D_RELEASE_
  O=.release
endif

SOURCE_FILES:=Source/NonoMain.c Source/Nonograms.c Source/LogicalRules.c Source/ValidityCheck.c Source/SwitchingComponent.c

.PHONY: all NonoSolverGraph
all: $(NONO_SOLVER_EXE)

.PHONY: all
all: $(NONO_MAIN_EXE) $(NONO_CONF_GEN)

# Nonogram solver program
$(NONO_MAIN_EXE): $(SOURCE_FILES)
	gcc $(CFLAGS) -o$@ $^
# Create the solver vith analyzer run.
NonoSolver_Analyzer: $(SOURCE_FILES)
	gcc -fanalyzer -o$(NONO_SOLVER_EXE) $^

# Build object directory
$(O):
	mkdir $(O)
