cmake_minimum_required(VERSION 3.22)
project(ControlUnitUartTests LANGUAGES CXX)

enable_testing()

set(transport_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../source/libraries/custom/Communication")
add_executable(uart_transport_test UartTransportTest.cpp "${transport_dir}/UartTransport.cpp")
target_include_directories(uart_transport_test PRIVATE fakes "${transport_dir}")
target_compile_features(uart_transport_test PRIVATE cxx_std_17)

add_test(NAME uart-transport COMMAND uart_transport_test)

set(app_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../source/app")
set(custom_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../source/libraries/custom")
set(core_inc "${CMAKE_CURRENT_SOURCE_DIR}/../../source/libraries/core/Core/Inc")
add_executable(uart_rail_integration_test
    UartRailIntegrationTest.cpp
    "${app_dir}/ControlUnit.cpp"
    "${custom_dir}/ControlUnit.cpp"
    "${transport_dir}/UartTransport.cpp"
    "${custom_dir}/Rails/SignalGenerator.cpp"
    "${custom_dir}/Rails/RailPower.cpp"
    "${custom_dir}/Rails/RailPowerLowLevel.cpp"
    "${custom_dir}/Rails/CurrentSensors.cpp"
    "${custom_dir}/Rails/RailShortCircuitDetector.cpp"
)
target_include_directories(uart_rail_integration_test PRIVATE fakes "${app_dir}" "${custom_dir}" "${core_inc}")
target_compile_features(uart_rail_integration_test PRIVATE cxx_std_17)

add_test(NAME uart-rail-integration COMMAND uart_rail_integration_test)
