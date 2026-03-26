cmake_minimum_required(VERSION 3.21)

# -------------------------------------------------------
# Projeto
# -------------------------------------------------------
project(AviwaSketchUpAPI
    VERSION     1.0.0
    DESCRIPTION "SketchUp API REST server via Oat++"
    LANGUAGES   CXX
)

# -------------------------------------------------------
# Padrão C++
# -------------------------------------------------------
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)  # evita extensões GNU não portáveis

# -------------------------------------------------------
# Dependências via vcpkg
# -------------------------------------------------------
find_package(oatpp CONFIG REQUIRED)

# -------------------------------------------------------
# SDKs externos
# -------------------------------------------------------
set(EXTERNAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external")

find_library(SKETCHUP_LIB
    NAMES SketchUpAPI
    PATHS "${EXTERNAL_ROOT}/lib/x64"
    NO_DEFAULT_PATH
)
if(NOT SKETCHUP_LIB)
    message(FATAL_ERROR
        "SketchUpAPI.lib nao encontrado.\n"
        "Caminho esperado: ${EXTERNAL_ROOT}/lib/x64"
    )
endif()

# -------------------------------------------------------
# Fontes
# -------------------------------------------------------
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp")

if(NOT SOURCE_FILES)
    message(FATAL_ERROR "Nenhum arquivo .cpp encontrado em src/")
endif()

# -------------------------------------------------------
# Executável
# -------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# -------------------------------------------------------
# Includes
# -------------------------------------------------------
target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
    SYSTEM PRIVATE
        "${EXTERNAL_ROOT}/include"
)

# -------------------------------------------------------
# Linkagem
# -------------------------------------------------------
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        oatpp::oatpp
        ${SKETCHUP_LIB}
)

# -------------------------------------------------------
# Definições de compilação
# -------------------------------------------------------
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        PROJECT_ROOT_DIR=R"(${CMAKE_CURRENT_SOURCE_DIR})"
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>  # só no MSVC
)

# -------------------------------------------------------
# Warnings
# -------------------------------------------------------
target_compile_options(${PROJECT_NAME}
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W3>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# -------------------------------------------------------
# DLLs — cópia automática para a pasta de saída
# -------------------------------------------------------
file(GLOB EXTERNAL_DLLS CONFIGURE_DEPENDS "${EXTERNAL_ROOT}/bin/x64/*.dll")

foreach(DLL ${EXTERNAL_DLLS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copiando ${DLL}..."
    )
endforeach()

# -------------------------------------------------------
# Debugger — PATH para DLLs sem copiar (VS / VS Code)
# -------------------------------------------------------
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_ENVIRONMENT         "PATH=${EXTERNAL_ROOT}/bin/x64;$ENV{PATH}"
    VS_DEBUGGER_WORKING_DIRECTORY   "${CMAKE_CURRENT_SOURCE_DIR}"
)

# -------------------------------------------------------
# Info de build
# -------------------------------------------------------
message(STATUS "Projeto  : ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Compiler : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build    : ${CMAKE_BUILD_TYPE}")
message(STATUS "Fontes   : ${SOURCE_FILES}")