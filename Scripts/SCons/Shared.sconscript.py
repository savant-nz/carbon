#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

import os
import sys
import SCons

# Add information about global build system command line options that are available
vars = Variables()
vars.AddVariables(
    ('type', 'Sets the build type, must be Debug or Release. Debug builds include additional debugging code, symbols and do '
             'not optimize the resulting executables.', 'Release'),
    ('strict', '(true/false) Whether a strict build should be done, this maximizes warnings and strictness checking and also '
               'treats build warnings as errors.', 'false'),
    ('platform', 'Overrides the default build platform, when this is set the build system will search for an appropriate build '
                 'script that supports building for the requested platform. This overrides the automatic platform detection.'),
    ('platformscript', 'Specifies the platform build to use for this build, this will be used instead of doing a search for an '
                       'appropriate platform script. This overrides the platform= argument.'),
    ('carbonroot', 'When building client applications this specifies the path to the Carbon repository to build against '
                   'instead of building against the installed SDK. Note that this does not cause the specified Carbon '
                   'repository itself to be rebuilt.'),
    ('static', '(true/false) Whether to build/use CarbonEngine as a static library instead of a dynamic library. Some '
               'platforms do not support building as a dynamic library.')
)
Help(vars.GenerateHelpText(Environment()))

# Parse command line arguments
buildType = ARGUMENTS.get('type', 'Release')
if buildType not in ['Debug', 'Release']:
    print('Error: the build type must either Debug or Release')
    Exit(1)
isDebugBuild = (buildType == 'Debug')
isReleaseBuild = (buildType == 'Release')
isStrictBuild = (ARGUMENTS.get('strict', 'false') == 'true')
Export('buildType', 'isDebugBuild', 'isReleaseBuild', 'isStrictBuild')


# Returns the path to the Dependencies directory relative to the calling SConstruct file.
def GetDependenciesPath():
    candidates = ['#/Dependencies', '#/..']
    if 'carbonroot' in ARGUMENTS:
        candidates = [os.path.join(ARGUMENTS['carbonroot'], 'Dependencies')]

    for candidate in candidates:
        candidatePath = str(Dir(candidate).srcnode())
        if os.path.isdir(candidatePath) and os.path.basename(candidatePath) == 'Dependencies':
            return candidate

    print('Error: unable to locate the Dependencies directory')
    Exit(1)

Export('GetDependenciesPath')


# Returns the full path to the specified dependency with an optional array of subpaths as well. Optional keywords: version.
def GetDependencyPath(dependency, *paths, **keywords):
    defaultVersions = {
        'AngelScript': '2.30.2', 'Bullet': '2.83', 'FreeImage': '3.17.0', 'FreeType': '2.6.1', 'Max': '', 'Maya': '',
        'OculusRift': '0.8', 'Ogg': '1.3.2', 'OpenALSoft': '1.16.0', 'OpenAssetImport': '3.1.1', 'PhysX': '3.3.2',
        'Vorbis': '1.3.5', 'ZLib': '1.2.8'
    }

    version = keywords.get('version')
    if not version:
        version = defaultVersions[dependency]

    return os.path.join(GetDependenciesPath(), dependency + '-' + version, *paths)

Export('GetDependencyPath')


# Returns the list of CPPDEFINES entries needed to include the specified dependencies in a build
def GetDependencyCPPDEFINES(*dependencies):
    return ['CARBON_INCLUDE_' + d.upper() for d in dependencies]

Export('GetDependencyCPPDEFINES')


# Returns the list of CPPPATH entries needed to include the specified dependencies in a build
def GetDependencyCPPPATH(*dependencies, **keywords):
    includePaths = {
        'AngelScript': [GetDependencyPath('AngelScript', 'Source', 'include', **keywords)],
        'Bullet': [GetDependencyPath('Bullet', 'Source', **keywords)],
        'FreeImage': [GetDependencyPath('FreeImage', 'Source', **keywords)],
        'FreeType': [GetDependencyPath('FreeType', 'Include', **keywords)],
        'Max': [GetDependencyPath('Max', 'Include', **keywords)],
        'Maya': [GetDependencyPath('Maya', 'Include', **keywords)],
        'OpenALSoft': [GetDependencyPath('OpenALSoft', 'Source', 'include', **keywords)],
        'OpenAssetImport': [GetDependencyPath('OpenAssetImport', 'Source', 'include', **keywords)],
        'OculusRift': [GetDependencyPath('OculusRift', 'Source/LibOVR/Include', **keywords)],
        'PhysX': [GetDependencyPath('PhysX', 'Include', **keywords)],
        'Vorbis': [GetDependencyPath('Ogg', 'Include', **keywords), GetDependencyPath('Vorbis', 'Include', **keywords)],
        'ZLib': [GetDependencyPath('ZLib', 'Source', **keywords)]
    }

    return [includePaths[d] for d in dependencies]

Export('GetDependencyCPPPATH')


# Returns the list of LIBPATH entries needed to include the specified dependencies in a build
def GetDependencyLIBPATH(*dependencies, **keywords):
    if platform == 'iOS':
        return [GetDependencyPath(d, 'Library', platform, **keywords) for d in dependencies]
    else:
        return [GetDependencyPath(d, 'Library', platform, architecture, **keywords) for d in dependencies]

Export('GetDependencyLIBPATH')


# Global accessors for the SCons object corresponding to the main engine library created by the engine build
def GetCarbonEngineBuildResult():
    global carbonEngineBuildResult
    return carbonEngineBuildResult


def SetCarbonEngineBuildResult(buildResult):
    global carbonEngineBuildResult
    if platform == 'Windows':
        carbonEngineBuildResult = next(x for x in buildResult if str(x).endswith('.lib'))
    else:
        carbonEngineBuildResult = buildResult[0]

Export('GetCarbonEngineBuildResult', 'SetCarbonEngineBuildResult')


# A platform script is responsible for providing a base SCons.Environment configured for building for a specific platform. The
# platform script to use is automatically detected unless a platform= or platformscript= argument is provided. Setting platform=
# triggers a search for an appropriate build script for the specified build platform. The platform script to use can also be set
# explicitly using the platformscript= argument.
platformScriptFile = None
if 'platformscript' in ARGUMENTS:
    platformScriptFile = ARGUMENTS['platformscript']
elif 'platform' in ARGUMENTS:
    platformScriptFile = ARGUMENTS['platform'] + '.sconscript.py'
elif os.name == 'nt':
    platformScriptFile = 'Windows.sconscript.py'
elif sys.platform == 'darwin':
    platformScriptFile = 'MacOSX.sconscript.py'
elif sys.platform == 'linux2':
    platformScriptFile = 'Linux.sconscript.py'

# Check the platform script exists
if not platformScriptFile or not os.path.exists(platformScriptFile):
    print('Unable to find a build script for the specified platform')
    Exit(1)

# Invoke the platform script
platformScript = SConscript(platformScriptFile)

# Retrieve and export the build details returned by the platform script
platform = platformScript['platform']
architecture = platformScript['architecture']
compiler = platformScript['compiler']
Export('platformScript', 'platform', 'architecture', 'compiler')

# Retrieve the base build environment so it can be extended
baseEnv = platformScript['env']
baseEnv.Append(CPPDEFINES=[], CPPPATH=[], LIBPATH=[], LIBS=[])


# Add a method for detecting 64-bit platforms
def Is64Bit():
    return architecture.find('64') != -1

Export('Is64Bit')


# Add a method for determining whether the main engine library is being linked statically. The default is to link statically
# except when running `scons install` on a POSIX platform.
def IsCarbonEngineStatic():
    if (platform == 'Linux' or platform == 'MacOSX') and 'install' in COMMAND_LINE_TARGETS:
        if ARGUMENTS.get('static', '') == 'true':
            print('Error: the \'install\' target should not be used when building with static=true')
            Exit(1)

        return False

    return platformScript.get('isCarbonEngineStatic', ARGUMENTS.get('static', 'true') == 'true')

Export('IsCarbonEngineStatic')


# Add a method that returns the name for the main engine library
carbonEngineLibraryName = 'CarbonEngine'
if isDebugBuild:
    carbonEngineLibraryName += 'Debug'
if platform == 'Windows' and Is64Bit():
    carbonEngineLibraryName += '64'

Export('carbonEngineLibraryName')


# GlobDirectories() takes a list of directories and returns a list of all source files found inside them that are relevant to
# the current platform. Optional keywords are 'recursive' (which defaults to False).
def GlobDirectories(self, *directories, **keywords):
    sourceFileExtensions = ['c', 'cpp', 'cc', 'cxx']
    if platform == 'MacOSX' or platform == 'iOS':
        sourceFileExtensions += ['mm']

    # Convert to array
    directories = [d for d in directories]

    # Expand the directory list if recursion is turned on
    if keywords.get('recursive'):
        basePathLength = len(str(Dir('.').srcnode())) + 1
        for directory in directories[:]:
            for dirpath, dirnames, filenames in os.walk(str(Dir(directory).srcnode())):
                directories += [os.path.join(dirpath, d)[basePathLength:] for d in dirnames]

    # Gather source files from all the directories
    files = []
    for directory in sorted(set(directories)):
        for extension in sourceFileExtensions:
            files += Glob(os.path.join(directory, '*.' + extension))

        # Include resource files when building for Windows
        if platform == 'Windows':
            for rcFile in Glob(os.path.join(directory, '*.rc')):
                files += self.RES(rcFile)

    return sorted(files, key=str)

baseEnv.AddMethod(GlobDirectories)


# Takes a list of headers and returns all header files that they include, i.e. a full list of all dependent headers
def GetDependentHeaders(self, explicitIncludes, searchPaths, result=None):
    if result is None:
        result = set()

    for header in explicitIncludes:
        header = File(header)
        if header not in result:
            result.add(header)
            dependents = SCons.Tool.CScanner(header, self, searchPaths)
            self.GetDependentHeaders(dependents, searchPaths, result)

    return result

baseEnv.AddMethod(GetDependentHeaders)


# Add a ChMod action on POSIX platforms
if platform == 'Linux' or platform == 'MacOSX':
    ChMod = SCons.Action.ActionFactory(os.chmod, lambda dest, mode: 'ChMod("%s", 0%o)' % (dest, mode))
    Export('ChMod')


# Make baseEnv.UsePrecompiledHeader() a no-op if it isn't defined
if not hasattr(baseEnv, 'UsePrecompiledHeader'):
    def UsePrecompiledHeader(self, header, **keywords):
        pass
    baseEnv.AddMethod(UsePrecompiledHeader)


# This method allows the precompiled header configuration to be done based on PRECOMPILED_HEADER and PRECOMPILED_HEADER_OPTIONS
# keywords
def SetupPrecompiledHeader(self, keywords):
    if 'PRECOMPILED_HEADER' in keywords:
        self.UsePrecompiledHeader(keywords['PRECOMPILED_HEADER'], **keywords.get('PRECOMPILED_HEADER_OPTIONS', {}))

baseEnv.AddMethod(SetupPrecompiledHeader)


# Add builder method for building a program against the installed SDK. The platform script is expected to have provided a
# Carbonize() method on baseEnv for this purpose that sets up a build environment appropriately.
def CarbonProgram(self, program, sources, **keywords):
    e = self.Clone()
    e.Carbonize(**keywords)
    return e.Program(program, sources)

baseEnv.AddMethod(CarbonProgram)

# Export the base build environment
Export('baseEnv')

# Determine the variant and install directories
outputPrefix = os.path.join(*[item for item in [platform, architecture, compiler, buildType] if item != ''])
baseVariantDir = os.path.join('#/.scons', outputPrefix)
installDir = os.path.join('#/Build', outputPrefix)
Export('baseVariantDir', 'installDir')


# Helper method that takes a target name and a set of source files and builds a program from it using baseEnv.CarbonProgram().
# Install() is called automatically as well. U
def BuildCarbonProgram(target, sourceFiles):
    result = baseEnv.CarbonProgram(target, sourceFiles)
    Alias(target, Install(installDir, result))
    return result

Export('BuildCarbonProgram')


# Wrapper method around SConscript() that automatically sets duplicate=0 and variant_dir. If variant_dir is not specified then
# one is deduced based on the name or directory of the script file being run. The variant_dir is automatically prepended with
# baseVariantDir. This method is otherwise identical to vanilla SConscript()
def CarbonSConscript(*scripts, **keywords):
    if 'variant_dir' not in keywords:
        keywords['variant_dir'] = os.path.splitext(os.path.basename(scripts[0]))[0]
        if keywords['variant_dir'] == 'SConscript':
            keywords['variant_dir'] = os.path.basename(os.path.dirname(scripts[0]))
    keywords['variant_dir'] = os.path.join(baseVariantDir, keywords['variant_dir'])
    keywords['duplicate'] = 0
    return SConscript(*scripts, **keywords)

Export('CarbonSConscript')

Return('CarbonSConscript')
