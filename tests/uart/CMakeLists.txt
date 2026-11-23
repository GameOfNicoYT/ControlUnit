cmake_minimum_required(VERSION 3.22)
project(ControlUnitUartTests LANGUAGES CXX)

enable_testing()

set(transport_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../source/libraries/custom/Communication")
add_executable(uart_transport_test UartTransportTest.cpp "${transport_dir}/UartTransport.cpp")
target_include_directories(uart_transport_test PRIVATE fakes "${transport_dir}")
target_compile_features(uart_transport_test PRIVATE cxx_std_17)

add_test(NAME uart-transport COMMAND uart_transport_test)
