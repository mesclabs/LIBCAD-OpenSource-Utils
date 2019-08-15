%% Windows Compile CUDA
mex OpenCLMexWrapper.cpp OpenCLWrapper.cpp -I'C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v5.0\include' -L'C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v5.0\lib\x64' -lOpenCL -g

%% Windows Compile Intel
mex OpenCLMexWrapper.cpp OpenCLWrapper.cpp -DCL_USE_DEPRECATED_OPENCL_1_1_APIS -I'C:\Program Files (x86)\Intel\OpenCL SDK\3.0\include' -L'C:\Program Files (x86)\Intel\OpenCL SDK\3.0\lib\x64' -lOpenCL

%% Windows Compile ATI
mex OpenCLMexWrapper.cpp OpenCLWrapper.cpp -I'C:\Program Files (x86)\ATI Stream\include' -L'C:\Program Files (x86)\ATI Stream\lib\x86_64' -lOpenCL

%% Windows g++(version)-multilib
%U can use the Makefile to compile on windows if gcc plays nice with your
%OpenCL sdk. nVidia's sdk supports VC only on windows for some reason.
%Of course modify the Makefile and set appropriate paths/compilers
%Note that you do not need to use -std=c++11, you can use -std=c++0x if
%your compiler doesn't support c++11
system('make');


%% LINUX PREREQUISITS g++(version)-multilib
%if mex, your gcc and OpenCL driver versions are compatible, u can use
%the same commandlines as windows, otherwise, use the Makefile.
%Of course modify the Makefile and set appropriate paths/compilers.
%Note that you do not need to use -std=c++11, you can use -std=c++0x if
%your compiler doesn't support c++11

%If you get an error 'GLIBCXX_3.*' when running the mex compiled
%using the makefile, symlink your libstdc++ present in 
%[matlabroot '/sys/os/PLATFORM/libstdc++'] to your gcc's libstdc++ usually
%present in /usr/lib/gcc/PLATFORM/VERSION/
%also make sure u install the appropriate packages since libstdc++ library
%on Ubuntu just run 'apt-get install libstdc++6'.
system('make');
