%OpenCLInterface Class: Provides a wrapper for interfacing with OpenCL.
%Create an object of this class to be able to run any kernel on any device 
%that supports OpenCL and query all the devices available to the system.

% Copyright (c) 2014, MESC Labs
% All rights reserved.
% 
% Redistribution and use in source and binary forms, with or without 
% modification, are permitted provided that the following conditions are 
% met:
% 
%     * Redistributions of source code must retain the above copyright 
%       notice, this list of conditions and the following disclaimer.
%     * Redistributions in binary form must reproduce the above copyright 
%       notice, this list of conditions and the following disclaimer in 
%       the documentation and/or other materials provided with the distribution
%       
% THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
% AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
% IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
% ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
% LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
% CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
% SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
% INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
% CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
% ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
% POSSIBILITY OF SUCH DAMAGE.
classdef OpenCLInterface < handle
    % Variables to hold information and C++ pointers
    properties (SetAccess = private, Hidden = true)
        %Do not modify any of these variables, use the 'Devices' for query
        %purpose only, make a getter function if the ones available are not
        %enough, for more information about its structure, see the
        %PrintDevices function and stay away from 'Environment_ptr',
        %or you could suffer from memory leaks, you have been warned :D
        
        Devices; %Cell of cells, contains list of all OpenCL devices, use it for query.
        Environment_ptr; %Holds the value of a pointer to object of the Environment class created in C++, don't mess with this variable.
    end
    methods
        function this = OpenCLInterface(this)
            %Class constructor, returns a class object. It takes no inputs.
            
            %Call mex to get list of devices
            this.Devices = OpenCLMexWrapper('deviceQuery');
            %Initialize the Environment_ptr to 0
            this.Environment_ptr = 0;
        end
        
        function delete(this)
            %Class destructor.
            if this.Environment_ptr ~= 0
                %Call mex to delete the environment variable
                OpenCLMexWrapper('deleteEnvironment',this.Environment_ptr);
                this.Environment_ptr = 0;
            end
            for i = 1:size(this.Devices,1)
                %Call mex to delete all device pointers
                OpenCLMexWrapper('deleteDevice', this.Devices{i}{1});
            end
        end
        
        function PrintDevices(this)
            %Print all devices in the variable Devices in a readable manner
            for i = 1:size(this.Devices,1)
                message = sprintf('Device No: %d\n ',i);
                message = [message sprintf('Device ID: %s\n ',this.Devices{i}{2})];
                message = [message sprintf('Device Name: %s\n ',this.Devices{i}{3})];
                switch this.Devices{i}{4}
                    case 0
                        message = [message sprintf('Device Type: %s\n ', 'CPU')];
                    case 1
                        message = [message sprintf('Device Type: %s\n ', 'GPU')];
                    case 2
                        message = [message sprintf('Device Type: %s\n ', 'Accelerator')];
                    case 3
                        message = [message sprintf('Device Type: %s\n ', 'Default')];
                    case 4
                        message = [message sprintf('Device Type: %s\n ', 'Custom')];
                end
                message = [message sprintf('Driver Version: %s\n ',this.Devices{i}{5})];
                message = [message sprintf('Global Memory Size: %.0f MB\n ',this.Devices{i}{6})];
                message = [message sprintf('Max Compute Units: %.0f\n ',this.Devices{i}{7})];
                message = [message sprintf('Max Workitem Dimensions: %.0f\n ',this.Devices{i}{8})];
                
                message = [message sprintf('Max Workitem Sizes:')];
                for j = 9:size(this.Devices{i},1)-1
                    message = [message sprintf(' %.0f ',this.Devices{i}{j})];
                end
                if this.Devices{i}{end}
                    message = [message sprintf('\n Endianness: %s','Little')];
                else
                    message = [message sprintf('\n Endianness: %s','Big')];
                end
                sprintf('Device Info:\n %s', message)
            end
        end
        
        function Endianness = GetDeviceEndianness(this, devId)
            %Returns true if the devices is Little Endian, false if Big
            Endianness = this.Devices{devId}{end};
        end
        
        function devIds = GetCPUDevices(this)
            devIds = [];
            for i = 1:size(this.Devices,1)
                if ~this.Devices{i}{4}
                    devIds = [devIds i];
                end
            end
        end
        
        function devIds = GetGPUDevices(this)
            devIds = [];
            for i = 1:size(this.Devices,1)
                if this.Devices{i}{4} == 1
                    devIds = [devIds i];
                end
            end
        end
        
        function CreateFunction(this, DeviceID, kernelSource, kernelName, isStringNotFile)
            %Compiles the OpenCL kernel.
            %--------------------------------------------------------------
            %Input:
            % o DeviceID: One-based ID of the target OpenCL device which is
            % obtained from PrintDevices.
            % o kernelSource: Path to kernel file (default), or a string
            % containing the kernel body if isStringNotFile flag is on.
            % o kernelName: Name of the target OpenCL Kernel in the input
            % string/file.
            % o isStringNotFile (Optional): A boolean flag that indicates
            % that kernelSource is the body of the kernel not a file path.
            
            %Internal Doc: the function creates an instance of the
            % Environment class in C++ which holds the compiled OpenCL code
            % and assigns a pointer to the class object in Environment_ptr
            if nargin > 4 && isStringNotFile
                inputString = kernelSource;
            elseif exist(kernelSource,'file')
                inputString = fileread(kernelSource);
            else
                error('File not found');
            end
            if(this.Environment_ptr ~= 0) % Delete previous instances of Environment_ptr if any
                OpenCLMexWrapper('deleteEnvironment',this.Environment_ptr);
                this.Environment_ptr = 0;
            end
            this.Environment_ptr = OpenCLMexWrapper('buildKernel', this.Devices{DeviceID}{1}, inputString, kernelName);
        end
        
        function Run(this, threadDimensions, localWorkSize, varargin)
            %Runs the OpenCL kernel compiled using 'CreateFunction' and
            %returns the output in the buffers marked as 'Output' using the
            %memory flags described below.
            %--------------------------------------------------------------
            %Input:
            % o threadDimensions: A 1 by n vector containing number of
            % threads in each dimension. The number of threads possible per
            % dimension and the number of possible dimensions (1D-2D-3D-nD)
            % is dependent on the device's architecture. All dimensions are
            % supported
            % o localWorkSize: A 1 by n vector containing number of threads
            % in each work-group (aka thread block). Pass 0 if you would
            % like OpenCL to set an arbitary size. Note that this size is
            % significant only if you are using Local (aka Shared) memory.
            % o varargin (Optional Parameters): These are the parameters of
            % the kernel, they must be passed in the same order as that of
            % kernel signature. The following describes different memory
            % modes and options:
            %   + To pass a scalar value, just pass it and don't follow it
            %   with any memory options.
            %   + To pass a buffer, pass the matrix, then follow it with
            %   its memory options (a set of the following):
            %    *Memory Type (mutually exclusive flags):
            %      M = image(texture memory), only float images
            %         are supported
            %      nothing = buffer(global memory),
            %    *I/O access (mutually exclusive flags):
            %      I = Input, O = Output (data written to this parameter
            %      during kernel execution will be read from the device
            %      into this variable, hint: use this for result buffers).
            %    *Read and write access (can be mixed):
            %      r = read, w = write
            %    *Initialization Behaviour
            %      c = Copy Host Ptr (initialize the device memory by
            %      copying the data in the buffer),
            %      u = Use Host Ptr (read from Host RAM not Device Memory)
            % Example:
            %  Run([10 5], 0, scalar1, scalar2, buffer1, 'MIrwc', ...
            %                     scalar3, buffer2, 'Irc', buffer3, 'Owc')
            %  This call launches 10 by 5 threads, who's organization into
            %  thread blocks is left to OpenCL (0 LocalWorkSize). Passes 3
            %  scalars, and 3 buffers. The sacalars as shown do not require
            %  memory flags. The buffers memory options are as follows:
            %   +buffer1: Image, Input, Read, Write, Copy from host.
            %   +buffer2: Buffer, Input, Read only, Copy from host.
            %   +buffer3: Buffer, Output, Write only, Copy from host.
            if this.Environment_ptr == 0
                error('Please compile an OpenCL kernel using ''CreateFunction''');
            end
            
            scalarCount = 0;
            bufferFlags = []; %This 1 by n array will hold memory flags of all buffers
            nextStringFlag = false;
            if length(threadDimensions) == 0
                error('Thread dimensions CANNOT be empty');
            end
            if length(varargin) == 0
                error('No data parameters supplied');
            end
            if length(localWorkSize) == 0 || ( localWorkSize(1)~=0 && length(localWorkSize) ~= length(threadDimensions))
                error('localWorkSize must have same dimensions as threadDimensions or equal to 0')
            end
            for i=1:size(varargin,2)
                if  ~iscellstr(varargin(i)) % Not a string
                    if nextStringFlag % Buffer Memory options were expected
                        error('Buffer No. %d type expected',floor((i-scalarCount)/2));
                    end
                    
                    if length(varargin{i}) ~= 1 % It is a Buffer (nothing to be done if its a scalar)
                        nextStringFlag = true; % Next parameter should be a string
                        continue;
                    end
                elseif nextStringFlag % Buffer Memory options are expected
                    str = varargin{i};
                    flags = 0;
                    ioset = false;
                    readset = false;
                    hostptrset = false;
                    %M = image (texture memory), empty = buffer(global memory), only float images are
                    %supported
                    %I O input output
                    %r = read, w = write
                    %c = copy_host_ptr, u = use_host_ptr (read from Host
                    %RAM not Device Memory)
                    for j=1:length(str)
                        switch str(j)
                            case 'I'
                                if ~ioset
                                    flags = bitor(flags , 1);
                                    ioset = true;
                                else
                                    error('CANNOT set I/O flag twice in parameter %d', i);
                                end
                            case 'O'
                                if ~ioset
                                    flags = bitor(flags , 2);
                                    ioset = true;
                                else
                                    error('CANNOT set I/O flag twice in parameter %d', i);
                                end
                            case 'r'
                                flags = bitor(flags , 4);
                                readset = true;
                            case 'w'
                                flags = bitor(flags , 8);
                                readset = true;
                            case 'c'
                                if ~hostptrset
                                    flags = bitor(flags , 16);
                                    hostptrset = true;
                                else
                                    error('CANNOT set host_ptr flag twice in parameter %d', i);
                                end
                            case 'u'
                                if ~hostptrset
                                    flags = bitor(flags , 32);
                                    hostptrset = true;
                                else
                                    error('CANNOT set host_ptr flag twice in parameter %d', i);
                                end
                                
                            case 'M'
                                flags = bitor(flags, 128); % 128 because 'a' is 64 and is resreved even though its not supported
                                
                                %not supported! but flag value is reserved
                                %                             case 'a'
                                %                                 if ~hostptrset
                                %                                     flags = bitor(flags , 64);
                                %                                     hostptrset = true;
                                %                                 else
                                %                                     error('CANNOT set host_ptr flag twice in parameter %d', i);
                                %                                 end
                            otherwise
                                error('Unkown buffer memory option');
                        end
                    end
                    if ~ioset || ~readset || ~hostptrset
                        error('Missing buffer memory options in parameter %d',i);
                    end
                    bufferFlags = [bufferFlags flags]; %add the flags to the bufferFlags array
                    nextStringFlag = false;
                else
                    error('Unexpected String Input in parameter %d',i);
                end
            end
            if nextStringFlag
                error('Last buffer memory options expected');
            end
            OpenCLMexWrapper('Run', this.Environment_ptr, bufferFlags, threadDimensions, localWorkSize, varargin);
        end
    end
end