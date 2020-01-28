SOURCES = main.cpp shared.cpp
OBJECTS = $(addprefix $(BUILD_DIR)/,$(SOURCES:.cpp=.o))
DEPS = $(addprefix $(BUILD_DIR)/,$(SOURCES:.cpp=.d))
EXE = $(addprefix $(BIN_DIR)/,prog)
CXXFLAGS = -I. 

ifeq (${ARCH},host)
CXX=g++
BUILD_DIR=../../build/host
BIN_DIR=../../bin/host
endif

ifeq (${ARCH},target)
CXX=arm-rpizw-g++
BUILD_DIR=../../build/target
BIN_DIR=../../bin/target
endif

.PHONY: all
all: $(EXE)

$(EXE): $(DEPS) $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) -lpthread -lrt

${BUILD_DIR}/%.d: %.cpp
	@mkdir -p $(@D)
	$(CXX) -MT$(@:.d=.o) -MM $(CXXFLAGS) $^ >> $@
#    $(CXX) -MT$@ -MM $(CXXFLAGS) $^ > $@

${BUILD_DIR}/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) -c $(CXXFLAGS) -o  $@ $<

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

.PHONY: clean
clean:
	@rm -f $(EXE) $(OBJ) $(DEPS)
.PHONY: help
help:
	@echo "use ARCH=host to compile for host or ARCH=target to compile for target"
