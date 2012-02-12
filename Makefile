include $(ROOT)/usr/include/make/commondefs

CFILES   = \
	main.c \
	capture.c \
	converter.c \
	callback.c \
	image.c \
	$(NULL)

COBJS = $(CFILES:.c=.o)

TARGETS = dmrecord

LLDLIBS =  -lvl \
	   -ldmedia \
	   -lcl \
	   -ldmalloc

default all: $(TARGETS)

include $(COMMONRULES)

DEFINES = -DDMALLOC \
	  -DDMALLOC_FUNC_CHECK
LDFLAGS_DMALLOC = -L/usr/people/oo/C/malloc/dmalloc.bin/lib

dmrecord: $(COBJS)
	$(CC) -o $@ $(DEFINES) $(COBJS) $(LDFLAGS) $(LDFLAGS) $(LDFLAGS_DMALLOC)

leaktest: leaktest.c
	@echo $(CC) -o $@ $(DEFINES) $(COBJS) $(LDFLAGS) $(LDFLAGS) $(LDFLAGS_DMALLOC)
