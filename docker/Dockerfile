FROM pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime
#FROM aimagelab/opencv4.4-cuda11.4.1-ubuntu20.04

RUN apt -qq update                  && \
    DEBIAN_FRONTEND=noninteractive apt -qq install build-essential    \
    python3-pip cmake gcc-8 g++-8 git -y

# CUDA Toolkit
RUN apt install -y nvidia-cuda-toolkit

# OpenCV
RUN git clone https://github.com/opencv/opencv.git /opencv
RUN git clone https://github.com/opencv/opencv_contrib.git /opencv_contrib
RUN mkdir -p /opencv/build && cd /opencv/build

RUN cmake /opencv \
  -D OPENCV_EXTRA_MODULES_PATH=/opencv_contrib/modules \
  -D BUILD_LIST="core,imgcodecs,imgproc,core,cudafeatures2d,cudaarithm,cudafilters,cudaimgproc,cudawarping,cudev,features2d,imgcodecs,imgproc" \
  -D WITH_CUDA=ON \
  -D WITH_CUDNN=ON \
  -D WITH_CUBLAS=ON \
  -D CUDA_ARCH_BIN=7.5 \
  -D CMAKE_BUILD_TYPE=RELEASE \
  -D CMAKE_INSTALL_PREFIX=/usr/local \
#  -D HAVE_opencv_python3=ON \
#  -D INSTALL_PYTHON_EXAMPLES=ON \
  -D CMAKE_C_COMPILER=/usr/bin/gcc-8 \
  -D CUDA_BIN_PATH=/usr/local/cuda
RUN make -j16
RUN make install
RUN ldconfig
#cmake /opencv -D OPENCV_EXTRA_MODULES_PATH=/opencv_contrib/modules  -D BUILD_LIST="core,imgcodecs,imgproc,core,cudafeatures2d,cudaarithm,cudafilters,cudaimgproc,cudawarping,cudev,features2d,imgcodecs,imgproc"  -D WITH_CUDA=ON  -D WITH_CUDNN=ON  -D WITH_CUBLAS=ON  -D CUDA_ARCH_BIN=7.5,8.0,8.6  -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=/usr/local 

RUN apt install ninja-build
RUN apt-get install -y google-perftools --fix-missing && apt -y install libgoogle-perftools-dev --fix-missing
RUN apt-get install -y zlib1g-dev && apt-get install -y libzip-dev
RUN apt install -y python3.8-venv

#RUN apt install -y pocl-opencl-icd
#RUN apt install -y ocl-icd-opencl-dev
#RUN apt install -y libopencv-dev

#RUN apt-get install -y build-essential unzip pkg-config
#RUN apt-get install -y libjpeg-dev libpng-dev libtiff-dev
#RUN apt-get install -y libavcodec-dev libavformat-dev libswscale-dev
#RUN apt-get install -y libv4l-dev libxvidcore-dev libx264-dev
#RUN apt-get install -y libgtk-3-dev
#RUN apt-get install -y libatlas-base-dev gfortran
#RUN apt-get install -y python3-dev

WORKDIR /workspace