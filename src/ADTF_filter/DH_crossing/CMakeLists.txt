cmake_minimum_required(VERSION 2.4)
project(crossing)
find_package(ADTF REQUIRED)

set(FILTER_NAME DH_crossing)

# include directories
include_directories(${OpenCV_INCLUDE_DIR}) 

# add the source
adtf_add_filter(${FILTER_NAME}
                stdafx.cpp
                stdafx.h
                crossing.h
                crossing.cpp
 )

# linking libs
target_link_libraries(${FILTER_NAME} ${OpenCV_LIBS})

# set folder (only needed and available for MSVC)
adtf_set_folder(${FILTER_NAME} aadcbase/imageproc) 

# Specify where it should be installed to
adtf_install_plugin(${FILTER_NAME} ${AADC_INSTALL_DIR})
