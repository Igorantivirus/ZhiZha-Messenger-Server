# A manually built OpenSSL (OPENSSL_ROOT_DIR) ships its own package config
# with correct static dependencies (ws2_32, crypt32, ...); prefer it and
# fall back to CMake's FindOpenSSL module for system installations.
if(OPENSSL_ROOT_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${OPENSSL_ROOT_DIR}")
    if(CMAKE_CROSSCOMPILING)
        # Let the re-rooted config search (Android NDK toolchain) reach it.
        list(APPEND CMAKE_FIND_ROOT_PATH "${OPENSSL_ROOT_DIR}")
    endif()
endif()

find_package(OpenSSL QUIET CONFIG)
if(NOT TARGET OpenSSL::SSL)
    find_package(OpenSSL REQUIRED)
endif()
if(NOT TARGET OpenSSL::SSL OR NOT TARGET OpenSSL::Crypto)
    message(FATAL_ERROR "ASYNCNET_WITH_TLS=ON but OpenSSL was not found! Install it or set -DOPENSSL_ROOT_DIR")
endif()

# Neither the generated OpenSSLConfig.cmake nor FindOpenSSL knows about the
# debug variants (libssld/libcryptod) living next to the release libraries.
# Wire them up so Debug configs do not mix /MT and /MTd runtimes on MSVC.
foreach(_ossl_target OpenSSL::SSL OpenSSL::Crypto)
    get_target_property(_ossl_release ${_ossl_target} IMPORTED_LOCATION)
    if(_ossl_release)
        string(REGEX REPLACE "\\.(lib|a)$" "d.\\1" _ossl_debug "${_ossl_release}")
        if(NOT _ossl_debug STREQUAL _ossl_release AND EXISTS "${_ossl_debug}")
            set_target_properties(${_ossl_target} PROPERTIES
                IMPORTED_LOCATION_DEBUG "${_ossl_debug}"
            )
        endif()
    endif()
    unset(_ossl_release)
    unset(_ossl_debug)
endforeach()
