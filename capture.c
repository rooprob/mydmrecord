
#include "recorder.h"

#include "dmedia/vl_mvp.h"
#include <signal.h>

void cb_ProcessEvent(VLServer, VLEvent *, void *);

void cb_ProcessEvent(VLServer server, VLEvent *event, void * clientData) {
	printf("made the callback!\n");
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
/* input to memory memdrn */
int setup_input() {
	VLControlValue controlvalue;

	VLCallbackProc callback ;
	DMparams *params;

	buffer.inbufs = 30 * 2 ;
	buffer.inbufsize = 640 * 240 * 8 ;

	vidsrc_discovery();
	// camera input ...
	if ((cstate.vidsrc = vlGetNode(cstate.server, VL_SRC, cstate.videoKind, cstate.videoPort)) == -1) {
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

	// memory input ...
	if ((cstate.memsrc = vlGetNode(cstate.server, VL_SRC, VL_MEM, VL_ANY)) == -1) {
		fprintf(stderr, "error: vlGetNode() memsrc: %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	// ... into video drain
	if ((cstate.viddrn = vlGetNode(cstate.server, VL_DRN, VL_VIDEO, VL_ANY)) == -1) {
		fprintf(stderr, "error: vlGetNode() viddrn: %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	// establish input path from memory to video
	if ((cstate.pathout = vlCreatePath(cstate.server,
					VL_ANY,
					cstate.memsrc, cstate.viddrn)) == -1) {
		fprintf(stderr, "error: vlCreatePath(): %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	// Setup a path lst pathin, pathout,...	
	if (vlSetupPaths(cstate.server, (VLPathList)&(cstate.pathin), 1, VL_SHARE, VL_SHARE) == -1) {
		fprintf(stderr, "error: vlSetupPaths() pathin: %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	if (vlSetupPaths(cstate.server, (VLPathList)&(cstate.pathout), 1, VL_SHARE, VL_SHARE) == -1) {
		fprintf(stderr, "error: vlSetupPaths() pathout: %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}

	// set the output drains timing back on the input source timing
	if (vlGetControl(cstate.server, cstate.pathin, cstate.vidsrc,
			       	VL_TIMING, &controlvalue) == 0) {
		if (vlSetControl(cstate.server, cstate.pathout, cstate.viddrn,
					VL_TIMING, &controlvalue) == -1) {
			fprintf(stderr, "error: vlSetControl(): %s\n", 
						vlStrError(vlGetErrno()));
			return 0;
		}
	}
	cstate.timing = controlvalue.intVal ;
	printf("info: cstate timing %d\n", cstate.timing);
	if ((cstate.timing == VL_TIMING_625_SQ_PIX) 
			|| (cstate.timing == VL_TIMING_625_CCIR601)) {
		printf("info: 25.0 fps\n");
	} else {
		printf("info: 29.97 fps\n");
	}
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

	buffer.inbufsize = vlGetTransferSize(cstate.server,
			cstate.pathin);
	printf("x = %d ; y = %d ; transfersize = %d\n", 
			controlvalue.xyVal.x,
			controlvalue.xyVal.y,
			buffer.inbufsize);

	if (dmParamsCreate(&params) != DM_SUCCESS) {
		fprintf(stderr, "error dmParamsCreate()\n");
		return 0;
	}

	if (dmBufferSetPoolDefaults(params, buffer.inbufs, buffer.inbufsize, 
				DM_TRUE, DM_TRUE) != DM_SUCCESS) {
		fprintf(stderr, "error: dmBufferSetPoolDefaults():\n");
		return 0;
	}
	if (vlDMGetParams(cstate.server, cstate.pathin, cstate.memsrc, params) == -1) {
		fprintf(stderr, "error: vlDMGetParams() vidsrc: %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	printf("memsrc: buffer_count: %d\n", dmParamsGetInt(params, DM_BUFFER_COUNT));
	// create DM buffer
	if (dmBufferCreatePool(params, &(buffer.inpool)) != DM_SUCCESS) {
		fprintf(stderr, "error: dmBufferCreatePool():\n");
		return 0;
	}
	dmParamsDestroy(params);

	if (vlDMPoolRegister(cstate.server, cstate.pathin, cstate.memdrn,
				buffer.inpool) == -1) {
		fprintf(stderr, "error: vlDMPoolRegister():\n");
		return 0;
	}

	/* Begin the data transfer */
	/*
	if(vlBeginTransfer(cstate.server, cstate.pathout, 0, NULL) == -1) {
		vlPerror("begin transfer out");
		return 1;
	}*/
	if(vlBeginTransfer(cstate.server, cstate.pathin, 0, NULL) == -1) {
		vlPerror("begin transfer in");
		return 1;
	}

	// register callback
	if (vlAddCallback(cstate.server, cstate.pathin, VLAllEventsMask,
				cb_ProcessEvent, NULL) == -1) {
		
		fprintf(stderr, "error: vlDMGetParams() memdrn: %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}

	printf("info: setup complete successfully\n");
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

	if (vlDMPoolDeregister(cstate.server, cstate.pathin,
				cstate.vidsrc, buffer.inpool) == -1) {
		fprintf(stderr, "warn: vlDMPoolDeregister()\n"); 
	}
	if (vlDMPoolDeregister(cstate.server, cstate.pathin,
				cstate.vidsrc, buffer.outpool) == -1) {
		fprintf(stderr, "warn: vlDMPoolDeregister()\n"); 
	}
	if (dmBufferDestroyPool(buffer.outpool) != DM_SUCCESS) {
		fprintf(stderr, "warn: dmBufferDestroyPool()\n");
	}

	if (vlDestroyPath(cstate.server, cstate.pathin) == -1) {
		fprintf(stderr, "error: vlDestroyPath(): %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}
	if (vlDestroyPath(cstate.server, cstate.pathout) == -1) {
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
	if (! setup_input()) {
		return 0;
	}

	printf("starting capture; device = %s, videoPort = %d\n",
			(cstate.viddevice)->name,
			cstate.videoPort);

	if (vlBeginTransfer(cstate.server, cstate.pathin, 0, NULL) == -1) {
		fprintf(stderr, "error: vlBeginTransfer(): %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}

	vlMainLoop(); 

	if (! shutdown()) {
		return 0;
	}
	return 1;
}
