set(CMAKE_OBJC_COMPILER "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang")
set(CMAKE_OBJC_COMPILER_ARG1 "")
set(CMAKE_OBJC_COMPILER_ID "AppleClang")
set(CMAKE_OBJC_COMPILER_VERSION "16.0.0.16000026")
set(CMAKE_OBJC_COMPILER_VERSION_INTERNAL "")
set(CMAKE_OBJC_COMPILER_WRAPPER "")
set(CMAKE_OBJC_STANDARD_COMPUTED_DEFAULT "11")
set(CMAKE_OBJC_EXTENSIONS_COMPUTED_DEFAULT "ON")
set(CMAKE_OBJC_STANDARD_LATEST "23")
set(CMAKE_OBJC_COMPILE_FEATURES "")
set(CMAKE_OBJC90_COMPILE_FEATURES "")
set(CMAKE_OBJC99_COMPILE_FEATURES "")
set(CMAKE_OBJC11_COMPILE_FEATURES "")
set(CMAKE_OBJC17_COMPILE_FEATURES "")
set(CMAKE_OBJC23_COMPILE_FEATURES "")

set(CMAKE_OBJC_PLATFORM_ID "Darwin")
set(CMAKE_OBJC_SIMULATE_ID "")
set(CMAKE_OBJC_COMPILER_FRONTEND_VARIANT "GNU")
set(CMAKE_OBJC_SIMULATE_VERSION "")


set(CMAKE_AR "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/ar")
set(CMAKE_OBJC_COMPILER_AR "")
set(CMAKE_RANLIB "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/ranlib")
set(CMAKE_OBJC_COMPILER_RANLIB "")
set(CMAKE_LINKER "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/ld")
set(CMAKE_LINKER_LINK "")
set(CMAKE_LINKER_LLD "")
set(CMAKE_OBJC_COMPILER_LINKER "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/ld")
set(CMAKE_OBJC_COMPILER_LINKER_ID "AppleClang")
set(CMAKE_OBJC_COMPILER_LINKER_VERSION 1115.7.3)
set(CMAKE_OBJC_COMPILER_LINKER_FRONTEND_VARIANT GNU)
set(CMAKE_MT "")
set(CMAKE_TAPI "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/tapi")
set(CMAKE_COMPILER_IS_GNUOBJC )
set(CMAKE_OBJC_COMPILER_LOADED 1)
set(CMAKE_OBJC_COMPILER_WORKS TRUE)
set(CMAKE_OBJC_ABI_COMPILED TRUE)

set(CMAKE_OBJC_COMPILER_ENV_VAR "OBJC")

set(CMAKE_OBJC_COMPILER_ID_RUN 1)
set(CMAKE_OBJC_SOURCE_FILE_EXTENSIONS m)
set(CMAKE_OBJC_IGNORE_EXTENSIONS h;H;o;O)
set(CMAKE_OBJC_LINKER_PREFERENCE 5)
set(CMAKE_OBJC_LINKER_DEPFILE_SUPPORTED FALSE)

foreach (lang C CXX OBJCXX)
  foreach(extension IN LISTS CMAKE_OBJC_SOURCE_FILE_EXTENSIONS)
    if (CMAKE_${lang}_COMPILER_ID_RUN)
      list(REMOVE_ITEM CMAKE_${lang}_SOURCE_FILE_EXTENSIONS ${extension})
    endif()
  endforeach()
endforeach()

# Save compiler ABI information.
set(CMAKE_OBJC_SIZEOF_DATA_PTR "8")
set(CMAKE_OBJC_COMPILER_ABI "")
set(CMAKE_OBJC_BYTE_ORDER "LITTLE_ENDIAN")
set(CMAKE_OBJC_LIBRARY_ARCHITECTURE "")

if(CMAKE_OBJC_SIZEOF_DATA_PTR)
  set(CMAKE_SIZEOF_VOID_P "${CMAKE_OBJC_SIZEOF_DATA_PTR}")
endif()

if(CMAKE_OBJC_COMPILER_ABI)
  set(CMAKE_INTERNAL_PLATFORM_ABI "${CMAKE_OBJC_COMPILER_ABI}")
endif()

if(CMAKE_OBJC_LIBRARY_ARCHITECTURE)
  set(CMAKE_LIBRARY_ARCHITECTURE "")
endif()





set(CMAKE_OBJC_IMPLICIT_INCLUDE_DIRECTORIES "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/16/include;/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.2.sdk/usr/include;/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include")
set(CMAKE_OBJC_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_OBJC_IMPLICIT_LINK_DIRECTORIES "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.2.sdk/usr/lib;/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.2.sdk/usr/lib/swift")
set(CMAKE_OBJC_IMPLICIT_LINK_FRAMEWORK_DIRECTORIES "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.2.sdk/System/Library/Frameworks")
