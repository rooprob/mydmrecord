#include "recorder.h"


void cb_ProcessEventToIC(VLServer server, VLEvent *ev, void * clientData) {

	char *dataPtr;
	DMbuffer dmbuf;
	int dataBytes;

	int *data = (int *)clientData ;


	switch (ev->reason) {
		case VLTransferComplete:
			printf("CBTO %s\n", vlEventToName(ev->reason));
			if (vlDMBufferGetValid(cstate.server, cstate.pathin, 
						cstate.memdrn, &dmbuf) == VLSuccess) {
				dataBytes = dmBufferGetSize(dmbuf);
				printf("VLTransferComplete: %d\n", dataBytes);

				/*
				if (dmICSend(cstate.ic, dmbuf, 0, NULL)
						!= DM_SUCCESS) {
					fprintf(stderr, "failed to send to IC\n");
					do_cleanup();
				} else {
					fprintf(stderr, ">> IC %d\n", dataBytes);
				}
				*/
				dmBufferFree(dmbuf);
			} else {
				fprintf(stderr, "VLTransferComplete: but vlDMBufferGetValid: %s\n", 
						vlStrError(vlGetErrno()));
			}
		break;
		case VLStreamStopped:
			printf("all stop\n");
			do_cleanup() ;
		break;
		case VLSequenceLost:
			printf("Aiiieee - seq lost\n");
			do_cleanup() ;
		break;
		default:
		break;
	}

	if (dmICReceive(cstate.ic, &dmbuf) == DM_SUCCESS) {
		dataBytes = dmBufferGetSize(dmbuf);
		fprintf(stderr, "<< IC %d\n", dataBytes);
		dmBufferFree(dmbuf);
	} else {
		fprintf(stderr, "dmICReceive: no data\n");
	}

	reportStatistics();
}
void cb_ProcessEventFromIC(VLServer server, VLEvent *ev, void * clientData) {

	char *dataPtr;
	DMbuffer dmbuf;
	int dataBytes;

	int *data = (int *)clientData ;

	printf("From IC made the in %d callback! %s \n", *data, vlEventToName(ev->reason));
	/// reportStatistics();

	switch (ev->reason) {
		case VLTransferComplete:
			while(vlDMBufferGetValid(
				cstate.server, cstate.pathout,
				cstate.memdrn, &dmbuf) == VLSuccess) {

				dataBytes = dmBufferGetSize(dmbuf);
				fprintf(stderr, "from IC dmbuf ok %d bytes at %d\n",
						dataBytes, &dmbuf);

				dmBufferFree(dmbuf);
			}

		break;
		case VLStreamStopped:
			printf("all stop\n");
			do_cleanup() ;
		break;
		case VLSequenceLost:
			printf("Aiiieee - seq lost\n");
			do_cleanup() ;
		break;
		default:
		break;
	}
}

int do_cleanup() {

	vlEndTransfer(cstate.server, cstate.pathin) ;
	vlEndTransfer(cstate.server, cstate.pathout) ;
	shutdown();

	return 1;
}
