%Make sure you compile the mex first. See Compile.m

a=OpenCLInterface;%initialize interface
a.PrintDevices %print devices on the system
devId = a.GetGPUDevices;
if isempty(devId)
    devId = a.GetCPUDevices;
    if isempty(devId)
        error('no devices found');
    end
end


%read kernel from from file
a.CreateFunction(devId,'TestKernel.cl','TestKernel',false); %false indicates being read from file

%read inline kernel
code = '__kernel void TestKernel(__global float* buffer, float scalar, int width){ buffer[(get_global_id(0)*width)+get_global_id(1)] = scalar;}';
a.CreateFunction(devId,code,'TestKernel',true); %true indicates being directly read from string

%Now the kernel is compiled and saved in 'a', any thread enqueueing will not 
%require recompilation of the kernel.

%parameters. CASTING IS VERY IMPORTANT
buff = single(ones(10,10)); %float
width = int32(10);%int
scalar = single(5); %float
%workload
globalWorkload = [10 10]; % 2D workload, 10 threads in each dimension
localWorkload = 0; %none, equivalent to passing NULL to OpenCL's enqueue
%execution
%buff's memory flags ('Orwc') in the call to Run are read/write with copy to
%host pointer and the 'O' means its an output variable.
%an output variable is one that changes during the execution and you want
%to obtain its values after execution like result buffers.
%for more details, see the documentation of the Run function in OpenCLInteface
%scalars don't need memory flags
buff %print before run
a.Run(globalWorkload,localWorkload,buff,'Orwc',scalar,width);
buff %print after run
