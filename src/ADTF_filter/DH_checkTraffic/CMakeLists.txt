set(FILTER_NAME DH_checkTraffic)


# add the source
adtf_add_filter(${FILTER_NAME}
                stdafx.cpp
                stdafx.h
                checkTraffic.h
                checkTraffic.cpp
 )


# Specify where it should be installed to
adtf_install_plugin(${FILTER_NAME} ${AADC_INSTALL_DIR})
