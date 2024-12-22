# WarpGen

## Requirements

### 1. Wasm Environment

* **Runtimes:** [Wasmer](https://github.com/wasmerio/wasmer), [Wasmtime](https://github.com/bytecodealliance/wasmtime), [WasmEdge](https://github.com/WasmEdge/WasmEdge), [WAMR](https://github.com/bytecodealliance/wasm-micro-runtime).

* **Compiler:** [Emscripten](https://emscripten.org).


### 2. Clang Tool Installation
#### 2.1 Clone and compile LLVM project:

```
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_TARGETS_TO_BUILD="host" ../llvm
make -j4
```

#### 2.2 Install clang tools for WarpGen:

* Copy the four folders `common`, `insert`, `use-define-tag`, `extract-blocks` (in `code/clang-tools-extra`) to the folder `clang-tools-extra` in the compiled LLVM project.

*  Add the following statements in `clang-tools-extra/CMakeLists.txt`:
    ```
    add_subdirectory(extract-blocks)
    add_subdirectory(use-define-tag)
    add_subdirectory(insert)
    ```

* Recompile the project under the `build` folder:
    ```
    cd build
    make -j4
    ```


### 3. Others

* JDK: Latest version.

* [Csmith](https://github.com/csmith-project/csmith).



## Running

* The entry of WarpGen is `code/warp/src/Warp.java`.

* Modify the configuration file `code/warp/configure.ini` based on your settings.

* Running WarpGen:
    ```
    jar cvf WarpGen.jar *
    java -cp WarpGen.jar Executor
    ```