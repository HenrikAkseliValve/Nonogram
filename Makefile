CFLAGS:=-Wall -Wno-unused-result -std=c17
NONO_SOLVER_EXE=NonoSolver.exe

# Check is debug enabled
DEBUG ?= 1
ifeq ($(DEBUG),1)
  CFLAGS+= -g -D_DEBUG_
  O=.debug
else
  CFLAGS+= -O3 -D_RELEASE_
  O=.release
endif

SOURCE_FILES:=Source/NonoSolver.c Source/Nonograms.c Source/LogicalRules.c Source/ValidityCheck.c Source/SwitchingComponent.c

.PHONY: all NonoSolverGraph
all: $(NONO_SOLVER_EXE)

#DEPRECATED
# Count given nonogram's solution count program.
#NonoSolutionCount.exe: Source/NonoSolutionCount.c
#	gcc $(CFLAGS) -o$@ $^

# Nonogram solver program
$(NONO_SOLVER_EXE): $(SOURCE_FILES)
	gcc $(CFLAGS) -o$@ $^
# Create the solver vith analyzer run.
NonoSolver_Analyzer: $(SOURCE_FILES)
	gcc -fanalyzer -o$(NONO_SOLVER_EXE) $^

# Build object directory
$(O):
	mkdir $(O)
