## What is Windivs?

Windivs™ is an Open Source effort to develop a quality operating system that is compatible with applications and drivers written for the Microsoft® Windows™ NT family of operating systems (NT4, 2000, XP, 2003, Vista, 7).

The Windivs project, although currently focused on Windows Server 2003 compatibility, is always keeping an eye toward compatibility with Windows Vista and future Windows NT releases.

The code of Windivs is licensed under GNU GPL 2.0.

> [!WARNING]
> Windivs is currently under heavy development, and you may encounter some issues. Different things may not work well, and it can corrupt the data on your drive. It is **strongly** recommended to try Windivs on a virtual machine or a computer without important data!

## Building
Build rosbewin.badge rosbeunix.badge coverity.badge

> [!NOTE]
> To build the system, it is strongly advised to use the Windivs Build Environment (WinBE). Up-to-date versions for Windows and for Unix/GNU-Linux are available from our download page at "Build Environment".\
> Alternatively, you can use Microsoft Visual C++ (MSVC) version 2015+. Building with MSVC is covered here: "Visual Studio or Microsoft Visual C++".

See the "Building Windivs" article for more details.

## Binaries
To build Windivs, you must run the configure script in the directory you want to have your build files. Choose configure.cmd or configure.sh depending on your system. Then run ninja <modulename> to build a module you want or just ninja to build all modules.

## Bootable images
To build a bootable CD image, run `ninja bootcd` from the build directory. This will create a CD image with the name `bootcd.iso`.

> [!TIP]
> You can always download fresh binary builds of bootable images from GitHub actions.

## Installing
By default, Windivs currently can only be installed on a machine that has a FAT16 or FAT32 partition as the active (bootable) partition. The partition on which Windivs is to be installed (which may or may not be the bootable partition) must also be formatted as FAT16 or FAT32. Windivs Setup can format the partitions if needed.

Starting with 0.4.10, Windivs can be installed using the Btrfs file system. But consider this as an experimental feature, and thus regressions not triggered on FAT setup may be observed.

To install Windivs from the bootable CD distribution, extract the archive contents. Then burn the CD image, boot from it, and follow the instructions.

See the "Installing Windivs" Wiki page or INSTALL for more details.

## Testing
Install the latest Windivs compile from GitHub actions (on the dev branch) to test the OS, If you find a bug, then please file a GitHub issue here, and optionally a pull request!

> [!IMPORTANT]
> The bug tracker is not for discussions. Please use our official chat or the forum.

## Contributing prwelcome.badge
We are always looking for developers! Check how to contribute if you are willing to participate.

> [!CAUTION]
> Legal notice: If you have seen proprietary Microsoft Windows source code (including but not limited to the leaked Windows NT 3.5, NT 4, 2000 source code and the Windows Research Kernel), your contribution won't be accepted because of potential copyright violation.


## More information
Windivs is a Free and Open Source operating system based on the Windows architecture, providing support for existing applications and drivers, and an alternative to the current dominant consumer operating system.

More information is available at: windivs.org.

Also see the media/doc subdirectory for some sparse notes.
