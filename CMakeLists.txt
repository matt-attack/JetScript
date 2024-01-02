cmake_minimum_required(VERSION 3.5.1)
project (jetscript)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

add_library(jetlib 
  Compiler.cpp
  Expressions.cpp
  JetContext.cpp
  Lexer.cpp
  GarbageCollector.cpp
  Object.cpp
  Parselets.cpp
  Parser.cpp
  Value.cpp
  Libraries/File.cpp
)
target_include_directories(jetlib PUBLIC include/)
set_property(TARGET jetlib PROPERTY CXX_STANDARD 11)

add_executable(${PROJECT_NAME} AsmVM.cpp)
add_dependencies(${PROJECT_NAME} jetlib)
target_include_directories(${PROJECT_NAME} PUBLIC include/)
target_link_libraries(${PROJECT_NAME} jetlib)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "d")

function(my_export_target _target _include_dir)
    file(
        WRITE "${CMAKE_CURRENT_BINARY_DIR}/${_target}Config.cmake"
        "
            include(\"\$\{CMAKE_CURRENT_LIST_DIR\}/${_target}Targets.cmake\")
            set_property(
                TARGET ${_target}
                APPEND PROPERTY
                    INTERFACE_INCLUDE_DIRECTORIES \"${_include_dir}\"
            )
            set(${_target}_INCLUDE_DIRS \"${_include_dir}\")
            set(${_target}_LIBRARIES ${_target})
        "
    )

    export(TARGETS ${_target} FILE "${CMAKE_CURRENT_BINARY_DIR}/${_target}Targets.cmake")

    # NOTE: The following call can pollute your PC's CMake package registry
    export(PACKAGE ${_target})
endfunction(my_export_target)

my_export_target(jetlib "${CMAKE_CURRENT_SOURCE_DIR}/include")
