
# source codes basename
SACSRCNAME = gigo

# SNET_LIBS and SNET_MISC are environment variables
LIBDIRS   = -Llib -L$(SNET_LIBS)

# snet threading and sac targets should match (though the names may be different)
THREAD    = lpel
SACTARGET = $(THREAD)
LIBVARIANT = -$(SACTARGET)

# source config options: LPEL_INCLUDES, LIBLPEL_LA
include $(SNET_MISC)/snet-config.mkf

# sac2c compiles sac sources to binary modules
SAC2C     = sac2c
S2CFLAGS  = -v1 -O2 -g -mt -ccflag"$(LPEL_INCLUDES)" -target $(SACTARGET) -nophm

# sac4c generates c wrapper code for external calls of sac modules
SAC4C     = sac4c
S4CFLAGS  = -g -v1 -mt -ccflag"$(LPEL_INCLUDES) $(LIBLPEL_LA)" -target $(SACTARGET) -incdir include -libdir lib -g $(LIBDIRS) -nophm
S4CINCS   = `$(SAC4C) $(S4CFLAGS) -ccflags $(SACSRCNAME) | sed 's/-pthread//'`
S4CLIBS   = `$(SAC4C) $(S4CFLAGS) -ldflags $(SACSRCNAME) | sed 's/-pthread//'`

# sac user type number of the snet handle type
SACTYPE_SNet_SNet = $(shell grep "\#define SACTYPE_SNet_SNet" include/cwrapper.h | cut -f3 -d' ' )
# snet compiler flags
SNETCFLAGS  = -DSACTYPE_SNet_SNet=$(SACTYPE_SNet_SNet) $(S4CINCS) $(S4CLIBS) -threading $(THREAD)

# exe target name
TARGETS = gigo_1-lpel gigo_3-lpel

# main target;
# the snetc compiler for some reason links with any .o it can find,
# hence we cleanup before running it
%-$(THREAD) : sac-libs %.snet
	rm -rf .libs/*.o
	rm -rf *.o *.lo
	snetc $(SNETCFLAGS) $*.snet -o $@

all : $(TARGETS)

# dependency for the sac module and interface compilation
sac-libs : lib/lib$(SACSRCNAME)Mod$(LIBVARIANT).so lib/lib$(SACSRCNAME)Mod$(LIBVARIANT).a lib/lib$(SACSRCNAME)Tree$(LIBVARIANT).so

# this compile sac sources to modules with sac2c and generates the interface code with sac4c
lib/lib%Mod$(LIBVARIANT).so: %.sac
	mkdir -p lib include
	$(SAC2C) $(S2CFLAGS) $(LIBDIRS) $*.sac -o lib
	$(SAC4C) $(S4CFLAGS) $(LIBDIRS) $*

clean :
	rm -rf include lib *.h *.c *.lo *.o $(TARGETS) mon_n*_info.log
