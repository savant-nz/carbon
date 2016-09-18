# Building Carbon

On Windows:
- Install [Visual Studio Community 2015](https://visualstudio.com).
    - Ensure that Visual C++ is selected in the installer.
    - The Professional and Enterprise editions of Visual Studio 2015 are also supported, but not the Express editions.
- Install [Python 2.7](http://www.python.org/downloads).
    - Ensure that '*Add python.exe to Path*' is enabled when installing.
    - Python 3.x is not yet supported.
- Install [PyWin32 for Python 2.7](
  http://sourceforge.net/projects/pywin32/files/pywin32/Build%20219/pywin32-219.win32-py2.7.exe/download).
- Install [SCons](http://www.scons.org/download.php), version 2.4 or later is required.
- Install [Ruby](http://rubyinstaller.org/downloads), version 2.0 or later is required.
    - Tick '*Add Ruby executables to your PATH*' and '*Associate .rb and .rbw files with this Ruby installation*' when
      installing.
- Install [Git](https://git-scm.com/download/win).
    - Ensure that '*Use Git from the Windows Command Prompt*' is selected when installing.

On Mac OS X:
- Install [Xcode](https://itunes.apple.com/app/xcode/id497799835) through the Mac App Store.
- Install [Homebrew](http://brew.sh).
- Install [SCons](http://www.scons.org) using Homebrew: `brew install scons`

On Ubuntu Linux:
- Install required packages using the following command: `apt-get install g++ git libopenal-dev libgl1-mesa-dev
  libsdl2-dev libudev-dev libxinerama-dev ruby scons`

Once this setup is complete build the engine's dependencies:

    ruby Dependencies/BuildDependencies.rb

If you are using Mac OS X and want to develop or test on iOS then build the dependencies for that platform also:

    ruby Dependencies/BuildDependencies.rb --platform iOS

After the dependencies have been built open the Visual Studio or Xcode project file in the `Source/` directory and build
and run the sample applications.

Alternatively, build and run sample applications from the command line:

    ruby Scripts/RunApplication.rb BoxesSample

Using the `RunApplication.rb` script is the preferred method on Linux as there are no IDE project files provided.

#### Building the SDK

To build the SDK follow the steps above, then do the following:

On Windows:
- Install [Doxygen](http://www.stack.nl/~dimitri/doxygen/download.html).
- Install [NSIS](http://nsis.sourceforge.net/), version 3.0 or later is required.

On Mac OS X:
- Install [Doxygen](http://www.stack.nl/~dimitri/doxygen) using Homebrew: `brew install doxygen`

Then build the SDK by running:

    ruby SDK/BuildSDK.rb

#### Installing on Linux

There is no SDK package for Linux, however the engine's shared library and headers can be be installed by running:

    scons install

The default install location is `/usr/local`, however this can be overridden:

    scons install prefix=/install/path
