
#include <stdio.h>
#include <errno.h>
#include <limits.h>

/* include SGI dmedia includes */
#include <dmedia/vl.h>
#include <dmedia/dm_params.h>
#include <dmedia/dm_imageconvert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define UNIT_FRAME 1
#define UNIT_FIELD 0

#define DO_FRAME (options.unit == UNIT_FRAME)
#define DO_FIELD (options.unit != UNIT_FRAME)

typedef struct {
	int		debug;
	int		verbose;

	char*		filename;
	long long 	duration;
	int 		quality;

	int       	unit;             /* UNIT_FRAME or UNIT_FIELD */
} options_t;

typedef struct {
	VLServer    server;
	VLDevice	*viddevice;
	VLDev       videoKind; // VL_VIDEO (int video h/w)
	int         videoPort; // VL_ANY (int camera port)
	VLPath      pathin;
	int		pathinfd;
	VLPath      pathout;
	int		pathoutfd;
	VLPath      pathcomp;
	VLNode      vidsrc;
	VLNode      viddrn;
	VLNode      memdrn;
	VLNode      memsrc;

	int         width;
	int         height;
	double      frameRate;
	int         frames;
	int         xferbytes;
	int    	ic_idx;
	DMimageconverter    ic;
	int         icfd;
	int         fdominance;
	int         timing;
	int         originX;
	int         originY;
	long        waitPerBuffer;  /* microseconds to wait for a buffer (frame */
	/* or field) to arrive */
} cstate_t;

typedef struct {
	// dmBuffer* details
	DMbufferpool 	inpool;
	int		inpoolfd;
	int		inbufs ;
	int		inbufsize;

	DMbufferpool 	outpool;
	int 		outpoolfd;
	int		outbufs ;
	int		outbufsize;

	DMbufferpool 	comppool;
	int 		comppoolfd;
	int		compbufs;
	int		compbufsize;
} buffer_t; 

options_t options;
cstate_t cstate ;
buffer_t buffer ;
