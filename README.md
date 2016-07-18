## ReaDDy

ReaDDy is a Python / C++ based particle reaction-diffusion simulator.
*Currently, it is under construction and should not be expected to be fully functional.* For a working and more feature complete software, please see the [Java implementation of ReaDDy](https://github.com/readdy/readdy_java).

### Status
| Travis | <span style="color:gray;">Appveyor</span> |
| --- | --- |
|[![Build Status](https://travis-ci.org/readdy/readdy.svg?branch=master)](https://travis-ci.org/readdy/readdy) | not yet supported |

### Dependencies
- HDF5
- boost
- cmake
- *optional*: python (2 or 3), numpy (for python bindings)
- *testing*: gtest (included by git submodule)

### Project structure
```
readdy/
|   README.md
|   ...
|
|___kernels/
|   |
|   |___cpu/
|___|___|___include/
|   |   |   (kernel includes)
|___|___|___src/
|   |   |   (kernel sources)
|___|___|___test/
|   |   |   (kernel tests)
|   |
|   |___cuda/
|   |   |   (yet to be added)
|
|___include/
|   |   *.h (core library includes)
|
|___readdy/
|   |
|___|___main/
|   |   |   (core library sources)
|___|___test/
|   |   |   (core library tests)
|
|___libraries/
|   |   (gtest, boost.DLL)
|
|___wrappers/
|___|___python/
|   |   |   (code for python api)
|___|___|___src/
|___|___|___|___cxx/
|   |   |   |   |   (c++ code for the python module)
|   |   |   |
|___|___|___|___python/
|   |   |   |   |   (python top-level api)

```
### Building

Before building, it should be noted that the build process will download
boost and store it in a cache directory. Per default, this is "${CMAKE_BINARY_DIR}/download/external/boost", where CMAKE_BINARY_DIR corresponds to the directory in which cmake was executed.

* Build by conda-build
  * Optionally set the boost cache dir by setting the *environment* variable "BOOST_DOWNLOAD_CACHE_DIR".
  * Install conda-build and then execute the conda recipe:
```bash
conda install conda-build
conda-build PATH_TO_READDY/tools/conda-recipe
```
* Build by using cmake
	* This type of build is suggested if one is interested in development of the software. There are a number of CMake options that influence the type of build:
		- READDY_CREATE_TEST_TARGET:BOOL (default ON) determining if the test targets should be generated
		- READDY_CREATE_MEMORY_CHECK_TEST_TARGET:BOOL (default OFF) determining if the test targets should be additionally called through valgrind. Requires the previous option to be ON and valgrind to be installed.
		- READDY_INSTALL_UNIT_TEST_EXECUTABLE:BOOL (default OFF) determining if the unit test executables should be installed. This is option is mainly important for the conda recipe.
		- READDY_BUILD_SHARED_COMBINED:BOOL (default OFF) determining if the core library should be built monolithically or as separated shared libraries.
		- READDY_BUILD_PYTHON_WRAPPER:BOOL (default ON) determining if the python wrapper should be built
		- READDY_DEBUG_PYTHON_MODULES:BOOL (default OFF) if this flag is set to ON, the generated python module will be placed in-source rather than in the output directory to enable faster development.
		- READDY_DEBUG_CONDA_ROOT_DIR:PATH (default "") this option is to be used in conjunction with the previous option and only has effect if it is set to ON. It should point to the conda environment which is used for development and then effects the output directory of the binary files such that they get compiled directly into the respective environment.
		- READDY_GENERATE_DOCUMENTATION_TARGET:BOOL (default OFF): Determines if the documentation target should be generated or not, which, if generated, can be called by "make doc".
		- READDY_GENERATE_DOCUMENTATION_TARGET_ONLY:BOOL (default OFF): This option has the same effect as the previous option, just that it does not need any dependencies other than doxygen to be fulfilled and generates the documentation target exclusively.
		- READDY_LOG_CMAKE_CONFIGURATION:BOOL (default OFF): This option determines if the status of relevant cmake cache variables should be logged at configuration time or not.
		- BOOST_DOWNLOAD_CACHE_DIR:PATH (default "${CMAKE_BINARY_DIR}/download/external/boost"): Cache variable determining the boost cache dir. If one can expect the output directory to be cleaned regularly, one should set this path to some other location.
		- READDY_KERNELS_TO_TEST:STRING (default "SingleCPU,CPU"): Comma separated list of kernels against which the core library should be tested within the test targets.
		- *advanced*: INCLUDE_PERFORMANCE_TESTS:BOOL (default OFF): Flag indicating if the performance tests should be part of the unit test target or not.
    * After having configured the cmake cache variables, one can invoke cmake and make and compile the project.
    * Altogether, a shell script invoking cmake with modified parameters in an environment with multiple python versions could look like [this](tools/dev/configure.sh).