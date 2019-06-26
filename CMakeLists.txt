cmake_minimum_required(VERSION 2.8.9)
project (jetscript)

set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")


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

add_executable(${PROJECT_NAME} AsmVM.cpp)
add_dependencies(${PROJECT_NAME} jetlib)
target_link_libraries(${PROJECT_NAME} jetlib)
