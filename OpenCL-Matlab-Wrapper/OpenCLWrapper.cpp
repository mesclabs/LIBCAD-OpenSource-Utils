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

#include "OpenCLWrapper.h"
#include <array>
#include <cstdio>
#include <fstream>
#include <stdarg.h>
#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif
using namespace std;

void printErr(const char *fmt, ...){
#ifndef MATLAB_MEX_FILE
	va_list args;
	va_start (args, fmt);
	vprintf (fmt, args);
	va_end (args);
#else
	va_list args;
	va_start (args, fmt);
	char x[10000];
	vsprintf (x, fmt, args);
	mexErrMsgIdAndTxt("Matlab:OpenCLWrapper", x);
	va_end (args);
#endif
}

void printWarn(const char *fmt, ...){
#ifndef MATLAB_MEX_FILE
	va_list args;
	va_start (args, fmt);
	vprintf (fmt, args);
	va_end (args);
#else
	va_list args;
	va_start (args, fmt);
	char x[10000];
	vsprintf (x, fmt, args);
	mexPrintf("Error in Matlab:OpenCLWrapper : %s", x);
	va_end (args);
#endif
}

Device::Device(cl_device_id devId, cl_platform_id platId):deviceId(devId),platformId(platId){
	cl_device_type type;
	cl_uint tmpClUInt;
	cl_ulong tmpClUlong;
	cl_bool tmpClBool;
	std::array<char,2000> tmpCharArray;
	clGetDeviceInfo(devId, CL_DEVICE_TYPE, sizeof(type),&type,nullptr);
	deviceType = type;

	clGetDeviceInfo(devId, CL_DEVICE_VENDOR, tmpCharArray.size(),tmpCharArray.data(),nullptr);
	vendor = tmpCharArray.data();

	clGetDeviceInfo(devId, CL_DEVICE_NAME, tmpCharArray.size(),tmpCharArray.data(),nullptr);
	deviceName = tmpCharArray.data();

	clGetDeviceInfo(devId, CL_DRIVER_VERSION, tmpCharArray.size(),tmpCharArray.data(),nullptr);
	driverVersion = tmpCharArray.data();

	clGetDeviceInfo(devId, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(tmpClUInt),&tmpClUInt,nullptr);
	maxComputeUnits = tmpClUInt;

	clGetDeviceInfo(devId, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(tmpClUInt),&tmpClUInt,nullptr);
	maxWorkItemsDimensions = tmpClUInt;

	maxWorkItemsSizes.reset(new size_t[maxWorkItemsDimensions],default_delete<size_t[]>());

	clGetDeviceInfo(devId, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t)*maxWorkItemsDimensions,maxWorkItemsSizes.get(),nullptr);
	
	clGetDeviceInfo(devId, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(tmpClUlong),&tmpClUlong,nullptr);
	globalMemorySize = tmpClUlong;

	clGetDeviceInfo(devId, CL_DEVICE_ENDIAN_LITTLE, sizeof(tmpClBool),&tmpClBool,nullptr);
	endianness = tmpClBool;
}

void Device::releaseAll(){
	// clReleaseDevice(deviceId); //not needed in OpenCL 1.1
}

bool debug=true;

vector<Device *> OpenCLQuery(cl_device_type deviceType){
	cl_uint numDevices;
	cl_int err;
	cl_uint numPlatt;
	cl_platform_id *platform;
	cl_device_id *dev;

	err = clGetPlatformIDs(NULL,NULL,&numPlatt);
	if(debug && CL_SUCCESS != err) printErr("%d platform detection failed", err);
	
	platform = (cl_platform_id*)malloc(numPlatt*sizeof(cl_platform_id));
	
	err = clGetPlatformIDs(numPlatt,platform,NULL);
	if(debug && CL_SUCCESS != err) printErr("%d platform detection failed", err);
	
	vector<Device *> v;

	for(int i=0;i<numPlatt;i++){

		err = clGetDeviceIDs(platform[i],deviceType,0,nullptr,&numDevices);
		if(debug && CL_SUCCESS != err) {printWarn("%d No devices of the chosen type detected on platform no. %d", err,i); continue;}

		dev = new cl_device_id[numDevices];

		err = clGetDeviceIDs(platform[i],deviceType,numDevices,dev,nullptr);
		if(debug && CL_SUCCESS != err) printWarn("%d No devices of the chosen type detected on this platform no. %d", err,i);

		if(CL_SUCCESS==err)
			for(cl_uint j=0;j<numDevices;j++)
				v.push_back(new Device(dev[j], platform[i]));
	}
	if(!v.size())
	{
		printErr("No devices detected on all platforms");
	}
	free(platform);
	return v;
}

Environment::Environment(Device dev):device(dev){
	cl_int err;

	context = clCreateContext( nullptr ,1,&dev.deviceId,nullptr,nullptr,&err);
	if(debug && err){ printErr("%d context creation failed",err); }

	queue = clCreateCommandQueue(context, device.deviceId, 0, &err);
	if(debug && err){ printErr("%d queue creation failed",err); }
}

bool Environment::setKernel(std::string kernelSrc, std::string kernelName){
	cl_int err;
	const char *src = kernelSrc.c_str();
	
	program = clCreateProgramWithSource(context, 1, const_cast<const char**> (&src), nullptr, &err);
	if(debug && err)
	{ 
		printErr("%d program creation failed",err);
		return false;
	}

	err = clBuildProgram(program, 1, &(device.deviceId), nullptr, nullptr, nullptr);
	if(debug && err)
	{
		size_t s = 2000;
		char *tmp = new char[s];
		clGetProgramBuildInfo(program, device.deviceId, CL_PROGRAM_BUILD_LOG, s, tmp, &s);
		printErr("program build failed\n Build Log:\n %s", tmp);
		delete[] tmp;
		return false;
	}

	kernel = clCreateKernel(program, kernelName.c_str(), &err);
	if(debug && err) { printErr("%d kernel creation failed",err); return false; }

	return true;
}

void Environment::releaseAll(){
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context)
;}

kernelArgument::kernelArgument(Environment e, void *d, size_t s, cl_mem_flags memflags):size(s),memoryFlags(memflags),env(e),scalar(false),image(false),height(0),width(0){
	data = d;
	cl_int err;
	memory = clCreateBuffer(e.context, memflags, s, d, &err);
	if(debug && err){	printErr("%d kernelArgument creation failed", err);	}
}


kernelArgument::kernelArgument(Environment e, void *d, size_t s, cl_mem_flags memflags, int h, int w, cl_image_format fmt):size(s),memoryFlags(memflags),env(e),scalar(false),image(true),height(h),width(w),format(fmt)
{
	data = d;
	cl_int err;
	memory = clCreateImage2D (e.context, memflags, &fmt, w, h, 0, d, &err);
	if(debug && err){	printErr("%d kernelArgument creation failed", err);	}
}

void kernelArgument::readBufferFromDevice(void *&d, bool allocate = true){
	if(allocate)
		d = malloc(size);
	cl_int err = 0;
	if(!image)
		 err = clEnqueueReadBuffer(env.queue, memory, CL_TRUE, 0, size, d, 0, nullptr, nullptr);
	else
	{
		size_t origin[3] = {0,0,0};
		size_t region[3] = {width,height,1};
		err = clEnqueueReadImage(env.queue, memory, CL_TRUE, origin, region, 0, 0, d, 0, nullptr, nullptr);
	}
	
	if(debug && err){ printErr("%d kernelArgument read failed", err);	}
}

void kernelArgument::releaseAll(){
	if(!scalar)
		clReleaseMemObject(memory);
}

void Run(cl_uint threadDimensionsCount, size_t *globalWorkSize, size_t *localWorkSize, std::vector<kernelArgument> inputs){
	cl_uint i=0;
	cl_int err=0;
	for(vector<kernelArgument>::iterator b=inputs.begin(); b<inputs.end(); b++){
		if(b->scalar)
			err = clSetKernelArg(b->env.kernel, i++, b->size, b->data);
		else
			err = clSetKernelArg(b->env.kernel, i++, sizeof(b->memory), &b->memory);
		
		if(debug && err) {
			printErr((b->scalar?"%d Couldn't set scalar as kernel argument":"%d Couldn't set buffer as kernel argument"), err);
			 return;
		}
	}
	
	err = clEnqueueNDRangeKernel(inputs[0].env.queue, inputs[0].env.kernel, threadDimensionsCount,
		 nullptr, globalWorkSize, localWorkSize, 0, nullptr,	nullptr);

	clFinish(inputs[0].env.queue);
	if(debug && err){ printErr("%d Failed to Enqueue", err); return; }
}

 /*Simple test main
int main()
{
	vector<Device *> devs = OpenCLQuery(CL_DEVICE_TYPE_ALL);

	Environment e(*devs[0]);
	// Read from file
	// std::ifstream ifs("TestKernel.cl");
 	//  std::string kernel( (std::istreambuf_iterator<char>(ifs) ),
 	//                      (std::istreambuf_iterator<char>()    ) );

	// As a static string
	std::string kernel = "__kernel void TestKernel(__global float* buffer, float scalar){ buffer[get_global_id(0)] = scalar;}"; // simple copy kernel

	cout<<e.setKernel(kernel, "TestKernel")<<endl;
	float *data=(float*)malloc(10*sizeof(float));
	for(int q = 0; q<100;q++)
		data[q] = 5;

	kernelArgument buff(e,data,10*sizeof(float), CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
	float r=3;
	kernelArgument scalar(e,&r,sizeof(float));
	cout<<buff.size<<endl<<scalar.size<<endl;
	cl_uint threadDimensionsCount = 2;
	size_t threads[2];
	threads[0] = 10;
	threads[1] = 10;
	size_t localWork = 5;
	vector<kernelArgument > v;
	// They must be added to the vector in the same order as the kernel's parameters
	v.push_back(buff);
	v.push_back(scalar);

	Run(threadDimensionsCount, threads, &localWork, v); 

	void *tmp = nullptr;
	buff.readBufferFromDevice(tmp);
	float *tmpFloat = (float*)tmp;

	for(int w = 0; w<100;w++)
		cout<<tmpFloat[w]<<endl;

	buff.releaseAll();
	free(data);
	e.releaseAll();

	for(unsigned int i=0;i<devs.size();i++)
		devs[i]->releaseAll();

	return 0;
}*/