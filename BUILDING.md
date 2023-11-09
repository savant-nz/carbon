# Building Carbon

On Windows:
- Install [Visual Studio Community 2022](https://visualstudio.com).
    - Ensure that Visual C++ is selected in the installer.
- Install [Python 3.6 or later](http://www.python.org/downloads).
    - Ensure that '*Add Python to environment variables*' is ticked when installing.
- Install [PyWin32](https://pypi.org/project/pywin32/#files).
- Install [SCons](http://www.scons.org/) by running `pip install scons`.
- Install [Ruby](http://rubyinstaller.org/downloads), version 3.0 or later is required.
- Install [Git](https://git-scm.com/download/win).

On macOS:
- Install [Xcode](https://itunes.apple.com/app/xcode/id497799835) through the Mac App Store.
- Install [Homebrew](http://brew.sh).
- Install [SCons](http://www.scons.org) using Homebrew: `brew install scons`

On Ubuntu Linux 22.04 and later:
- Install required packages using the following command: `sudo apt install g++ git libopenal-dev libgl1-mesa-dev
  libsdl2-dev libudev-dev libxinerama-dev ruby scons`

Once this setup is complete build the engine's dependencies:

    ruby Dependencies/BuildDependencies.rb

If you are using macOS and want to develop or test on iOS then build the dependencies for that platform also:

    ruby Dependencies/BuildDependencies.rb --platform iOS

After the dependencies have been built open the Visual Studio or Xcode project file in the `Source/` directory and build
and run the sample applications.

Alternatively, build and run sample applications from the command line:

    ruby Scripts/RunApplication.rb BoxesSample

Using the `RunApplication.rb` script is the preferred method on Linux as there are no IDE project files provided.

#### Building the SDK

To build the SDK follow the steps above, then do the following:

On Windows:
- Install [Doxygen](https://www.doxygen.nl/download.html).
- Install [NSIS](http://nsis.sourceforge.net/), version 3.0 or later is required.

On macOS:
- Install [Doxygen](https://www.doxygen.nl/) using Homebrew: `brew install doxygen`

Then build the SDK by running:

    ruby SDK/BuildSDK.rb

#### Installing on Linux

There is no SDK package for Linux, however the engine's shared library and headers can be be installed by running:

    scons install

The default install location is `/usr/local`, however this can be overridden:

    scons install prefix=/install/path
