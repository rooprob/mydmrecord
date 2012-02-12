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
			if ((dmICStatus = dmICCreate(idx, &(cstate.ic))) == DM_SUCCESS) {
				dmICDestroy(cstate.ic);
				cstate.ic_idx = idx;
				fprintf(stderr, "debug: chosen ic as idx %d\n", idx);
				break;
			}
		}
	}
	dmParamsDestroy(params);
	return 1;
}

int setup_converter_compressor() {

	DMparams params;

	dmParamsCreate(&params);

	// setup params for the dmIC source
	dmSetImageDefaults(params, cstate.width, cstate.height,
			DM_IMAGE_PACKING_CbYCrY);
	dmParamsSetEnum(params,DM_IMAGE_ORIENTATION,DM_IMAGE_TOP_TO_BOTTOM);
	dmParamsSetString(params, DM_IMAGE_COMPRESSION, DM_IMAGE_UNCOMPRESSED);

	if (DO_FRAME) {
            dmParamsSetEnum(params, DM_IMAGE_INTERLACING, DM_IMAGE_NONINTERLACED);
        } else if (cstate.frameRate == 25.0) {
            dmParamsSetEnum(params, DM_IMAGE_INTERLACING, DM_IMAGE_INTERLACED_EVEN);
        } else {
            dmParamsSetEnum(params, DM_IMAGE_INTERLACING, DM_IMAGE_INTERLACED_ODD);
        }
	if (dmICSetSrcParams(cstate.ic, params) != DM_SUCCESS) {
		fprintf(stderr, "error: dmICSetSrcParams setup_converter_buffer\n");
		return 0;
	}	

	// setup params for the dmIC destination
	dmParamsSetString(params, DM_IMAGE_COMPRESSION, DM_IMAGE_JPEG);
	dmParamsSetFloat(params, DM_IMAGE_RATE, cstate.frameRate);
	if (DO_FRAME) {
           dmParamsSetEnum(params, DM_IMAGE_INTERLACING, DM_IMAGE_NONINTERLACED);
        } else if (cstate.frameRate == 25.0) {
            dmParamsSetEnum(params, DM_IMAGE_INTERLACING, DM_IMAGE_INTERLACED_EVEN);
        } else {
            dmParamsSetEnum(params, DM_IMAGE_INTERLACING, DM_IMAGE_INTERLACED_ODD);
	}
	if (dmICSetDstParams(cstate.ic, params) != DM_SUCCESS) {
		fprintf(stderr, "error: dmICSetDstParam ssetup_converter_buffer\n");
		return 0;
	}	
	dmParamsDestroy(params);


	return 1;
}

int setup_converter_paramters() {
	DMparams params;
	dmParamsCreate(&params);

	dmParamsSetFloat(params, DM_IMAGE_QUALITY_SPATIAL, 
		options.quality * DM_IMAGE_QUALITY_MAX / 100.0);

	if (dmICSetConvParams(cstate.ic, params) != DM_SUCCESS) {
		fprintf(stderr, "error: dmICSetDstParam ssetup_converter_buffer\n");
		return 0;
	}	

	dmParamsDestroy(params);
	return 1;
}

int setup_converter_buffers() {
	DMparams params;
	dmParamsCreate(&params);

	// memdrn -> inpool .... dmIC .... outpool -> filewriter
	dmBufferSetPoolDefaults(params, buffer.inbufs, cstate.xferbytes,
			DM_TRUE, DM_TRUE);
	dmICGetSrcPoolParams(cstate.ic, params);

	vlDMGetParams(cstate.server, cstate.pathin, cstate.memdrn,
			params);
	dmBufferCreatePool(params, &(buffer.inpool));

	if (vlDMPoolRegister(cstate.server, cstate.pathin, cstate.memdrn,
			buffer.inpool) != -1) {
		fprintf(stderr, "error: vlDMPoolRegister() for dmIC input: %s\n", 
					vlStrError(vlGetErrno()));
		return 0;
	}	
	dmParamsDestroy(params);

	dmParamsCreate(&params);
	dmBufferSetPoolDefaults(params, buffer.outbufs, cstate.xferbytes,
			DM_TRUE, DM_TRUE);
	dmICGetDstPoolParams(cstate.ic, params);
	dmBufferCreatePool(params, &(buffer.inpool));
	
	if (dmICSetDstPool(cstate.ic, buffer.outpool) != DM_SUCCESS) {
		fprintf(stderr, "error: dmICSetDstPool\n");
		return 0;
	}

	dmParamsDestroy(params);
	return 1;
}

int setup_converter() {

	if (comp_discover()) {
		return 0;
	}
	if (dmICCreate(cstate.ic_idx, &(cstate.ic)) != DM_SUCCESS) {
		fprintf(stderr, "error: dmICCreate\n");
		return 0;
	}

	if (setup_converter_compressor()) {
		return 0;
	}
	if (setup_converter_parameters()) {
		return 0;
	}
	if (setup_converter_buffers()) {
		return 0;
	}
	
	return 1;
}

int shutdown_converter() {

	dmICDestroy(cstate.ic);
       	return 1;
}
