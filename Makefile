
# Dependencies and directories. --------------------------------------------------------

# 1. SAC

#   request and download from http://ds.iris.edu/ds/nodes/dmc/forms/sac/, compile and install to some directory.
#   a copy can be found in ./Dependencies/

# if configured with "$ ./configure --prefix=/usr/local/sac", then the directory will be:
SACDIR    := /usr/local/sac/


# 2. fftw3. (version 3.3.8)

#   A. (recommended) use package management softwares (apt-get, dnf, yum, brew, macport) search/install "fftw3".
#
#   or ..
#
#   B. download from http://www.fftw.org/download.html and compile, install to some directory.
#      a copy can be found in ./Dependencies/
#      if installed this way, you need to set a FFTWDIR, and modified this Makefile accordingly (see how SACDIR and GMT5DIR were used).
#      
#


# 3. GMT5. (version 5.4.5)

#   A. use package management softwares to install gmt5:
#      
#      Mac (recommended, Homebrew, https://brew.sh/):
#      
#         $ brew install gmt@5
#         
#      Mac (MacPorts, https://www.macports.org/):
#         
#         $ sudo port install gdal +curl +geos +hdf5 +netcdf
#         $ sudo port install gmt5
#    
#      Linux (not recommended because package managment softwares may stop providing gmt5, current version is gmt6):
#      
#         $ sudo apt-get install gmt-dcw gmt-gshhg
#         $ sudo apt-get install gmt
#
#   or ...
#
#   B. (recommended for Linux) download from https://github.com/GenericMappingTools/gmt/releases/tag/5.4.5, compile and install to some directory.
#

# if installed using Homebrew, by default the directory is:
GMT5DIR   := /usr/local/Cellar/gmt@5/5.4.5_4


# --------------------------------------------------------------------------------------



COMP      := c++ -std=c++14 -Wall
OUTDIR    := .
INCDIR    := -I./CPP-Library-Headers -I$(SACDIR)/include -I$(GMT5DIR)/include
LIBDIR    :=  -L$(SACDIR)/lib -L$(GMT5DIR)/lib
LIBS      := -lgmt -lfftw3 -lsac -lsacio -lm


# all *cpp files
SRCFILES  := $(wildcard *.cpp)
DEPFILES  := $(patsubst %.cpp, $(OUTDIR)/%.d, $(SRCFILES))
OBJS      := $(patsubst %.d, %.o, $(DEPFILES))

# main files
MAINS     := $(filter-out %.fun.cpp, $(SRCFILES))
EXEFILES  := $(patsubst %.cpp, $(OUTDIR)/%.out, $(MAINS))

# function files
FUNFILES  := $(wildcard *fun.cpp)
FUNOBJS   := $(patsubst %.cpp, $(OUTDIR)/%.o, $(FUNFILES))

all: $(EXEFILES) $(OBJS)
	@echo > /dev/null

# Resolve dependencies automatically.
-include $(DEPFILES)

%.out: %.o $(FUNOBJS)
	@echo "Updating: $@ ..."
	@$(COMP) -o $@ $^ $(INCDIR) $(LIBDIR) $(LIBS)

$(OUTDIR)/%.o: %.cpp
	@echo "Updating: $@ ..."
	@$(COMP) -MD -MP -c $< -o $@ $(INCDIR)

clean:
	rm -f $(OUTDIR)/*.out $(OUTDIR)/*.o $(OUTDIR)/*.d
