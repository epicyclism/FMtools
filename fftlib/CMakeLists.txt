cmake_minimum_required (VERSION 3.30)

add_library (fftlib fftlib.cpp fftlib.h FFT.h FFTImpl.h ProcFFT.h ProcFFTImpl.h)
target_link_libraries(fftlib PRIVATE Boost::math)
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET fftlib PROPERTY CXX_STANDARD 20)
endif()
