cmake_minimum_required( VERSION 3.14 )

project( TcpIpTransport LANGUAGES CXX )

###########
### Library
###########
message( "${CYN}Configuring library : ${PROJECT_NAME}${NOMR}" )
file( GLOB CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp )
add_library(
    ${PROJECT_NAME}
STATIC
    ${CPP_FILES}
)
target_include_directories(
    ${PROJECT_NAME}
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(
    ${PROJECT_NAME}
LINK_PRIVATE
    Tools
    ${Boost_LIBRARIES}
)
if( MINGW )
  target_link_libraries(
    ${PROJECT_NAME}
LINK_PUBLIC
    ws2_32
    wsock32
)
endif()

########
### Test
########
if( WIN32 )
    set( EXECUTABLE_NAME ${PROJECT_NAME}Test )
else()
    set( EXECUTABLE_NAME ${PROJECT_NAME}Test.out )
endif()

message( "${MAG}Configuring test : ${EXECUTABLE_NAME}${NORM}" )
include_directories( ${Boost_INCLUDE_DIRS} )
message( "${BLU}Boost headers : ${Boost_INCLUDE_DIRS}${NORM}" )
message( "${BLU}Located boost compiled libs : ${Boost_LIBRARIES}${NORM}" )

file( GLOB CXX_FILES ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp )
add_executable(
    ${EXECUTABLE_NAME}
    ${CXX_FILES}
)

target_link_libraries (
    ${EXECUTABLE_NAME}
LINK_PUBLIC
    ${PROJECT_NAME}
    Tools
    ${Boost_LIBRARIES}
)

if( MINGW )
  target_link_libraries(
    ${EXECUTABLE_NAME}
LINK_PUBLIC
    ws2_32
    wsock32
)
endif()