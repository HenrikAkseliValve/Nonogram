CC?=gcc
CFLAGS:=-Wall -Wno-unused-result -std=gnu17
NONO_MAIN_EXE=NonoMain.exe
NONO_CONF_GEN_EXE=NonoConfGen.exe

# Check is debug enabled
DEBUG ?= 1
ifeq ($(DEBUG),1)
  CFLAGS+= -g -D_DEBUG_
else
  CFLAGS+= -O3 -D_RELEASE_
endif

SOURCE_FILES:=Source/NonoMain.c Source/Nonograms.c Source/LogicalRules.c Source/ValidityCheck.c Source/SwitchingComponent.c

SOURCE_FILES_NONO_CONG_GEN_EXE:=Source/NonoConfGen.c Source/Nonograms.c

PROFILE ?= 0
ifeq ($(PROFILE),1)
  CFLAGS+=-D_PROFILE_
  SOURCE_FILES+=Source/Profiling.c
endif

.PHONY: all
all: $(NONO_MAIN_EXE) $(NONO_CONF_GEN_EXE)

.PHONY: install
install:
	@echo "Not implemented."

.PHONY: clean
clean:
	-rm $(NONO_MAIN_EXE)
	-rm $(NONO_CONF_GEN_EXE)

# Nonogram solver program
$(NONO_MAIN_EXE): $(SOURCE_FILES)
	$(CC) $(CFLAGS) -o$@ $^
# Create the solver vith analyzer run.
NonoSolver_Analyzer: $(SOURCE_FILES)
	$(CC) -fanalyzer -o$(NONO_MAIN_EXE) $^

# Build nonogram configuration file generator.
$(NONO_CONF_GEN_EXE): $(SOURCE_FILES_NONO_CONG_GEN_EXE)
	$(CC) $(CFLAGS) -o$@ $^

# Build object directory
$(O):
	mkdir $(O)
