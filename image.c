#include "recorder.h"

int setup_image() {

	VLControlValue controlvalues;

	controlvalues.intVal = VL_PACKING_YVYU_422_8;
	if (vlSetControl(cstate.server, cstate.pathin, cstate.memdrn,
				VL_PACKING, &controlvalues) == -1) {
		fprintf(stderr, "error: setup_image vlSetControl(): %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	if (DO_FRAME) {
		controlvalues.intVal = VL_CAPTURE_INTERLEAVED;
		fprintf(stderr, "debug: img capture_interleaved\n");
	} else {
		controlvalues.intVal = VL_CAPTURE_NONINTERLEAVED;
		fprintf(stderr, "debug: img capture_noninterleaved\n");
	}
	if (vlSetControl(cstate.server, cstate.pathin, cstate.memdrn,
				VL_CAP_TYPE, &controlvalues) == -1) {
		fprintf(stderr, "error: setup_image vlSetControl(): %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	
	if (vlGetControl(cstate.server, 
				cstate.pathin, 
				cstate.vidsrc, 
				VL_SIZE, &controlvalues) == -1) {
		fprintf(stderr, "error: setup_image vlGetControl(): %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	// XXX video input source always in frame size

	// truncate width to multiples of 16
	cstate.width = ((controlvalues.xyVal.x) / 16) * 16;
	// h is frame height, rounded up to multiple of 16
	cstate.height = ((controlvalues.xyVal.y + 15) / 16) * 16;

	if (DO_FIELD) {
		cstate.height /= 2;
	}
	controlvalues.xyVal.x = cstate.width;
	controlvalues.xyVal.y = cstate.height;

	vlSetControl(cstate.server, cstate.pathin, cstate.memdrn,
			VL_SIZE, &controlvalues) ;

	cstate.xferbytes = cstate.width * cstate.height * 2;
	fprintf(stderr, "setup_image: image (%s) width = %d ; height = %d ; xferbytes = %d\n",
	       (DO_FRAME ? "frame" : "field"),
       		cstate.width, cstate.height, cstate.xferbytes);

	return 1;
}
