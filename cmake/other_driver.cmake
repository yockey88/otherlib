## compatibility shim: driver macros live in add_other_driver.cmake (single
##  impl); this used to carry a drifted copy that broke installed consumers
include("${CMAKE_CURRENT_LIST_DIR}/add_other_driver.cmake")
