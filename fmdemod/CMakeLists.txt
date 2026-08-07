cmake_minimum_required (VERSION 3.30)

add_executable(fmdemod fmdemod.cpp wav_file.cpp)
target_link_libraries(fmdemod PRIVATE fmt::fmt)