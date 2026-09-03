cmake_minimum_required (VERSION 3.30)

add_executable(sig_gen SigGen.cpp)
target_link_libraries(sig_gen PRIVATE fmt::fmt)

add_executable(dsp_test DSPTest.cpp)
target_link_libraries(dsp_test PRIVATE fmt::fmt fftlib)

add_executable(fftit fftit.cpp)
target_link_libraries(fftit PRIVATE fmt::fmt fftlib common)

add_executable(fmgenerate FMGenerate.cpp)
target_link_libraries(fmgenerate PRIVATE fmt::fmt)

add_executable(jitter_gen JitterGen.cpp jtestfns.h)
target_link_libraries(jitter_gen PRIVATE fmt::fmt)

add_executable(thd thd.cpp)
target_link_libraries(thd PRIVATE fmt::fmt common ctre)
