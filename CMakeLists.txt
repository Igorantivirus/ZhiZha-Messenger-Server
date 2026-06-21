cmake_minimum_required(VERSION 3.25)
# cmake_policy(SET CMP0167 NEW)
project(Tests VERSION 0.1.0 LANGUAGES CXX)

# ============== НАСТРОЙКА ==============

set(EXECUTABLE_NAME ${PROJECT_NAME})

set(CPP_FILES
    src/main.cpp
)

set(INCLUDE_PATHS extern src)

# ============== ПОДКЛЮЧЕНИЕ БИБЛИОТЕК ==============

include(cmake/FindBoost.cmake)

find_package(SQLiteCpp CONFIG REQUIRED)

# ============== СОЗДАНИЕ БИНАРНИКА ==============

add_executable(${EXECUTABLE_NAME} ${CPP_FILES})

# ============== TARGET НАСТРОЙКА ==============

set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_definitions(${EXECUTABLE_NAME} PUBLIC
    CROW_USE_BOOST=1
    ZHI_ZHA_SERVER=1
    PROTOCOL_USE_JSON_PARSING=1
)

target_include_directories(${EXECUTABLE_NAME} PUBLIC ${INCLUDE_PATHS})

target_link_libraries(${EXECUTABLE_NAME} PUBLIC
    Boost::headers
    Threads::Threads
    SQLiteCpp
)

if(MSVC)
    target_compile_options(${EXECUTABLE_NAME} PRIVATE /bigobj)
endif()