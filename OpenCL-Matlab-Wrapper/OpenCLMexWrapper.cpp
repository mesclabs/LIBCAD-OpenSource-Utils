/*
Copyright (c) 2014, MESC Labs
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are 
met:

    * Redistributions of source code must retain the above copyright 
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in 
      the documentation and/or other materials provided with the distribution
      
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
*/

#include "mex.h"
#include "OpenCLWrapper.h"
#include "class_handle.hpp"
#include <iostream>
#include <functional>
#include <sstream>
using namespace std;

/**********************************************HELPER FUNCTIONS*****************************************************/
// Simple function to convert any prhs element such as prhs[2] to string.
string getString(const mxArray *prhs){
	char *buf;
	size_t buflen;
	buflen = mxGetN(prhs)*sizeof(mxChar)+1;
	buf = (char*)mxMalloc(buflen);
	/* Copy the string data into buf. */
	mxGetString(prhs, buf, (mwSize)buflen);
	string s = ""; s += buf;
	mxFree(buf);
	return s;
}

// damn Nvidia SDK works only with VC on windows, wish i could use c++11's string literals and constexpr :'(
enum string_code{
	deviceQueryHash,
	buildKernelHash,
	RunHash,
	deleteDeviceHash,
	deleteEnvironmentHash
};

struct BufferInfo
{
    int paramNumber;
	int i;
	int elementCount;
	mxClassID classID;
};

string_code hash_(string s)
{
    if(s == "deviceQuery") return deviceQueryHash;
    if(s == "buildKernel") return buildKernelHash;
    if(s == "Run") return RunHash;
    if(s == "deleteDevice") return deleteDeviceHash;
    if(s == "deleteEnvironment") return deleteEnvironmentHash;
}
/******************************************END HELPER FUNCTIONS*****************************************************/

bool verbose = false;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	if(!mxIsChar(prhs[0]))
		mexErrMsgIdAndTxt("Matlab:OpenCLMexWrapper","String Command Expected");
	
	
	string command = getString(prhs[0]);

	switch(hash_(command))
	{
		case deviceQueryHash:
		{	
			//Get all devices
			vector<Device *> devs = OpenCLQuery(CL_DEVICE_TYPE_ALL);

			mwSize ndim = 1;
			mwSize dims[] = {devs.size()};

			mxArray *devsCellArrayPtr = mxCreateCellArray(ndim, dims); //Create Devices cell array. Devices is a cell array of cell arrays (i.e. {{{},{},{}},{{},{},{}}}), each cell array contains info of 1 device

			stringstream ss;
			for(int i=0;i<devs.size();i++){
				// The pointer will be set as double using the class_handle

				mwSize dim[] = {9 + devs[i]->maxWorkItemsDimensions}; //Device info is 10 entries + n entries (where n is the number of the thread dimensions possible on the card)
				mxArray *deviceCellArrayPtr = mxCreateCellArray(ndim, dim);

				int j = 0;
				mxSetCell(deviceCellArrayPtr, j++, convertPtr2Mat<Device>(devs[i]));
				ss.str(std::string()); // Clear stringstream
				ss << devs[i]->deviceId; 
				mxSetCell(deviceCellArrayPtr, j++,  mxCreateString(ss.str().c_str()));
				ss.str(std::string());
				ss << devs[i]->deviceName;
				mxSetCell(deviceCellArrayPtr, j++,  mxCreateString(ss.str().c_str()));
				
				// Device Type
				if (devs[i]->deviceType & CL_DEVICE_TYPE_CPU)
					mxSetCell(deviceCellArrayPtr, j++,  mxCreateDoubleScalar(0));
				else if (devs[i]->deviceType & CL_DEVICE_TYPE_GPU)
					mxSetCell(deviceCellArrayPtr, j++,  mxCreateDoubleScalar(1));
				else if (devs[i]->deviceType & CL_DEVICE_TYPE_ACCELERATOR)
					mxSetCell(deviceCellArrayPtr, j++,  mxCreateDoubleScalar(2));
				else if (devs[i]->deviceType & CL_DEVICE_TYPE_DEFAULT)
					mxSetCell(deviceCellArrayPtr, j++,  mxCreateDoubleScalar(3));
				#ifdef CL_DEVICE_TYPE_CUSTOM
				else if (devs[i]->deviceType & CL_DEVICE_TYPE_CUSTOM)
					mxSetCell(deviceCellArrayPtr, j++,  mxCreateDoubleScalar(4));
				#endif

				ss.str(std::string());
				ss << devs[i]->driverVersion;
				mxSetCell(deviceCellArrayPtr, j++,  mxCreateString(ss.str().c_str()));
				ss.str(std::string());
				mxSetCell(deviceCellArrayPtr, j++,  mxCreateDoubleScalar(devs[i]->globalMemorySize/1024/1024));
				mxSetCell(deviceCellArrayPtr, j++,  mxCreateDoubleScalar(devs[i]->maxComputeUnits));
				mxSetCell(deviceCellArrayPtr, j++,  mxCreateDoubleScalar(devs[i]->maxWorkItemsDimensions));

				
				for(auto a = devs[i]->maxWorkItemsSizes.get(); a < devs[i]->maxWorkItemsSizes.get()+devs[i]->maxWorkItemsDimensions;a++){
					mxSetCell(deviceCellArrayPtr, j++, mxCreateDoubleScalar(*a));
				}

				//Endiannness
				mxSetCell(deviceCellArrayPtr, j++,  mxCreateLogicalScalar(devs[i]->endianness)); // boolean value (True is Little Endian)

				mxSetCell(devsCellArrayPtr, i, deviceCellArrayPtr);
			}

			plhs[0] = devsCellArrayPtr;

			break;
		}

		case buildKernelHash:
		{
			Device * d = convertMat2Ptr<Device>(prhs[1]);
			Environment *e = new Environment(*d); // Create environment variable 

			if(!mxIsChar(prhs[2]))
				mexErrMsgIdAndTxt("Matlab:OpenCLMexWrapper","String kernel expected");

			if(!mxIsChar(prhs[3]))
				mexErrMsgIdAndTxt("Matlab:OpenCLMexWrapper","String kernelName expected");
			
			// Set kernel
			if(!e->setKernel(getString(prhs[2]), getString(prhs[3])))
				mexErrMsgIdAndTxt("Matlab:OpenCLWrapper","Can't set kernel");

			plhs[0] = convertPtr2Mat(e);
			break;
		}

		case RunHash:
		{
			Environment *e = convertMat2Ptr<Environment>(prhs[1]); //Environment Pointer
			if(!mxIsNumeric(prhs[2])) //Buffer memory flags (created in the wrapper as a 1 by n array of integers)
				mexErrMsgIdAndTxt("Matlab:OpenCLMexWrapper","Buffer flags array expected");
			if(!mxIsNumeric(prhs[3]))
				mexErrMsgIdAndTxt("Matlab:OpenCLMexWrapper","Thread dimensions expected");
			if(!mxIsNumeric(prhs[4]))
				mexErrMsgIdAndTxt("Matlab:OpenCLMexWrapper","Local worksize expected");
			if(!mxIsCell(prhs[5]))
				mexErrMsgIdAndTxt("Matlab:OpenCLMexWrapper","Data cell array expected");

			size_t dimensionsCount = mxGetN(prhs[3]);
			double *tmpDoublePtr = mxGetPr(prhs[3]); // thread dimensions input
			size_t *threadDimensions = new size_t[dimensionsCount];
			for(int i=0;i<dimensionsCount;i++){
				threadDimensions[i] = *(tmpDoublePtr+i);
				if(verbose)
					mexPrintf("Thread dimensions: %d\n",threadDimensions[i]);
			}

			tmpDoublePtr = mxGetPr(prhs[4]); //local work size input

			size_t *localWorkSize = nullptr;
			if(*tmpDoublePtr){ //IF 0 was not passed in from matlab. Otherwise localWorkSize would remain null and get passed as null to Run
				if(dimensionsCount != mxGetN(prhs[4]))
					mexErrMsgIdAndTxt("Matlab:OpenCLMexWrapper","Local worksize dimensionality doesn't match global worksize");

				localWorkSize = new size_t[dimensionsCount];
				for(int i=0;i<dimensionsCount;i++){
					localWorkSize[i] = *(tmpDoublePtr+i);
					if(verbose)
						mexPrintf("Local worksize: %d\n", localWorkSize[i]);
				}
			}

			vector<kernelArgument> kernArgs;
			int bufferCount = 0;
			tmpDoublePtr = mxGetPr(prhs[2]); // Buffer flags
			vector<BufferInfo> outputBufferIdx; // used to keep track of which matrices are going to be output and therefore read from memory after execution
			for(int i=0;i<mxGetN(prhs[5]);i++){ // Loop over all buffers/scalars
				mxArray *c = mxGetCell(prhs[5], i);

				// Scalar
				if(mxIsNumeric(c) && mxGetN(c) == 1 && mxGetM(c) == 1){
					kernArgs.push_back(kernelArgument(*e,mxGetData(c),mxGetElementSize(c)));
					if(verbose)
						mexPrintf("kernel arg data problem, data dump: %d\n",*((int*)kernArgs[i-bufferCount-1].data));
				}
				else{
					// WARNING! Matlab calling interface must do sanity check on memory flags!

					double flags = *tmpDoublePtr;
					int flag = static_cast<int>(flags);
					cl_mem_flags memFlags = 0;
					if(flag & 4) // Read
						if(flag & 8) // Write
							memFlags |= CL_MEM_READ_WRITE;
						else // Read only
							memFlags |= CL_MEM_READ_ONLY;
					else // Write only
						memFlags |= CL_MEM_WRITE_ONLY;

					if(flag & 16) // Copy_host_ptr
						memFlags |= CL_MEM_COPY_HOST_PTR;
					else if(flag & 32) // Use_host_ptr
						memFlags |= CL_MEM_USE_HOST_PTR;
					else if(flag & 64) // Allocate_host_ptr
						memFlags |= CL_MEM_ALLOC_HOST_PTR;

					if(flag & 2){ // Output
						BufferInfo b;
                        b.paramNumber = i; // param No. in the cell array
						b.i = i-bufferCount; // b.i -> index in kernArgs vector. Every buffer skips 1 text arguments
						b.elementCount = mxGetN(c)*mxGetM(c);
						b.classID = mxGetClassID(c);
						outputBufferIdx.push_back(b);
					}

					if(verbose)
					{
						mexPrintf("pointer: %p size: %d\n",mxGetData(c),mxGetN(c)*mxGetM(c)*mxGetElementSize(c));
						mexPrintf("%f %d %d %p\n",flags, flag,mxGetElementSize(c),mxGetData(c));
					}
					
					if(flag & 128) //Image memory
					{
						cl_image_format format;
						if(mxGetNumberOfDimensions(c) > 2) // No. of channels
					 		format.image_channel_order = CL_RGBA;
					 	else // grayscale
					 		format.image_channel_order = CL_R;
					 	
					 	format.image_channel_data_type = CL_FLOAT; // Only floats are supported

						kernArgs.push_back(kernelArgument(*e,mxGetData(c),mxGetN(c)*mxGetM(c)*mxGetElementSize(c),memFlags,mxGetN(c),mxGetM(c),format));
					}
					else // Normal buffer
						kernArgs.push_back(kernelArgument(*e,mxGetData(c),mxGetN(c)*mxGetM(c)*mxGetElementSize(c),memFlags));

					tmpDoublePtr++;
					bufferCount++;
					++i; // skip the text arguments, its the flags string
				}
			}
			
			// Launch OpenCL kernel
			Run(static_cast<cl_uint>(dimensionsCount), threadDimensions, localWorkSize, kernArgs);

			// Read outputs
			for(int i=0; i<outputBufferIdx.size();i++){
				void *ptr = mxGetData(mxGetCell(prhs[5], outputBufferIdx[i].paramNumber));
				kernArgs[outputBufferIdx[i].i].readBufferFromDevice(ptr, false);
			}

			// Clear device memory
			for(int i=0;i<kernArgs.size();i++){
				kernArgs[i].releaseAll();
			}
			break;
		}

		case deleteDeviceHash:
		{
			Device *p = convertMat2Ptr<Device>(prhs[1]);
			p->releaseAll();

			if(verbose)
				mexPrintf("Device:deleted c pointer\n");

			destroyObject<Device>(prhs[1]);
			
			if(verbose)
				mexPrintf("Device:matlab class handle deleted\n");
			
			break;
		}

		case deleteEnvironmentHash:
		{
			
			Environment *p = convertMat2Ptr<Environment>(prhs[1]);
			p->releaseAll();

			if(verbose)
				mexPrintf("Environment:deleted c pointer %u\n",p->program);

			destroyObject<Environment>(prhs[1]);

			if(verbose)
				mexPrintf("Environment:matlab class handle deleted\n");

			break;
		}

		default:
			mexErrMsgIdAndTxt("Matlab:OpenCLMexWrapper", "Command not found");
			break;
	}

}