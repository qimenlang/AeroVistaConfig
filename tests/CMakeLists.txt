set(TARGET_NAME aerovistaConfigTests)

add_executable(${TARGET_NAME}
    Common.h
    HostConfigTests.cpp
    EngineChannelConfigTests.cpp
    EntityCatalogTests.cpp
    GroundGridConfigTests.cpp
)

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${TARGET_NAME}
    PRIVATE
        AeroVistaConfig
        Catch2::Catch2WithMain
)

if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE /utf-8)
endif()

add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
