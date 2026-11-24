cmake_minimum_required(VERSION 3.22)
project(ControlUnitRailTests LANGUAGES CXX)

enable_testing()

set(rails_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../source/libraries/custom/Rails")
add_executable(signal_generator_test SignalGeneratorTest.cpp "${rails_dir}/SignalGenerator.cpp")
target_include_directories(signal_generator_test PRIVATE "${rails_dir}")
target_compile_features(signal_generator_test PRIVATE cxx_std_17)

add_test(NAME signal-generator COMMAND signal_generator_test)
