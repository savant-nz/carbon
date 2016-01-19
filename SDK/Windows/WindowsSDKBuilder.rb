#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

require 'rexml/document'

# Subclass of SDKBuilderBase that creates the Windows SDK package.
class WindowsSDKBuilder < SDKBuilderBase
  attr_accessor :fast_build

  def initialize
    super

    create_subst_drive
  end

  def create_sdk
    build_targets
    create_visual_studio_project_template
    create_sample_application_visual_studio_project_files
    create_sdk_installer
  end

  def cleanup
    super

    delete_sample_application_visual_studio_project_files
    restore_working_directory
    remove_subst_drive
  end

  def subst_drive
    'P:'
  end

  def create_subst_drive
    remove_subst_drive

    run "subst #{subst_drive} #{REPOSITORY_ROOT}", echo: false, error: 'Failed mapping drive with subst'

    Dir.chdir subst_drive
  end

  def remove_subst_drive
    run("subst #{subst_drive} /D", echo: false) {}
  end

  def scons_arguments(options = {})
    { static: false }.merge super
  end

  def scons_x64(options)
    options[:arguments][:architecture] = :x64
    scons options
  end

  def build_targets
    build_dependencies

    # x86 builds
    scons targets: [], arguments: scons_arguments
    scons targets: :CarbonEngine, arguments: scons_arguments(type: :Debug)

    # x64 builds
    scons_x64 targets: [:CarbonEngine, :CarbonExporterMaya2009, :CarbonExporterMaya2014], arguments: scons_arguments
    scons_x64 targets: :CarbonEngine, arguments: scons_arguments(type: :Debug)
  end

  def create_visual_studio_project_template
    create_template_files class_name: '[!output SAFE_PROJECT_NAME]', header_name: '[!output PROJECT_NAME].h',
                          target_without_extension: 'SDK/ProjectTemplates/VisualStudio/Templates/1033/Application'
  end

  # Creates the Visual Studio project files for the sample applications. This is done by taking the XML project files for the
  # samples in Source/ and making a few chnages so that they build against the SDK rather than the local repository. See the
  # SampleApplicationProjectFile class below for details
  def create_sample_application_visual_studio_project_files
    @sample_application_visual_studio_project_files = sample_applications.map do |sample|
      project_file = SampleApplicationProjectFile.new "Source/#{sample}/#{sample}.vcxproj"

      project_file.additional_include_directories = '$(CARBON_SDK_PATH)\Include'
      project_file.additional_library_directories = '$(CARBON_SDK_PATH)\Library'
      project_file.remove_project_references

      project_file.write "SDK/Windows/#{sample}.vcxproj"
    end
  end

  def delete_sample_application_visual_studio_project_files
    FileUtils.rm_f @sample_application_visual_studio_project_files if @sample_application_visual_studio_project_files
  end

  # This class takes an XML Visual Studio project file and exposes methods to alter parts of it and then write it out as a new
  # project file.
  class SampleApplicationProjectFile
    def initialize(filename)
      @document = REXML::Document.new File.read(filename)
    end

    def additional_include_directories=(directories)
      @document.elements.each 'Project/ItemDefinitionGroup/ClCompile/AdditionalIncludeDirectories' do |element|
        element.text = directories
      end
    end

    def additional_library_directories=(directories)
      @document.elements.each 'Project/ItemDefinitionGroup/Link' do |element|
        libdir_element = element.elements['AdditionalLibraryDirectories']
        libdir_element ||= (element.elements << REXML::Element.new('AdditionalLibraryDirectories'))

        libdir_element.text = directories
      end
    end

    def remove_project_references
      @document.elements.each('Project/ItemGroup/ProjectReference', &:remove)
    end

    def write(filename)
      File.write filename, @document.to_s

      filename
    end
  end

  def sdk_filename
    "SDK/Carbon SDK #{sdk_version}.exe"
  end

  def create_sdk_installer
    NSIS.nsis_with_script_content ERB.new(File.read('SDK/Windows/SDK.nsi.erb')).result(binding)
  end
end
