#include "recorder.h"

#define DMICJPEG 'jpeg'

int comp_discover() {
	DMparams *params;
	int dmICStatus;
	int totalIC; 
	int idx;

	totalIC = dmICGetNum() ;
	dmParamsCreate(&params);

	fprintf(stderr, "info: found %d image converters\n", totalIC);
	for (idx = 0; idx < totalIC; idx++) {
		if (dmICGetDescription(idx, params) != DM_SUCCESS) {
			continue;
		}
		/* Look for the right combination */
		if ((dmParamsGetInt(params, DM_IC_ID) == DMICJPEG) &&
			(dmParamsGetEnum(params, DM_IC_CODE_DIRECTION) == DM_IC_CODE_DIRECTION_ENCODE) &&
			(dmParamsGetEnum(params, DM_IC_SPEED) == DM_IC_SPEED_REALTIME)) {
			if ((dmICStatus = dmICCreate(idx, &cstate.ic)) == DM_SUCCESS) {
				cstate.ic_idx = idx;
				fprintf(stderr, "debug: chosen ic as idx %d\n", idx);
				break;
			}
		}
	}
	dmParamsDestroy(params);
	fprintf(stderr, "debug: comp discovery complete\n");
	return 1;
}

int setup_converter_compressor() {

	DMparams *params;

	dmParamsCreate(&params);

	printf("image details: width %d, height %d\n", cstate.width,
			cstate.height);
	// setup params for the dmIC source
	if (dmSetImageDefaults(params, cstate.width, cstate.height,
			DM_IMAGE_PACKING_CbYCrY) != DM_SUCCESS) {
		fprintf(stderr, "error: dmSetImageDefaults\n");
		return 0;
	}	
	dmParamsSetEnum(params, DM_IMAGE_ORIENTATION,DM_IMAGE_TOP_TO_BOTTOM);
	dmParamsSetString(params, DM_IMAGE_COMPRESSION, DM_IMAGE_UNCOMPRESSED);

	if (DO_FRAME) {
            dmParamsSetEnum(params, DM_IMAGE_INTERLACING, DM_IMAGE_NONINTERLACED);
	    fprintf(stderr, "debug: do_frame\n");
        } else if (cstate.frameRate == 25.0) {
            dmParamsSetEnum(params, DM_IMAGE_INTERLACING, DM_IMAGE_INTERLACED_EVEN);
	    fprintf(stderr, "debug: field 25\n");
        } else {
            dmParamsSetEnum(params, DM_IMAGE_INTERLACING, DM_IMAGE_INTERLACED_ODD);
	    fprintf(stderr, "debug: field 30\n");
        }
	fprintf(stderr, "debug: setting src params\n");
	if (dmICSetSrcParams(cstate.ic, params) != DM_SUCCESS) {
		fprintf(stderr, "error: dmICSetSrcParams\n");
		return 0;
	}
	fprintf(stderr, "debug: set source ok\n");
	// setup params for the dmIC destination
	dmParamsSetString(params, DM_IMAGE_COMPRESSION, DM_IMAGE_JPEG);
	dmParamsSetFloat(params, DM_IMAGE_RATE, cstate.frameRate);
	if (DO_FRAME) {
           dmParamsSetEnum(params, DM_IMAGE_INTERLACING, DM_IMAGE_NONINTERLACED);
	    fprintf(stderr, "debug: do_frame\n");
        } else if (cstate.frameRate == 25.0) {
            dmParamsSetEnum(params, DM_IMAGE_INTERLACING, DM_IMAGE_INTERLACED_EVEN);
	    fprintf(stderr, "debug: field 25\n");
        } else {
            dmParamsSetEnum(params, DM_IMAGE_INTERLACING, DM_IMAGE_INTERLACED_ODD);
	    fprintf(stderr, "debug: field 30\n");
	}
	if (dmICSetDstParams(cstate.ic, params) != DM_SUCCESS) {
		fprintf(stderr, "error: dmICSetDstParam\n");
		return 0;
	}	
	dmParamsDestroy(params);

	fprintf(stderr, "compressor setup\n");
	return 1;
}

int setup_converter_configure() {
	DMparams *params;
	dmParamsCreate(&params);

	dmParamsSetFloat(params, DM_IMAGE_QUALITY_SPATIAL, 
		options.quality * DM_IMAGE_QUALITY_MAX / 100.0);

	if (dmICSetConvParams(cstate.ic, params) != DM_SUCCESS) {
		fprintf(stderr, "error: dmICSetConvParams\n");
		return 0;
	}	

	dmParamsDestroy(params);
	fprintf(stderr, "compressor configured\n");
	return 1;
}

int setup_converter_buffers() {
	DMparams *params;
	dmParamsCreate(&params);

	fprintf(stderr, "compressor inbufs = %d ; outbufs = %d buffers ok\n",
			buffer.inbufs, buffer.outbufs);
	// memdrn -> inpool .... dmIC .... outpool -> filewriter
	dmBufferSetPoolDefaults(params, buffer.inbufs, cstate.xferbytes,
			DM_TRUE, DM_TRUE);
	dmICGetSrcPoolParams(cstate.ic, params);
	vlDMGetParams(cstate.server, cstate.pathin, cstate.memdrn,
			params);
	if (dmBufferCreatePool(params, &(buffer.inpool)) != DM_SUCCESS) {
		fprintf(stderr, "error: dmBufferCreatePool inpool\n");
		return 0;
	}

	if (vlDMPoolRegister(cstate.server, cstate.pathin, cstate.memdrn,
			buffer.inpool) == -1) {
		fprintf(stderr, "error: vlDMPoolRegister() for dmIC input: %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}	
	dmParamsDestroy(params);

	dmParamsCreate(&params);
	dmBufferSetPoolDefaults(params, buffer.outbufs, cstate.xferbytes,
			DM_TRUE, DM_TRUE);
	dmICGetDstPoolParams(cstate.ic, params);
	if (dmBufferCreatePool(params, &(buffer.outpool)) != DM_SUCCESS) {
		fprintf(stderr, "error: dmBufferCreatePool outpool\n");
		return 0;
	}
	if (dmICSetDstPool(cstate.ic, buffer.outpool) != DM_SUCCESS) {
		fprintf(stderr, "error: dmICSetDstPool\n");
		return 0;
	}

	dmParamsDestroy(params);
	return 1;
}

int setup_converter() {

	if (! comp_discover()) {
		return 0;
	}
	if (! setup_converter_compressor()) {
		return 0;
	}
	if (! setup_converter_configure()) {
		return 0;
	}
	if (! setup_converter_buffers()) {
		return 0;
	}

	fprintf(stderr, "converter setup ok\n");	
	return 1;
}

int shutdown_converter() {

	if (vlDMPoolDeregister(cstate.server, cstate.pathin,
				cstate.memdrn, buffer.inpool) == -1) {
		fprintf(stderr, "warn: vlDMPoolDeregister()\n"); 
	}
	printf("debug: vlDMPoolDeregister pathin, memdrn, buffer inpool ok\n");
	if (dmBufferDestroyPool(buffer.inpool) != DM_SUCCESS) {
		fprintf(stderr, "warn: dmBufferDestroyPool()\n");
	}
	printf("debug: dmBufferDestroyPool inpool\n");
	if (dmBufferDestroyPool(buffer.outpool) != DM_SUCCESS) {
		fprintf(stderr, "warn: dmBufferDestroyPool()\n");
	}
	printf("debug: dmBufferDestroyPool outpool\n");

	dmICDestroy(cstate.ic);
       	return 1;
}
