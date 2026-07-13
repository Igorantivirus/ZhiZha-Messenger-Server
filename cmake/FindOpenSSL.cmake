# The OpenSSL install prefix comes in through the regular CMAKE_PREFIX_PATH
# (see CMakeUserPresets.json). When cross-compiling (Android NDK) package
# search is confined to the sysroot, so the prefixes must also be search
# roots.
if(CMAKE_CROSSCOMPILING AND CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_FIND_ROOT_PATH ${CMAKE_PREFIX_PATH})
endif()

# CONFIG: use the OpenSSLConfig.cmake shipped with the OpenSSL build.
# Unlike CMake's FindOpenSSL module it knows the static-link dependencies
# (ws2_32, crypt32, ...).
find_package(OpenSSL REQUIRED CONFIG)

# The config only knows the release libraries; wire up the debug variants
# (libssld/libcryptod) living next to them so Debug configs do not mix
# /MT and /MTd runtimes on MSVC.
foreach(_ossl_target OpenSSL::SSL OpenSSL::Crypto)
    get_target_property(_ossl_release ${_ossl_target} IMPORTED_LOCATION)
    string(REGEX REPLACE "\\.(lib|a)$" "d.\\1" _ossl_debug "${_ossl_release}")
    if(EXISTS "${_ossl_debug}")
        set_target_properties(${_ossl_target} PROPERTIES
            IMPORTED_LOCATION_DEBUG "${_ossl_debug}"
        )
    endif()
    unset(_ossl_release)
    unset(_ossl_debug)
endforeach()