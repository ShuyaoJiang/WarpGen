set(LLVM_LINK_COMPONENTS support)

add_clang_executable(insert
  Insert.cpp
  # ../common/common.cpp
  # ../common/type.cpp
)

ADD_DEFINITIONS(-fexceptions)
REMOVE_DEFINITIONS(-fno-exceptions)

target_link_libraries(insert
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