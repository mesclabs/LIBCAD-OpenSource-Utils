MATLABROOT=/home/user/MATLAB/R2011a
CC=g++-4.8
CFLAG= -std=c++11 -Wall -m64 -I$(MATLABROOT)/extern/include $(SRC) $(LIBS) -o $(EXE) 
MEXFLAG= -std=c++11 -m64 -shared -fPIC -DMATLAB_MEX_FILE -I$(MATLABROOT)/extern/include $(LIBS) -D__STDC_UTF_16__

LIBS= -L$(MATLABROOT)/bin/glnxa64 -L$(MATLABROOT)/extern/lib/glnxa64 -lOpenCL -lmex -lmx -lmwlapack -lmwblas -leng
MEXTGT=OpenCLMex
SRC=OpenCLMexWrapper.cpp OpenCLWrapper.cpp
MEXSRC=OpenCLMexWrapper.cpp OpenCLWrapper.cpp
all: $(MEXTGT)

$(MEXTGT): $(MEXSRC)
	$(CC) -c $(MEXFLAG) OpenCLMexWrapper.cpp -o OpenCLMexWrapper.o
	$(CC) -c $(MEXFLAG) OpenCLWrapper.cpp -o OpenCLWrapper.o 
	$(CC) OpenCLMexWrapper.o OpenCLWrapper.o $(MEXFLAG) -o OpenCLMexWrapper.mexa64
	
