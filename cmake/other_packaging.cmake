###
##  CPack packaging for the Other Environment SDK.
##
##  Drives the install rules into distributable artifacts under build/packages/
##  (invoked through `oecli package`, or cpack directly from the build directory).
##  ZIP works everywhere; the NSIS generator produces the Windows installer and
##  needs NSIS (makensis) on the machine: oecli package -G "NSIS;ZIP"
##
set(CPACK_PACKAGE_NAME "OtherEnvironment")
set(CPACK_PACKAGE_VENDOR "Other Environment")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Other Environment - a game development framework for C++")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_FILE_NAME "OtherEnvironment-${PROJECT_VERSION}-windows-x64")

## the installer's default target is C:\OtherEnvironment, matching both the cmake
##  default install prefix and the default root oecli probes for an installed SDK
set(CPACK_PACKAGE_INSTALL_DIRECTORY "OtherEnvironment")
set(CPACK_NSIS_INSTALL_ROOT "C:")
set(CPACK_NSIS_DISPLAY_NAME "Other Environment")
set(CPACK_NSIS_PACKAGE_NAME "Other Environment")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
## offers to put <prefix>/bin (oecli) on PATH during install
set(CPACK_NSIS_MODIFY_PATH ON)

set(CPACK_GENERATOR "ZIP")

include(CPack)
