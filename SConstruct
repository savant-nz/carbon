#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

SConscript('Scripts/SCons/Shared.sconscript.py')
SConscript('Source/SConscript')


vars = Variables()
vars.AddVariables(
    ('externalapp', 'The name of an external application to include in with the main engine build. This allows '
                    'external apps to hook into this build system for debugging and development without having to set '
                    'up their own build script. The externalappsourceroot= argument must specify the relative path to '
                    'search in for the app\'s source code.'),

    ('externalappsourceroot', 'The relative path to the directory that holds the external application\'s source code. '
                              'Should be relative to this SConstruct file, e.g. ../MyApplication')
)
Help(vars.GenerateHelpText(Environment()))


if 'externalapp' in ARGUMENTS:
    import os
    Import('*')

    externalAppName = ARGUMENTS['externalapp']
    externalAppSourceRoot = ARGUMENTS['externalappsourceroot']

    # Setup a variant dir so that build outputs don't get put with the app's code (only works if the source root is
    # relative)
    variantDir = os.path.join(baseVariantDir, externalAppName)
    VariantDir(variantDir, os.path.join('#', externalAppSourceRoot), duplicate=0)

    # Enumerate source files recursively
    sourceFiles = []
    for dirpath, dirnames, filenames in os.walk(externalAppSourceRoot):
        dirpath = dirpath[len(externalAppSourceRoot) + 1:]
        filenames = [f for f in filenames if f.endswith('.cpp') or f.endswith('.mm')]
        sourceFiles += [os.path.join(variantDir, dirpath, f) for f in filenames]
    if sourceFiles == []:
        print('Error: external application has no source files, check its source root "%s" is correct.'
              % externalAppSourceRoot)
        Exit(1)

    # Build and install the app
    result = baseEnv.Clone().CarbonClientProgram(os.path.join(variantDir, externalAppName + 'External'), sourceFiles)
    Alias(externalAppName, InstallAs(os.path.join(installDir, externalAppName + baseEnv['PROGSUFFIX']), result[0]))
