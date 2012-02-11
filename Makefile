include $(ROOT)/usr/include/make/commondefs

CFILES   = \
	main.c \
	$(NULL)

COBJS = $(CFILES:.c=.o)

TARGETS = dmrecord

LLDLIBS = -lmoviefile \
	  -lvl \
          -ldmedia \
          -lcl \
          -laudio \
	  -lgen \
	  -ldmalloc

default all: $(TARGETS)

include $(COMMONRULES)

DEFINES = -DDMALLOC \
	  -DDMALLOC_FUNC_CHECK
LDFLAGS_DMALLOC = -L/usr/people/oo/C/malloc/dmalloc.bin/lib

dmrecord: $(COBJS)
	$(CC) -o $@ $(DEFINES) $(COBJS) $(LDFLAGS) $(LDFLAGS) $(LDFLAGS_DMALLOC)
