# AeroVistaConfig — 配置 JSON 契约辅助（nlohmann 语法解析之上）。
# 不依赖 vsg / 引擎 / sync。engine 与 aerovistaSync 共用。

cmake_minimum_required(VERSION 3.20)
project(AeroVistaConfig VERSION 0.1.0 LANGUAGES CXX)

if(NOT TARGET nlohmann_json AND NOT TARGET nlohmann_json::nlohmann_json)
    find_package(nlohmann_json REQUIRED)
endif()

if(TARGET nlohmann_json::nlohmann_json)
    set(AEROVISTA_CONFIG_JSON_TARGET nlohmann_json::nlohmann_json)
elseif(TARGET nlohmann_json)
    set(AEROVISTA_CONFIG_JSON_TARGET nlohmann_json)
endif()

add_library(AeroVistaConfig STATIC
    include/aerovista/config/ConfigJson.h
    include/aerovista/config/IgConfig.h
    include/aerovista/config/HostConfig.h
    include/aerovista/config/EngineChannelConfig.h
    include/aerovista/config/EntityCatalog.h
    src/ConfigJson.cpp
    src/IgConfig.cpp
    src/HostConfig.cpp
    src/EngineChannelConfig.cpp
    src/EntityCatalog.cpp
)
add_library(aerovista::config ALIAS AeroVistaConfig)

target_include_directories(AeroVistaConfig PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(AeroVistaConfig
    PUBLIC
        ${AEROVISTA_CONFIG_JSON_TARGET}
)

target_compile_features(AeroVistaConfig PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(AeroVistaConfig PRIVATE /utf-8)
endif()

# 库自带测试。本仓库与 config 独立构建默认打开；只链接库的工程可关掉。
# 不依赖 vsg / sync。父工程须先提供 Catch2::Catch2WithMain；独立顶层工程否则 FetchContent。
option(AEROVISTA_CONFIG_BUILD_TESTS "Build AeroVistaConfig Catch2 tests" ON)
if(AEROVISTA_CONFIG_BUILD_TESTS)
    if(NOT TARGET Catch2::Catch2WithMain)
        include(FetchContent)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.15.2
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(Catch2)
    endif()
    add_subdirectory(tests)
endif()
