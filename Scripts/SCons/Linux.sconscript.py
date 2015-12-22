#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

import os

Import('*')

vars = Variables()
vars.AddVariables(
    ('gccversion', 'Sets the specific GCC version to use when building, this is suffixed onto \'g++-\'.'),
)
Help(vars.GenerateHelpText(Environment()))

env = SConscript('Compilers/GCC.sconscript.py')
env['ENV']['PATH'] = os.environ['PATH']

architecture = os.popen('uname -m').readlines()[0].strip(' \n\t')

# Append a specific GCC version to the invocations if one was specified
if 'gccversion' in ARGUMENTS:
    env['CC'] += '-' + ARGUMENTS['gccversion']
    env['CXX'] += '-' + ARGUMENTS['gccversion']


# The SetupForLinkingCarbon() method sets up the environment for linking Carbon as a dynamic library or linking Carbon as a
# static library into a final application
def SetupForLinkingCarbon(self, **keywords):
    defaultDependencies = ['AngelScript', 'Bullet', 'FreeImage', 'FreeType', 'OpenAssetImport', 'Vorbis', 'ZLib']

    dependencies = keywords.get('dependencies', defaultDependencies)

    self['LIBPATH'] += GetDependencyLIBPATH(*dependencies)
    self['LIBS'] += ['dl', 'GL', 'openal', 'pthread', 'SDL2', 'udev', 'X11', 'Xinerama'] + dependencies

env.AddMethod(SetupForLinkingCarbon)


# Add a method for setting up an environment ready for building against the installed SDK
def Carbonize(self, **keywords):
    if IsCarbonEngineStatic():
        self['CPPDEFINES'] += ['CARBON_STATIC_LIBRARY']

    self['LIBS'] += ['CarbonEngine' + {'Debug': 'Debug', 'Release': ''}[buildType]]

    if 'carbonroot' in ARGUMENTS:
        self['CPPPATH'] += [os.path.join(ARGUMENTS['carbonroot'], 'Source')]
        self['LIBPATH'] += [os.path.join(ARGUMENTS['carbonroot'], 'Build', 'Linux', architecture, 'GCC', buildType)]

        if IsCarbonEngineStatic():
            self.SetupForLinkingCarbon()
    else:
        # On Linux the headers and library files are expected to be found through the default include and library paths

        if IsCarbonEngineStatic():
            self.SetupForLinkingCarbon(dependencies=[])

    self.Append(**keywords)
    self.SetupPrecompiledHeader(keywords)

env.AddMethod(Carbonize)

# Return the build details
details = {'platform': 'Linux', 'architecture': architecture, 'compiler': 'GCC', 'env': env}
Return('details')
