cmake_minimum_required (VERSION 3.30)

set(common_src 
    BandpassFilter.h 
    basic_file.h 
    constants.h 
    exception_sys_err.h 
    FMDemodFunctions.h 
    HilbertTransformer.h 
    HRException.h 
#    media_file.cpp 
#    media_file.h 
    mm_file.h 
    wav_file.cpp 
    wav_file.h
    audio_file_reader.h
    audio_file_reader.cpp
)

add_library (common STATIC ${common_src})
#target_link_libraries(common PRIVATE fmt::fmt ${FFMPEG_LIBRARIES})
target_link_libraries(common PRIVATE fmt::fmt miniaudio)