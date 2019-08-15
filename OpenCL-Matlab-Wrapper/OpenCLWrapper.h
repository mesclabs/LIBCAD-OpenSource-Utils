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

#ifndef OpenCLWrapper_H
#define OpenCLWrapper_H
#include <iostream>
#include <string>
#include <CL/cl.h>
#include <memory>
#include <vector>

// Contains all Device Information
struct Device{

	explicit Device(cl_device_id devId, cl_platform_id);
	explicit Device(){}
	cl_device_id deviceId;
	cl_platform_id platformId;
	cl_device_type deviceType; 
	std::string vendor;
	std::string deviceName; 
	std::string driverVersion;
	cl_uint maxComputeUnits;
	cl_uint maxWorkItemsDimensions;
	std::shared_ptr<size_t> maxWorkItemsSizes;
	cl_ulong globalMemorySize;
	bool endianness;

	void releaseAll();
};

// Contains the whole environment information such as  context, queue, program, device and kernel. setKernel compiles the kernel and saves its pointer to save time for recalls.
struct Environment{
	cl_kernel kernel;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	Device device;
	explicit Environment(Device dev);
	explicit Environment(){}

	bool setKernel(std::string kernel, std::string kernelName);
	void releaseAll();
};

// A kernel argument, contains information and constructors that helps with managing the arguments.
struct kernelArgument{
	cl_mem memory;
	size_t size;
	cl_mem_flags memoryFlags;
	Environment env;
	void *data;
	bool scalar;
	bool image;
	int height,width;
	cl_image_format format;
	explicit kernelArgument():size(0),memoryFlags(0),data(nullptr),scalar(false),image(false){}
	//Environmet of the argument, void pointer to the data, size of the data in BYTES, memory flags.
	explicit kernelArgument(Environment e, void *d, size_t s, cl_mem_flags memFlags);
	//Environmet of the argument, void pointer to the data, size of the data in BYTES, memory flags, height and width of the image, and image format
	explicit kernelArgument(Environment e, void *d, size_t s, cl_mem_flags memFlags, int h, int w, cl_image_format fmt);
	//Environmet of the argument, void pointer to the data, size of the data in BYTES.
	explicit kernelArgument(Environment e, void *d, size_t s):size(s),env(e),data(d),scalar(true){};

	void readBufferFromDevice(void *&d, bool allocate);

	void releaseAll();
};

// Copies the buffers, launches the threads and awaits finishing.
// threadDimensionsCount must be specified
// globalWorkSize must be specified
// Pass nullptr to localWorkSize if you don't want to specify local worksize
// kernel arguments vector must be arranged in the same order as the parameters of the kernel, i.e. inputs[0] will be assigned to kernel argument number 0
void Run(cl_uint threadDimensionsCount, size_t *globalWorkSize, size_t *localWorkSize, std::vector<kernelArgument> inputs);

// Gets all available devices of the specified device type, u can pass CL_DEVICE_TYPE_ALL to get all devices
std::vector<Device *> OpenCLQuery(cl_device_type deviceType);
#endif