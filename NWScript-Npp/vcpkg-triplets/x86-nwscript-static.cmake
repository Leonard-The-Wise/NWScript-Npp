set(VCPKG_TARGET_ARCHITECTURE x86)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_CONFIGURE_OPTIONS "-DwxUSE_SOCKETS=0" "-DwxUSE_IPV6=0" "-DwxUSE_FS_INET=0" "-DwxUSE_PROTOCOL=0" "-DwxUSE_URL=0" "-DwxUSE_ALL_THEMES=1" "-DwxUSE_WINSOCK2=0")