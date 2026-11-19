cmake_minimum_required(VERSION 3.5)
project(backend_test CXX)

# The API tests call the real controllers, so their sources are compiled in.
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../controllers TEST_CTL_SRC)

add_executable(${PROJECT_NAME} test_main.cc api_test.cc ${TEST_CTL_SRC})
target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# ##############################################################################
# If you include the drogon source code locally in your project, use this method
# to add drogon
# target_link_libraries(${PROJECT_NAME}_test PRIVATE drogon)
#
# and comment out the following lines
target_link_libraries(${PROJECT_NAME}
    PRIVATE Drogon::Drogon PkgConfig::SODIUM)

ParseAndAddDrogonTests(${PROJECT_NAME})
