set(LLVM_LINK_COMPONENTS
  Support
)

add_clang_executable(extract-blocks 
  ExtractBlocks.cpp
  Common.cpp
)

ADD_DEFINITIONS(-fexceptions)
REMOVE_DEFINITIONS(-fno-exceptions)

target_link_libraries(extract-blocks
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangSerialization
  clangTooling
  clangToolingCore
  clangRewrite
  LLVMFrontendOpenMP
)