
What is Windivs?

Windivs™ is an Open Source effort to develop a quality operating system that is compatible with applications and drivers written for the Microsoft® Windows™ NT family of operating systems (NT4, 2000, XP, 2003, Vista, 7).

The Windivs project, although currently focused on Windows Server 2003 compatibility, is always keeping an eye toward compatibility with Windows Vista and future Windows NT releases.

The code of Windivs is licensed under GNU GPL 2.0.

Product quality warning
Windivs is currently an Alpha quality operating system. This means that Windivs is under heavy development, and you have to be ready to encounter some problems. Different things may not work well, and it can corrupt the data present on your hard disk. It is HIGHLY recommended to test Windivs on a virtual machine or on a computer with no sensitive or critical data!

Building
Build rosbewin.badge rosbeunix.badge coverity.badge

To build the system, it is strongly advised to use the Windivs Build Environment (WinBE). Up-to-date versions for Windows and for Unix/GNU-Linux are available from our download page at: "Build Environment".

Alternatively, one can use Microsoft Visual C++ (MSVC) version 2015+. Building with MSVC is covered here: "Visual Studio or Microsoft Visual C++".

See the "Building Windivs" article for more details.

Binaries
To build Windivs, you must run the configure script in the directory you want to have your build files. Choose configure.cmd or configure.sh depending on your system. Then run ninja <modulename> to build a module you want or just ninja to build all modules.

Bootable images
To build a bootable CD image, run ninja bootcd from the build directory. This will create a CD image with a filename bootcd.iso.

You can always download fresh binary builds of bootable images from the "Daily builds" page.

Installing
By default, Windivs currently can only be installed on a machine that has a FAT16 or FAT32 partition as the active (bootable) partition. The partition on which Windivs is to be installed (which may or may not be the bootable partition) must also be formatted as FAT16 or FAT32. Windivs Setup can format the partitions if needed.

Starting with 0.4.10, Windivs can be installed using the BtrFS file system. But consider this as an experimental feature, and thus regressions not triggered on FAT setup may be observed.

To install Windivs from the bootable CD distribution, extract the archive contents. Then burn the CD image, boot from it, and follow the instructions.

See the "Installing Windivs" Wiki page or INSTALL for more details.

Testing
If you discover a bug in Windivs, search on JIRA first - it might be reported already. If not, report the bug providing logs and as much information as possible.

See "File Bugs" for a guide.

NOTE: The bug tracker is not for discussions. Please use our official chat or our forum.

Contributing prwelcome.badge
We are always looking for developers! Check how to contribute if you are willing to participate.

Legal notice: If you have seen proprietary Microsoft Windows source code (including but not limited to the leaked Windows NT 3.5, NT 4, 2000 source code and the Windows Research Kernel), your contribution won't be accepted because of potential copyright violation.

Try out cloud-based Windivs development using Gitpod and Docker:

Open in Gitpod

You can also support Windivs by donating! We rely on our backers to maintain our servers and accelerate development by hiring full-time devs.

More information
Windivs is a Free and Open Source operating system based on the Windows architecture, providing support for existing applications and drivers, and an alternative to the current dominant consumer operating system.

It is not another wrapper built on Linux, like WINE. It does not attempt or plan to compete with WINE; in fact, the user-mode part of Windivs is almost entirely WINE-based, and our two teams have cooperated closely in the past.

Windivs is also not "yet another OS." It does not attempt to be a third player like any other alternative OS out there. People are not meant to uninstall Linux and use Windivs instead; Windivs is a replacement for Windows users who want a Windows replacement that behaves just like Windows.

More information is available at: windivs.org.

Also see the media/doc subdirectory for some sparse notes.
