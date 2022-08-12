add_library(mbed-CCS811 INTERFACE)

target_include_directories(mbed-CCS811
    INTERFACE
        .
)

target_sources(mbed-CCS811
    INTERFACE
        CCS811.cpp
)
