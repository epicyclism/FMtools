cmake_minimum_required (VERSION 3.30)

add_executable(fmdemod fmdemod.cpp)
target_link_libraries(fmdemod PRIVATE fmt::fmt common)