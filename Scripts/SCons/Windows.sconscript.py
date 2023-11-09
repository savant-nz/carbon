#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

import os

Import('*')

# Add descriptions for the Windows-specific build system options
vars = Variables()
vars.AddVariables(
    ('architecture', 'Sets the target build architecture, must be either x86 or x64.', 'x86'),
    ('slim', '(true/false) Reduce code size as much as possible, currently this only affects debug builds.', 'false')
)
Help(vars.GenerateHelpText(Environment()))

# Set the compiler and architecture being targeted by this build
compiler = 'VisualStudio2022'
architecture = ARGUMENTS.get('architecture', 'x86')
if architecture not in ['x86', 'x64']:
    print('Error: invalid architecture specified')
    Exit(1)

env = SConscript('Compilers/VisualStudio.sconscript.py', exports='architecture compiler')

# Alter flags for slim builds
if buildType == 'Debug' and ARGUMENTS.get('slim', 'false') == 'true':
    env['CCFLAGS'].remove('/Od')
    env['CCFLAGS'].remove('/RTC1')
    env['CCFLAGS'] += ['/O1']


# This method sets up the environment for linking Carbon as a dynamic library or linking Carbon as a static library into
# a final application
def SetupForLinkingCarbon(self):
    self['LIBPATH'] += GetDependencyLIBPATH('AngelScript', 'Bullet', 'FreeImage', 'FreeType', 'OpenALSoft',
                                            'OpenAssetImport', 'OculusRift', 'Vorbis', 'ZLib')

env.AddMethod(SetupForLinkingCarbon)


# Add a method for setting up an environment ready for building against the installed SDK
def Carbonize(self, **keywords):
    if 'carbonroot' in ARGUMENTS:
        self['CPPPATH'] += [os.path.join(ARGUMENTS['carbonroot'], 'Source')]
        self['LIBPATH'] += [os.path.join(ARGUMENTS['carbonroot'], 'Build/Windows', architecture, compiler, buildType)]
    else:
        if 'CARBON_SDK_PATH' not in os.environ:
            print('Error: there is no Carbon SDK installed')
            Exit(1)

        self['CPPPATH'] += [os.path.join(os.environ['CARBON_SDK_PATH'], 'Include')]
        self['LIBPATH'] += [os.path.join(os.environ['CARBON_SDK_PATH'], 'Library')]

    if self.IsCarbonEngineStatic():
        self['CPPDEFINES'] += ['CARBON_STATIC_LIBRARY']
        self.SetupForLinkingCarbon()

        if 'carbonroot' not in ARGUMENTS:
            print('Error: using a static library build of Carbon on Windows requires that carbonroot be specified')
            Exit(1)

    self['LINKFLAGS'] += ['/SUBSYSTEM:WINDOWS']
    self.Append(**keywords)
    self.SetupPrecompiledHeader(keywords)

env.AddMethod(Carbonize)

# Return all the build setup details for this platform
details = {'platform': 'Windows', 'architecture': architecture, 'compiler': compiler, 'env': env}
Return('details')
