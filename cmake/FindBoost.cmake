if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost QUIET)

if(NOT Boost_FOUND)
    if(Boost_ROOT)
        message(STATUS "Boost not found in system. Using manual path: ${Boost_ROOT}")

        # Создаем "импортированную" цель. 
        # Она ведет себя как настоящая библиотека, но живет только в этом проекте.
        if(NOT TARGET Boost::headers)
            add_library(Boost::headers INTERFACE IMPORTED)
            set_target_properties(Boost::headers PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${Boost_ROOT}"
            )
        endif()
    else()
        message(FATAL_ERROR "Boost not found! Please install it or set -DBoost_ROOT")
    endif()
endif()

find_package(Threads REQUIRED)
