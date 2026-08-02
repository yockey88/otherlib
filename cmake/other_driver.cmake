## Compatibility shim: the driver macros live in add_other_driver.cmake (one
## implementation for both in-tree and installed-SDK builds). This file used to
## carry a drifted copy that broke installed consumers.
include("${CMAKE_CURRENT_LIST_DIR}/add_other_driver.cmake")
