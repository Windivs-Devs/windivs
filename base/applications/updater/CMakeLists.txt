project(updater)

list(APPEND SOURCE
        main.cpp
        main.h)
set_module_type(updater win32gui UNICODE)
#add_importlibs(advapi32 comctl32 gdi32 wininet user32 shell32 shlwapi ole32 setupapi gdiplus msvcrt kernel32 ntdll)
#add_executable(updater ${SOURCE})
