################################################################################
# DeluxeReader Makefile
#
# Copyright (C) 2015, Loïc Le Page
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
################################################################################

#Build configuration
#
#Available options are:
#  BUILD=[debug] or release
#  PLATFORM=[linux] or mingw
#  ARCH=[64] or 32
#
BUILD?=debug
PLATFORM?=linux
ARCH?=64

ifneq ($(BUILD),debug)
    ifneq ($(BUILD),release)
        $(error Invalid BUILD type, please select "debug" or "release")
    endif
endif

ifneq ($(PLATFORM),linux)
    ifneq ($(PLATFORM),mingw)
        $(error Invalid PLATFORM type, please select "linux" or "mingw")
    endif
endif

ifneq ($(ARCH),64)
    ifneq ($(ARCH),32)
        $(error Invalid ARCH type, please select "64" or "32")
    endif
endif

#Project definitions
PROJECT_NAME=DeluxeReader

SRC_DIR=src
BUILD_DIR=build
BIN_DIR=bin
TEST_DIR=test

DOC_DIR=doc/generated
DOXYFILE=doc/Doxyfile

EXT_DIR=ext
TEST_FRAMEWORK=EasyTest++

SRC_MAIN_FILE=main.cpp
SRC_PRECOMP_HEADER=precomp.h

PACKAGE_EXTRA_FILES=LICENSE README.md

#Build paths and tools
ifeq ($(PLATFORM),linux)
    CXX=g++
else
    ifeq ($(ARCH),64)
        CXX=x86_64-w64-mingw32-g++
    else
        CXX=i686-w64-mingw32-g++
    endif
endif

LD=$(CXX)
MKDIR=mkdir -p
RMDIR=rmdir --ignore-fail-on-non-empty
RM=rm -rf
CP=cp -f
SED=sed
DOCGEN=doxygen
TAR=tar

#Compiler/Linker default options
CXXFLAGS+=-Wall -Werror -std=c++11 -pedantic -fno-rtti

ifeq ($(ARCH),64)
    CXXFLAGS+=-m64
    LDFLAGS+=-m64
else
    CXXFLAGS+=-m32
    LDFLAGS+=-m32
endif

ifeq ($(PLATFORM),linux)
    CXXFLAGS+=-pthread
    LDFLAGS+=-pthread
    LDLIBS=-lX11 -lGL\
           -larchive\
           -lmupdf -lfreetype -ljpeg -lopenjp2 -lz -ljbig2dec\
           -lpng\
           -lgif\
           -ltiff
else
    LDLIBS = -lopengl32
endif

ifeq ($(BUILD),debug)
    CXXFLAGS:=-g $(CXXFLAGS)
else
    CXXFLAGS:=-O3 -s -DNDEBUG $(CXXFLAGS)
    LDFLAGS:=-s $(LDFLAGS)
endif

ifeq ($(PLATFORM),mingw)
    LDFLAGS+=-static -mwindows
endif

#Build variables
TARGET=$(BIN_DIR)/$(PROJECT_NAME)_$(PLATFORM)$(ARCH)_$(BUILD)
OBJ_DIR=$(BUILD_DIR)/$(PLATFORM)$(ARCH)_$(BUILD)

TEST_TARGET=$(BIN_DIR)/Test_$(PROJECT_NAME)_$(PLATFORM)$(ARCH)_$(BUILD)
TEST_OBJ_DIR=$(BUILD_DIR)/test_$(PLATFORM)$(ARCH)_$(BUILD)

ifeq ($(PLATFORM),mingw)
    TARGET:=$(TARGET).exe
    TEST_TARGET:=$(TEST_TARGET).exe
endif

RECFIND_CPP=$(strip $(wildcard $1/*.cpp) $(foreach d, $(wildcard $1/*), $(call RECFIND_CPP, $d)))
SRC_FILES=$(call RECFIND_CPP, $(SRC_DIR))
TEST_FILES=$(call RECFIND_CPP, $(TEST_DIR)) $(filter-out $(SRC_DIR)/$(SRC_MAIN_FILE), $(SRC_FILES))

OBJ_FILES=$(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))
DEP_FILES=$(patsubst %.cpp, $(OBJ_DIR)/%.d, $(SRC_FILES))

TEST_OBJ_FILES=$(patsubst %.cpp, $(TEST_OBJ_DIR)/%.o, $(TEST_FILES))
TEST_DEP_FILES=$(patsubst %.cpp, $(TEST_OBJ_DIR)/%.d, $(TEST_FILES))

TEST_INCDIRS=-I$(EXT_DIR)/$(TEST_FRAMEWORK)/include
TEST_LIBDIRS=-L$(EXT_DIR)/$(TEST_FRAMEWORK)/bin
TEST_LDLIBS=-l$(TEST_FRAMEWORK)_$(PLATFORM)$(ARCH)_$(BUILD)
TEST_FRAMEWORK_LIBFILE=$(EXT_DIR)/$(TEST_FRAMEWORK)/bin/lib$(TEST_FRAMEWORK)_$(PLATFORM)$(ARCH)_$(BUILD).a

#Package variables
PACKAGE_NAME=$(BIN_DIR)/$(PROJECT_NAME).tgz
PACKAGE_BINS=$(BIN_DIR)/$(PROJECT_NAME)_linux??_release $(BIN_DIR)/$(PROJECT_NAME)_mingw??_release.exe
PACKAGE_MSC_BINS=$(BIN_DIR)/$(PROJECT_NAME)_msc??_release.exe

#Build targets and rules
.PHONY: all clean test clean-test doc clean-doc package clean-package clean-all

all: $(TARGET)

clean:
	$(info Cleaning $(BUILD) build on $(PLATFORM) $(ARCH) bits...)
	@$(RM) $(OBJ_DIR) $(TARGET)
	@-$(RMDIR) $(BIN_DIR) $(BUILD_DIR) 2>/dev/null

test: $(TEST_TARGET)

clean-test:
	$(info Cleaning $(BUILD) tests on $(PLATFORM) $(ARCH) bits...)
	@$(RM) $(TEST_OBJ_DIR) $(TEST_TARGET)
	@-$(RMDIR) $(BIN_DIR) $(BUILD_DIR) 2>/dev/null
	@$(MAKE) -C $(EXT_DIR)/$(TEST_FRAMEWORK) -f Makefile -w clean

doc:
	$(info Building documentation...)
	@$(DOCGEN) $(DOXYFILE)

clean-doc:
	$(info Cleaning documentation...)
	@$(RM) $(DOC_DIR)

package:
	$(info Packaging $(PACKAGE_NAME)...)
	@$(MAKE) --no-print-directory BUILD=release PLATFORM=linux ARCH=64
	@$(MAKE) --no-print-directory BUILD=release PLATFORM=linux ARCH=32
	@$(MAKE) --no-print-directory BUILD=release PLATFORM=mingw ARCH=64
	@$(MAKE) --no-print-directory BUILD=release PLATFORM=mingw ARCH=32
	@$(TAR) -czf $(PACKAGE_NAME) --no-acls --transform "s;_release;;" `ls -x $(PACKAGE_BINS) $(PACKAGE_MSC_BINS) 2>/dev/null` $(PACKAGE_EXTRA_FILES)

clean-package:
	$(info Cleaning package $(PACKAGE_NAME)...)
	@$(RM) $(PACKAGE_NAME)
	@-$(RMDIR) $(BIN_DIR) 2>/dev/null

clean-all:
	$(info Cleaning all builds, tests, packages and documentation on all platforms...)
	@$(RM) $(BIN_DIR) $(BUILD_DIR) $(DOC_DIR)
	@$(MAKE) -C $(EXT_DIR)/$(TEST_FRAMEWORK) -f Makefile -w clean-all

$(TARGET): $(OBJ_FILES)
	$(info Linking $(BUILD) binary on $(PLATFORM) $(ARCH) bits...)
	@$(MKDIR) $(@D)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBDIRS) $(LDLIBS)

$(TEST_FRAMEWORK_LIBFILE):
	@$(MAKE) -C $(EXT_DIR)/$(TEST_FRAMEWORK) -f Makefile -w

$(TEST_TARGET): $(TEST_OBJ_FILES) $(TEST_FRAMEWORK_LIBFILE)
	$(info Linking $(BUILD) tests binary on $(PLATFORM) $(ARCH) bits...)
	@$(MKDIR) $(@D)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBDIRS) $(TEST_LIBDIRS) $(LDLIBS) $(TEST_LDLIBS)

$(OBJ_DIR)/%.d: %.cpp
	@$(MKDIR) $(@D)
	@$(CXX) -MM $(CXXFLAGS) $(INCDIRS) $< 2>/dev/null | $(SED) "s#.*: *#$(OBJ_DIR)/$*.o $@: #g" > $@

$(TEST_OBJ_DIR)/%.d: %.cpp
	@$(MKDIR) $(@D)
	@$(CXX) -MM $(CXXFLAGS) $(INCDIRS) $(TEST_INCDIRS) $< 2>/dev/null | $(SED) "s#.*: *#$(TEST_OBJ_DIR)/$*.o $@: #g" > $@

ifeq ($(MAKECMDGOALS),test)
    -include $(TEST_DEP_FILES)
else ifneq ($(MAKECMDGOALS),doc)
    ifneq ($(MAKECMDGOALS),package)
        ifeq ($(findstring clean,$(MAKECMDGOALS)),)
            -include $(DEP_FILES)
        endif
    endif
endif

ifneq ($(strip $(SRC_PRECOMP_HEADER)),)
    $(OBJ_DIR)/$(SRC_PRECOMP_HEADER): $(SRC_DIR)/$(SRC_PRECOMP_HEADER)
	@$(MKDIR) $(@D)
	@$(CP) $< $@

    $(TEST_OBJ_DIR)/$(SRC_PRECOMP_HEADER): $(SRC_DIR)/$(SRC_PRECOMP_HEADER)
	@$(MKDIR) $(@D)
	@$(CP) $< $@

    PRECOMP_DEP=$(SRC_PRECOMP_HEADER:%.h=%.d)

    $(OBJ_DIR)/$(PRECOMP_DEP): $(OBJ_DIR)/$(SRC_PRECOMP_HEADER)
	@$(MKDIR) $(@D)
	@$(CXX) -MM $(CXXFLAGS) $(INCDIRS) $< 2>/dev/null | $(SED) "s#.*: *#$(OBJ_DIR)/$(SRC_PRECOMP_HEADER).gch $@: #g" > $@

    $(TEST_OBJ_DIR)/$(PRECOMP_DEP): $(TEST_OBJ_DIR)/$(SRC_PRECOMP_HEADER)
	@$(MKDIR) $(@D)
	@$(CXX) -MM $(CXXFLAGS) $(INCDIRS) $(TEST_INCDIRS) $< 2>/dev/null | $(SED) "s#.*: *#$(TEST_OBJ_DIR)/$(SRC_PRECOMP_HEADER).gch $@: #g" > $@

    ifeq ($(MAKECMDGOALS),test)
        -include $(TEST_OBJ_DIR)/$(PRECOMP_DEP)
    else ifneq ($(MAKECMDGOALS),doc)
        ifneq ($(MAKECMDGOALS),package)
            ifeq ($(findstring clean,$(MAKECMDGOALS)),)
                -include $(OBJ_DIR)/$(PRECOMP_DEP)
            endif
        endif
    endif

    $(OBJ_DIR)/$(SRC_PRECOMP_HEADER).gch: $(OBJ_DIR)/$(SRC_PRECOMP_HEADER)
	@$(MKDIR) $(@D)
	$(CXX) $(CXXFLAGS) $(INCDIRS) -o $@ -c $<

    $(TEST_OBJ_DIR)/$(SRC_PRECOMP_HEADER).gch: $(TEST_OBJ_DIR)/$(SRC_PRECOMP_HEADER)
	@$(MKDIR) $(@D)
	$(CXX) $(CXXFLAGS) $(INCDIRS) $(TEST_INCDIRS) -o $@ -c $<

    $(OBJ_DIR)/%.o: %.cpp $(OBJ_DIR)/$(SRC_PRECOMP_HEADER).gch
	@$(MKDIR) $(@D)
	$(CXX) $(CXXFLAGS) $(INCDIRS) -include $(OBJ_DIR)/$(SRC_PRECOMP_HEADER) -o $@ -c $<

    $(TEST_OBJ_DIR)/%.o: %.cpp $(TEST_OBJ_DIR)/$(SRC_PRECOMP_HEADER).gch
	@$(MKDIR) $(@D)
	$(CXX) $(CXXFLAGS) $(INCDIRS) $(TEST_INCDIRS) -include $(TEST_OBJ_DIR)/$(SRC_PRECOMP_HEADER) -o $@ -c $<
else
    $(OBJ_DIR)/%.o: %.cpp
	@$(MKDIR) $(@D)
	$(CXX) $(CXXFLAGS) $(INCDIRS) -o $@ -c $<

    $(TEST_OBJ_DIR)/%.o: %.cpp
	@$(MKDIR) $(@D)
	$(CXX) $(CXXFLAGS) $(INCDIRS) $(TEST_INCDIRS) -o $@ -c $<
endif
