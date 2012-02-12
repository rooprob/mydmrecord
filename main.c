

#include "recorder.h"


int main(int args, char ** argv) {

	options.verbose = 1;
	options.debug = 1;
	options.unit = UNIT_FIELD ;
	options.quality = 70.0;
	options.duration = 60;
	

	buffer.inbufs = 60;
	buffer.outbufs = 60;

	printf("Hello world. This is the recorder: verbose = %d, debug = %d\n", 
			options.verbose, options.debug);

	if ((cstate.server = vlOpenVideo("")) == NULL) {
		fprintf(stderr, "error: vlOpenVideo(): %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}

	capture(args, argv);

	if (vlCloseVideo(cstate.server) == -1) {
		fprintf(stderr, "error: vlCloseVideo(): %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}

	printf("successfully close video connection\n");
	return 1;
}
