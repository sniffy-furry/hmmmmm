# Custom vcpkg triplet: x64-windows, but RELEASE ONLY.
#
# The stock x64-windows triplet builds every dependency twice (Release + Debug).
# For a heavy from-source port like ffmpeg that doubles build time for a Debug
# variant this toolkit never ships. VCPKG_BUILD_TYPE=release skips it entirely.
#
# Used via:  vcpkg install --triplet x64-windows-rel --overlay-triplets triplets
# and CMake: -DVCPKG_TARGET_TRIPLET=x64-windows-rel -DVCPKG_OVERLAY_TRIPLETS=triplets
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE   dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_BUILD_TYPE    release)
