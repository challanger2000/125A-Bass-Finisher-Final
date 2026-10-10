cmake_minimum_required(VERSION 3.25)
project(BassFinisherPerformanceMatrix LANGUAGES CXX)

# Build identical test harness against separate frozen V3.0.0 and V3.0.1
# source trees with the same compiler flags on the same Windows CI runner.
if(NOT DEFINED BASS_DSP_ROOT)
    message(FATAL_ERROR "Pass -DBASS_DSP_ROOT=<checked-out-repo>")
endif()

file(TO_CMAKE_PATH "${BASS_DSP_ROOT}" BASS_NORMALIZED_DSP_ROOT)

set(BASS_DSP_FILES
    AdaptiveBandController.cpp
    AdaptiveResonanceSuppressor.cpp
    AutoLevelCompensator.cpp
    ToneMatchAnalyzer.cpp
    ToneMatchDSP.cpp
    MetalFinisherDSP.cpp
)

foreach(file IN LISTS BASS_DSP_FILES)
    list(APPEND BASS_ABSOLUTE_DSP_FILES
        "${BASS_NORMALIZED_DSP_ROOT}/src/dsp/${file}")
endforeach()

add_executable(BassFinisherPerformanceMatrix
    "${CMAKE_CURRENT_LIST_DIR}/../performance_matrix.cpp"
    ${BASS_ABSOLUTE_DSP_FILES}
)
target_include_directories(BassFinisherPerformanceMatrix
    PRIVATE "${BASS_NORMALIZED_DSP_ROOT}/src/dsp")
target_compile_features(BassFinisherPerformanceMatrix
    PRIVATE cxx_std_17)
if(MSVC)
    target_compile_options(BassFinisherPerformanceMatrix
        PRIVATE /O2 /W4 /permissive-)
else()
    target_compile_options(BassFinisherPerformanceMatrix
        PRIVATE -O3 -Wall -Wextra -Wpedantic)
endif()
