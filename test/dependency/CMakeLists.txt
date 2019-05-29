cmake_minimum_required(VERSION 3.10)

project(Dependency
        LANGUAGES C CXX)

find_package(Utopia REQUIRED)

enable_testing()
add_unit_test(NAME my_unit_test
              GROUP dependency
              SOURCES unit_test.cc)

add_model(DependencyTest DependencyTest.cc)
