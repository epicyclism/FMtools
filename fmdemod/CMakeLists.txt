cmake_minimum_required (VERSION 3.30)

add_executable(fmdemod fmdemod.cpp)
target_link_libraries(fmdemod PRIVATE fmt::fmt common)

add_executable(fm2 fm2.cpp)
target_link_libraries(fm2 PRIVATE fmt::fmt common)