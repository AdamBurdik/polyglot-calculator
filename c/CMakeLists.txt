cmake_minimum_required(VERSION 3.5)
project(polyglotcalc C)
set(CMAKE_C_STANDARD 99)
find_package(raylib REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Python3 COMPONENTS Development.Embed REQUIRED)
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)

set(JAVA_HOME "/usr/lib64/jvm/java-25-openjdk")
set(JVM_LIB_DIR "${JAVA_HOME}/lib/server")
set(JVM_INCLUDE_DIR "${JAVA_HOME}/include")

pkg_check_modules(MHD REQUIRED libmicrohttpd)
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.c")
set(PROJECT_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/include/")
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
	${PROJECT_INCLUDE}
	${Python3_INCLUDE_DIRS}
	${MHD_INCLUDE_DIRS}
	${JVM_INCLUDE_DIR} 
    ${JVM_INCLUDE_DIR}/linux
	
)
target_link_libraries(polyglotcalc PRIVATE
	raylib
	${MHD_LIBRARIES}
	m
	Threads::Threads
	Python3::Python
	${JVM_LIB_DIR}/libjvm.so
)
