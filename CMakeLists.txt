cmake_minimum_required(VERSION 2.8)
Project(PsychoBot)

set(SOURCES
    src/main.cpp
    src/webhooks.cpp
)

set(TESTSRC
)

set(CMAKE_CXX_FLAGS "-std=c++11")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
find_package(Lua REQUIRED)
find_package(Curlpp REQUIRED)
find_package(MicroHttpd REQUIRED)
add_definitions(${MICROHTTPD_DEFINITIONS})

include_directories (
    ${CMAKE_SOURCE_DIR}/include
    ${LUA_INCLUDE_DIR}
    ${CURLPP_INCLUDE_DIRS}
    ${MICROHTTPD_INCLUDE_DIRS}
)

add_executable(
    ${CMAKE_PROJECT_NAME}
    MACOSX_BUNDLE
    ${SOURCES}
)

#math library
if(UNIX)
    target_link_libraries(${CMAKE_PROJECT_NAME} m)
endif(UNIX)

target_link_libraries(${CMAKE_PROJECT_NAME} ${LUA_LIBRARIES})
target_link_libraries(${CMAKE_PROJECT_NAME} ${CURLPP_LIBRARIES})
target_link_libraries(${CMAKE_PROJECT_NAME} ${MICROHTTPD_LIBRARIES})

#install(TARGETS ${CMAKE_PREJECT_NAME} LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
#install(FILES ${CMAKE_SOURCE_DIR}/include/Gif.h DESTINATION include)

######### Build Tests
#find_package(Check)
#if(CHECK_FOUND)
#    enable_testing()

#    foreach(_file ${TESTSRC})
#        string(REPLACE ".check" ".c" _genfile ${_file})
#        string(REPLACE ".check" "" _binary ${_file})
#        string(REGEX REPLACE "(^.*)/([^/]*)\$" "${CMAKE_BINARY_DIR}/\\1" _dir ${_file})
#        add_custom_command(
#            OUTPUT ${_dir}
#            COMMAND mkdir
#            ARGS -p ${_dir}
#        )
#        add_custom_command(
#            OUTPUT ${_genfile}
#            COMMAND checkmk
#            ARGS ${CMAKE_SOURCE_DIR}/${_file} > ${CMAKE_BINARY_DIR}/${_genfile}
#            DEPENDS ${_file} ${_dir}
#        )
#        add_executable(${_binary} ${_genfile} ${SOURCES})
#        target_link_libraries(${_binary} ${CHECK_LIBRARIES} tinygif)
#        add_test(${_binary} ${_binary})
#    endforeach()
#endif(CHECK_FOUND)