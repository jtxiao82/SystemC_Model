SYSTEMC_HOME = /usr/local/systemc-2.3.1

SYSTEMC_LIB = lib-linux64
###Use following library if you want to compile 32-bit binary
#SYSTEMC_LIB = lib-linux 

OPTIMIZE_FLAG=-O0
DEBUG_FLAG=-g
CXXFLAGS = ${OPTIMIZE_FLAG} -Wall ${DEBUG_FLAG}
LDFLAGS = -Wl,-L/lib,-L/usr/lib/

LDLIBS = -lsystemc 

###Use following flags if you want to compile 32-bit binary
#CXXFLAGS = ${OPTIMIZE_FLAG} -Wall -m32 -Wa,--32 ${DEBUG_FLAG}
#LDFLAGS = -m32 -Wl,-L/lib,-L/usr/lib/

### If you want to keep the compilation/linking errors ###
#COMPILER_ERROR = 2> compiler.error
COMPILER_ERROR = 
#LINKER_ERROR = 2> linker.error
LINKER_ERROR = 

#Included headers 
INCLUDE_DIRS = -I. 
INCLUDE_DIRS += -I"$(SYSTEMC_HOME)/include" 

#Included libraries
LIB_DIRS = -L. 
LIB_DIRS += -L"$(SYSTEMC_HOME)/$(SYSTEMC_LIB)" 
 
# User defined parameters
CXXFLAGS+=$(USER_DEFINE_FLAGS)

### Definitions ###
executive= main
sources= main.cpp
objects= $(subst .cpp,.o,$(sources))

### what to do? ###
all: $(executive)

$(executive) : $(objects)
	$(CXX) $(LDFLAGS) $(LIB_DIRS) $(objects) -o $@ $(LINKER_ERROR) $(LDLIBS) 

%.o: %.cpp
	$(CXX) $(CXXFLAGS) \
	$(INCLUDE_DIRS) -c $< -o $@ \
	$(COMPILER_ERROR)

main.o: 

### phony ###
clean: 
	rm -f $(objects) $(executive) compiler.error linker.error *.txt main *.o
