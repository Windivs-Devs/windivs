project(updater)

list(APPEND SOURCE
        main.cpp
        main.h)
#set_module_type(updater win32gui UNICODE)
#add_importlibs(msvcrt kernel32 ntdll)
#add_executable(updater ${SOURCE})
