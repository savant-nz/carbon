/*
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
 * distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

function OnFinish() {
  try {
    var projectPath = wizard.FindSymbol('PROJECT_PATH');
    var projectName = wizard.FindSymbol('PROJECT_NAME');

    // Check that the target drive is usable (e.g. not a network drive)
    if (!CanUseDrive(projectPath)) {
      return VS_E_WIZARDBACKBUTTONPRESS;
    }

    // Create a safe project name and add it as a symbol. The name 'CarbonApplication' is avoided because it causes errors.
    if (projectName === 'CarbonApplication') {
      projectName = 'MyCarbonApplication';
    }
    wizard.AddSymbol('SAFE_PROJECT_NAME', CreateSafeName(projectName));

    // Create the project
    var project = CreateProject(projectName, projectPath);

    // Setup the build configurations
    setupConfiguration(project.Object.Configurations('Debug'), 'Debug', 'x86');
    setupConfiguration(project.Object.Configurations('Debug|x64'), 'Debug', 'x64');
    setupConfiguration(project.Object.Configurations('Release'), 'Release', 'x86');
    setupConfiguration(project.Object.Configurations('Release|x64'), 'Release', 'x64');

    // Add files to the project based on the contents of Templates.inf
    AddFilesToProjectWithInfFile(project, projectName);

    project.Object.Save();
  } catch (e) {
    if (e.description.length !== 0) {
      SetErrorInfo(e);
    }

    return e.number;
  }
}

function SetFileProperties() {
}

function GetTargetName(templateName, projectName) {
  if (templateName === 'Application.h') {
    return projectName + '.h';
  } else if (templateName === 'Application.cpp') {
    return projectName + '.cpp';
  } else {
    return templateName;
  }
}

function setupConfiguration(config, buildType, architecture) {
  if (!config) {
    return;
  }

  var compiler = config.Tools('VCCLCompilerTool');
  var linker = config.Tools('VCLinkerTool');

  compiler.AdditionalIncludeDirectories = '$(CARBON_SDK_PATH)\\Include';
  compiler.WarningLevel = warningLevel_4;
  compiler.PreprocessorDefinitions = '_WINDOWS';

  linker.AdditionalLibraryDirectories = '$(CARBON_SDK_PATH)\\Library';
  linker.SubSystem = subSystemWindows;

  if (buildType == 'Debug') {
    compiler.DebugInformationFormat = debugEditAndContinue;
    compiler.Optimization = optimizeDisabled;
    compiler.PreprocessorDefinitions += ';_DEBUG';
    compiler.MinimalRebuild = true;
    compiler.BasicRuntimeChecks = runtimeBasicCheckAll;
    compiler.RuntimeLibrary = rtMultiThreadedDebug;

    linker.LinkIncremental = linkIncrementalYes;
    linker.GenerateDebugInformation = true;

  } else if (buildType == 'Release') {

    compiler.DebugInformationFormat = debugEnabled;
    compiler.PreprocessorDefinitions += ';NDEBUG';
    compiler.RuntimeLibrary = rtMultiThreaded;

    linker.OptimizeReferences = optReferences;
    linker.EnableCOMDATFolding = optFolding;

    if (architecture == 'x86') {
      compiler.EnableEnhancedInstructionSet = enhancedInstructionSetTypeSIMD2;
    }
  }

  if (architecture == 'x86') {
    compiler.PreprocessorDefinitions += ';WIN32';
  }
}
