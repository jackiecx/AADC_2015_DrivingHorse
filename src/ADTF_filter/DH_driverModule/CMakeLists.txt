set(FILTER_NAME DH_driverModule)

adtf_add_filter(${FILTER_NAME}
            stdafx.cpp
            stdafx.h
            juryEnums.h
            cDHDriverModule.cpp            
            cDHDriverModule.h
            Maneuverlist.h
            ${HEADER_MOC}
)


# set folder (only needed and available for MSVC)
adtf_set_folder(${FILTER_NAME} aadcuser/stateController) 

adtf_install_plugin(${FILTER_NAME} ${AADC_INSTALL_DIR})
    




