project(updater)

list(APPEND SOURCE
        main.cpp
        main.h)
add_executable(updater ${SOURCE})
set_module_type(updater win32gui UNICODE)
#add_importlibs(msvcrt kernel32 ntdll)
add_cd_file(TARGET updater DESTINATION reactos/system32 FOR all)
