cmake_minimum_required(VERSION 3.25)
# cmake_policy(SET CMP0167 NEW)
project(Tests VERSION 0.1.0 LANGUAGES CXX)

# ============== НАСТРОЙКА ==============

set(EXECUTABLE_NAME ${PROJECT_NAME})

set(CPP_FILES
    src/main.cpp
    src/App/ServerApplication.cpp
    src/AuthService/AccessTokenStore.cpp
    src/AuthService/AuthService.cpp
    src/AuthService/RefreshTokenStore.cpp
    src/AuthService/TokenService.cpp
    src/AuthService/UserQueryService.cpp
    src/AuthService/UserRepository.cpp
    src/ChatService/ChatQueryService.cpp
    src/ChatService/ChatService.cpp
    src/ChatService/MembersRepository.cpp
    src/ChatService/MessageRepository.cpp
    src/ChatService/RoomPermissions.cpp
    src/ChatService/RoomRepository.cpp
    src/SessionService/SessionService.cpp
    src/Transport/ApiController.cpp
    src/Transport/AuthController.cpp
    src/Transport/Helpers.cpp
    src/Transport/HttpServer.cpp
    src/Transport/MessagesController.cpp
    src/Transport/RoomsController.cpp
    src/Transport/UsersController.cpp
    src/Transport/WsServer.cpp
    src/Utils/PasswordHasher.cpp
    src/Utils/StringRule.cpp
    src/Utils/Validator.cpp
)

set(INCLUDE_PATHS extern src include)

# ============== ПОДКЛЮЧЕНИЕ БИБЛИОТЕК ==============

include(cmake/FindBoost.cmake)
include(cmake/FindOpenSSL.cmake)

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
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
        _WIN32_WINNT=0x0A00 # Windows 10
        WIN32_LEAN_AND_MEAN
    )
endif()