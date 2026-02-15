cmake_minimum_required(VERSION 3.20)
project("Test-Project" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(EXECUTABLE_NAME ${PROJECT_NAME})

set(CPP_FILES
    src/main.cpp
    src/ChatServer.cpp
    src/core/Room.cpp
    src/protocol/JsonPacker.cpp
    src/protocol/JsonParser.cpp
)

set(HPP_FILES
    src/ChatServer.hpp
    src/core/Room.hpp
    src/core/Types.hpp
    src/core/UserContext.hpp
    src/protocol/JsonPacker.hpp
    src/protocol/JsonParser.hpp
    src/protocol/JsonMessages.hpp
)

# CROW
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Crow REQUIRED)
#end CROW

#nlohmann_json
if(WIN32)
    find_package(nlohmann_json 3.12.0 REQUIRED)
    add_library(nlohmann_json INTERFACE IMPORTED)
else()
    find_package(nlohmann_json REQUIRED)
endif()
#end nlohmann_json

find_package(Boost 1.70 REQUIRED COMPONENTS system)

add_executable(${EXECUTABLE_NAME} ${CPP_FILES} ${HPP_FILES})

target_link_libraries(${EXECUTABLE_NAME}
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    Crow::Crow
    nlohmann_json::nlohmann_json
)
target_include_directories(${EXECUTABLE_NAME} PUBLIC src/)

# Подключаем директории include
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${Boost_INCLUDE_DIRS})

# Определения для header-only режима
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE 
    BOOST_ASIO_NO_DEPRECATED
    BOOST_BEAST_USE_STD_STRING_VIEW
)

# Линкуем только system (beast и asio header-only)
if(TARGET Boost::system)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE Boost::system)
else()
    # Для старых версий CMake
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${Boost_SYSTEM_LIBRARY})
endif()