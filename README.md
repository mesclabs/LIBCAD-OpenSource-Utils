# Introduction #

This repo contains opensource files used in the development of [LIBCAD](http://libcad.mesclabs.com/) (the LIBrary for Computer
Aided Detection). These files are either for methods and algorithms or for system architecture and
deployment.

# Contents #

## PHP-Layer ##

(This PHP layer exists in the folder `PHP-Layer`). A novel form of LIBCAD is the online
(cloud-based) API. The interface to this API is established through a PHP layer. This means that
this API is accessible by anyone and anywhere with just an internet connection. The API itself is an
interface to LIBCAD (the DLL or the SO version) depending on the cloud server whether it is Windows
or Linux respectively. So, the CAD algorithms are compiled and deployed to the cloud server in
either library forms (DLL or SO), then the API layer accounts as a cloud interface to either one of
these forms of library.

## OpenCL-Matlab-Wrapper ##

(This wrapper exists in the folder `OpenCL-Matlab-Wrapper`). At the early time of this work (circa
2012), MATLAB did not have the currently available OpenCL toolbox; so, in order to facilitate our
work with the GPU, we wrote MATLAB [opensource
wrapper](https://se.mathworks.com/matlabcentral/fileexchange/46826-opencl-matlab-wrapper.) This
wrapper automated the process of compling OpenCL kernels, converting MATLAB matrices to GPU buffers,
converting data formats and reshaping the data. The result of this is that it has become much easier
to run, experiment and deploy our custom kernels with a few lines of MATLAB code, and zero lines of
C++ code. This wrapper provides an interface between MATLAB and OpenCL in a fashion similar to that
of Mathematica's OpenCLLink. It does control everything in the environment: copy data back and
forth, launch threads in an intuitive fashion; all organized in a class and done by simple function
interface. It takes only 2 calls to compile a kernel, copy buffers, launch threads and read the data
back. The class includes:

``` matlab
obj = OpenCLInterface %Constructor which queries all available devices.
obj.PrintDevices      %Print all available devices.
obj.GetGPUDevices     %Get ids of all GPU devices.
obj.GetCPUDevices     %Get ids of all CPU devices.
obj.CreateFunction    %Read kernel code from file or string, compile it and cache it.
obj.Run               %Launch the kernel with the specified local and global workloads, scalars and buffers with their memory flags. Buffers specified as Outputs will contain the result data after execution. 

%Sample Program layout:
obj = OpenCLInterface;
obj.CreateFunction(deviceId,code,'KernelName');
obj.Run(globalWorkload,LocalWorkload,scalar,buffer1,MemoryFlagsOfBuffer1,buffer2,MemoryFlagsOfBuffer2);
```

# Citation #

Please cite this work as:

``` tex
@Manual{LIBCADUtil2012,
author = {MESC Labs for Research and Development}
title = {LIBCAD-OpenSource: Software Utilities for Computer Aided Detection (CAD)},
year = {2012},
url = {https://github.com/mesclabs/LIBCAD-OpenSource}
}
```
