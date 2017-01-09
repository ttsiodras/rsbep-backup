bin_PROGRAMS = rsbep rsbep_chopper

if HAVE_X86ASM
rsbep_CFLAGS = -DASM_I386
rsbep_SOURCES = rsbep.c rs32.c rs32asm.s rs32.h
else
rsbep_CFLAGS = 
rsbep_SOURCES = rsbep.c rs.c rs.h
endif

rsbep_chopper = chopper.c

