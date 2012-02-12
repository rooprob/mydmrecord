
#include "recorder.h"

#include "dmedia/vl_mvp.h"
#include <signal.h>

void cb_ProcessEventToIC(VLServer, VLEvent *, void *);
void cb_ProcessEventFromIC(VLServer, VLEvent *, void *);

/* Get the start time (for frame rate measurement) */
struct timeval  tv_save;
void initStatistics(void) {
    gettimeofday(&tv_save);
}
/* Calculate and display the frame rate */
void reportStatistics(void) {
    double rate;
    struct timeval tv;
    int delta_t;

    gettimeofday(&tv);

    /* Delta_t in microseconds */
    delta_t = (tv.tv_sec*1000 + tv.tv_usec/1000) -
              (tv_save.tv_sec*1000 + tv_save.tv_usec/1000);

    rate = cstate.frameRate*1000.0/delta_t;

    printf("got %5.2f %s/sec\n", rate, "frames");
    tv_save.tv_sec = tv.tv_sec;
    tv_save.tv_usec = tv.tv_usec;
}


static void sigint(void) {
	shutdown();
}
int vidsrc_discovery() {

	VLDevice *selectedDevice;
	VLDevList deviceList;
	VLNodeInfo *node;
	int idx;

	if (vlGetDeviceList(cstate.server, &deviceList) == -1) {
		fprintf(stderr, "error: vlGetDeviceList(): %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	// iterate of list of discovered devices
	for (idx = 0; idx < deviceList.numDevices ; idx++) {
		selectedDevice = &(deviceList.devices[idx]);
		printf("device: %d %s\n", idx, selectedDevice->name);
		if (strcmp(selectedDevice->name, "mvp") == 0) {
			break;
		}
	}

	// store device pointer
	cstate.viddevice = selectedDevice;

	printf("vidsrc device: name = %s ; numNodes = %d ; dev = %d\n",
		selectedDevice->name, selectedDevice->numNodes,
		selectedDevice->dev);

	for (idx = 0; idx < selectedDevice->numNodes; idx ++) {
		node = &(selectedDevice->nodes[idx]) ;
		if (node->type == VL_SRC && node->number == VL_MVP_VIDEO_SOURCE_CAMERA) {
			printf("vidsrc node: port name = %s ; number = %d ; kind = %d\n",
			       node->name, node->number, node->kind);	
			cstate.videoKind = node->kind ;
			cstate.videoPort = node->number ;
			break;
		}
	}

	return 1;
}

int get_video_rate() {

	VLControlValue controlvalue;

	vlGetControl(cstate.server, cstate.pathin, cstate.vidsrc,
			VL_TIMING, &controlvalue) ;
	cstate.timing = controlvalue.intVal ;
	printf("info: cstate timing %d\n", cstate.timing);
	if ((cstate.timing == VL_TIMING_625_SQ_PIX) 
			|| (cstate.timing == VL_TIMING_625_CCIR601)) {
		cstate.frameRate = 25;
		printf("videorate: 25.0 fps\n");
	} else {
		cstate.frameRate = 29.97 ;
		printf("videorate: 29.97 fps\n");
	}

	cstate.waitPerBuffer = (long) (1000000.0 / cstate.frameRate) ;
	if (DO_FIELD)
		cstate.waitPerBuffer /= 2;
	return 1;
}

/* input to memory memdrn */
int setup_input_paths() {

	if (!vidsrc_discovery()) {
		return 0;
	}

	// camera input ...
	if ((cstate.vidsrc = vlGetNode(cstate.server, VL_SRC, 
					cstate.videoKind, cstate.videoPort)) == -1) {
		fprintf(stderr, "error: vlGetNode() vidsrc: %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	// ... into memory drain
	if ((cstate.memdrn = vlGetNode(cstate.server, VL_DRN, VL_MEM, VL_ANY)) == -1) {
		fprintf(stderr, "error: vlGetNode() memdrn: %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	// establish input path from camera to memory
	if ((cstate.pathin = vlCreatePath(cstate.server,
					(cstate.viddevice)->dev,
					cstate.vidsrc, cstate.memdrn)) == -1) {
		fprintf(stderr, "error: vlCreatePath(): %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}

	// Setup a path lst pathin
	if (vlSetupPaths(cstate.server, (VLPathList)&(cstate.pathin), 1, VL_SHARE, VL_SHARE) == -1) {
		fprintf(stderr, "error: vlSetupPaths() pathin: %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}


	// XXX where did this come from?
	/*
	controlvalue.intVal = VL_PACKING_RGB_8;
	vlSetControl(cstate.server, cstate.pathin, cstate.memdrn,
			VL_PACKING, &controlvalue) ;
	vlSetControl(cstate.server, cstate.pathout, cstate.memsrc,
			VL_PACKING, &controlvalue) ;
	controlvalue.intVal = VL_FORMAT_RGB;
	vlSetControl(cstate.server, cstate.pathin, cstate.memdrn,
			VL_FORMAT, &controlvalue) ;
	vlSetControl(cstate.server, cstate.pathout, cstate.memsrc,
			VL_FORMAT, &controlvalue) ;
	vlGetControl(cstate.server, cstate.pathin, cstate.memdrn,
			VL_SIZE, &controlvalue) ;
	*/

	return 1;

}
/*
int setup_xfer_buffers() {

	DMparams *params;

	buffer.inbuf = 60 ; // XXX just made this up, approx 2 seconds
	buffer.inbufsize = cstate.xferbytes;
	printf("xfer_buffers x = %d ; y = %d ; xferbytes = %d\n", 
			cstate.width,
			cstate.height,
			cstate.xferbyes);

	if (dmParamsCreate(&params) != DM_SUCCESS) {
		fprintf(stderr, "error dmParamsCreate()\n");
		return 0;
	}
	if (dmBufferSetPoolDefaults(params, buffer.inbufs, buffer.inbufsize, 
				DM_TRUE, DM_TRUE) != DM_SUCCESS) {
		fprintf(stderr, "error: dmBufferSetPoolDefaults():\n");
		return 0;
	}
	if (dmICGetSrcPoolParams(cstate.ic, params)) != DM_SUCCESS) {
		fprintf(stderr, "error: dmICGetSrcPoolParams():\n");
		return 0;
	}
	if (vlDMGetParams(cstate.server, cstate.pathin, cstate.memdrn, params) == -1) {
		fprintf(stderr, "error: vlDMGetParams() memdrn: %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	printf("memdrn: buffer_count: %d\n", dmParamsGetInt(params, DM_BUFFER_COUNT));
	// create DM buffer
	if (dmBufferCreatePool(params, &(buffer.inpool)) != DM_SUCCESS) {
		fprintf(stderr, "error: dmBufferCreatePool():\n");
		return 0;
	}
	if (vlDMPoolRegister(cstate.server, cstate.pathin, cstate.memdrn,
				buffer.inpool) == -1) {
		fprintf(stderr, "error: vlDMPoolRegister():\n");
		return 0;
	}
	dmParamsDestroy(params);
}

*/

int register_callbacks() {
	int one = 1;

	// receive callbacks
	vlSelectEvents(cstate.server, cstate.pathin, VLAllEventsMask);
	// register callback
	if (vlAddCallback(cstate.server, cstate.pathin, VLAllEventsMask,
				cb_ProcessEventToIC, &one) == -1) {
		fprintf(stderr, "error: vlAddCallback() pathin: %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	fprintf(stderr, "debug: callbacks ok\n");
	return 1;
}

int begin_data_transfer() {

	VLTransferDescriptor xferDesc;

	xferDesc.mode = VL_TRANSFER_MODE_DISCRETE;
	xferDesc.count = 60;
	xferDesc.delay = 0;
	xferDesc.trigger = VLTriggerImmediate;

	if(vlBeginTransfer(cstate.server, cstate.pathin, 1, &xferDesc) == -1) {
		fprintf(stderr, "error: vlBeginTransfer(): pathin\n");
		return 0;
	}

	printf("info: data_transfer ok\n");
	return 1;
}

int shutdown () {
	int transferSize ;
	if ((transferSize = vlGetTransferSize(cstate.server,
					cstate.pathin)) == -1) {
		fprintf(stderr, "warn: vlGetTransferSize(): %s\n", 
					vlStrError(vlGetErrno()));
	}
	printf("info: transfered %d\n", transferSize);

	shutdown_converter();

	if (vlDestroyPath(cstate.server, cstate.pathin) == -1) {
		fprintf(stderr, "error: vlDestroyPath(): %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	printf("info: shutdown complete successfully\n");
	exit(0);
	return 1;
}

int capture(int args, char ** argv) {

	sigset(SIGINT, sigint);

	printf("setting up device; verbose = %d\n", options.verbose);
	if (! setup_input_paths()) {
		return 0;
	}
	if (! setup_image()) {
		return 0;
	}
	if (! get_video_rate()) {
		return 0;
	}
	if (! setup_converter()) {
		return 0;
	}
	if (! register_callbacks()) {
		return 0;
	}

	initStatistics();
	printf("starting capture; device = %s, videoPort = %d\n",
			(cstate.viddevice)->name,
			cstate.videoPort);

	if (! begin_data_transfer()) {
		return 0;
	}

	vlMainLoop(); 

	// wont get here
	if (! shutdown()) {
		return 0;
	}
	return 1;
}
