add_library(controlunit_custom STATIC
    Rails/RailPowerLowLevel.cpp
    Rails/RailPower.cpp
    Rails/RailShortCircuitDetector.cpp
    Rails/CurrentSensors.cpp
    Rails/SignalGenerator.cpp
    ControlUnit.cpp
    Communication/UartTransport.cpp
)

target_include_directories(controlunit_custom PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Rails/h
    ${CMAKE_CURRENT_SOURCE_DIR}/Communication/h
)

target_link_libraries(controlunit_custom PUBLIC
    stm32cubemx
)