# Generated from tools.pro.

if((android_app OR NOT ANDROID) AND (android_app OR NOT INTEGRITY) AND (NOT ANDROID OR NOT CMAKE_CROSSCOMPILING) AND (NOT ANDROID OR NOT WASM) AND (NOT CMAKE_CROSSCOMPILING OR NOT INTEGRITY) AND (NOT INTEGRITY OR NOT WASM))
    add_subdirectory(balsam)
    add_subdirectory(meshdebug)
    add_subdirectory(shadergen)
endif()
