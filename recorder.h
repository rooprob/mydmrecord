
#include <stdio.h>
#include <errno.h>
#include <limits.h>

/* include SGI dmedia includes */
#include <dmedia/vl.h>
#include <dmedia/dm_params.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

typedef struct {

	int		debug;
	int		verbose;

	char*		filename;
	long long 	duration;
	int 		quality;

} options_t;

typedef struct
{
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
    double      rate;
    int         frames;
    int         xferbytes;
//    DMimageconverter    ic;
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
	int		inbufs;
	int		inbufsize;
	
	DMbufferpool 	outpool;
	int 		outpoolfd;
	int		outbufs;
	int		outbufsize;

	DMbufferpool 	comppool;
	int 		comppoolfd;
	int		compbufs;
	int		compbufsize;
} buffer_t; 

options_t options;
cstate_t cstate ;
buffer_t buffer ;

