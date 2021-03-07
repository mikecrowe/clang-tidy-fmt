set(LLVM_LINK_COMPONENTS support)

add_clang_executable(trace-convert
  TraceConvert.cpp
  )
target_link_libraries(trace-convert
  PRIVATE
  clangTooling
  clangBasic
  clangASTMatchers
  )
