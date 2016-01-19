#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

require 'tmpdir'

# Subclass of SDKBuilderBase that creates the Mac OS X SDK package.
class MacOSXSDKBuilder < SDKBuilderBase
  BUILD_LOCATION = '/tmp/CarbonSDK'.freeze

  def initialize
    super

    @package_root = Dir.mktmpdir

    FileUtils.rm_f BUILD_LOCATION
    File.symlink REPOSITORY_ROOT, BUILD_LOCATION
    Dir.chdir BUILD_LOCATION
  end

  def create_sdk
    build_targets
    copy_headers
    move_documentation
    copy_scripts
    create_xcode_project_template
    sample_applications.each { |sample| create_sample_application sample }
    copy_sample_applications_xcode_project
    copy_licenses
    create_final_sdk_package
  end

  def cleanup
    super

    restore_working_directory

    FileUtils.rm BUILD_LOCATION
    FileUtils.rm_r @package_root
  end

  def build_targets
    [:MacOSX, :iOS].each { |target_platform| build_dependencies target_platform: target_platform }

    scons targets: [], arguments: scons_arguments
    scons targets: :CarbonEngine, arguments: scons_arguments(type: :Debug)

    [:Debug, :Release].product(ios_architectures.keys).each do |build_type, architecture|
      scons arguments: scons_arguments(platform: :iOS, type: build_type, architecture: architecture)
    end

    [:Debug, :Release].each do |build_type|
      create_static_library_for_macosx build_type
      create_static_library_for_ios build_type
    end
  end

  def copy_headers
    public_headers.each { |header, output_name| cp header, "#{@package_root}/Include/#{output_name}" }
  end

  def engine_library_name(build_type, prefix = '')
    "libCarbonEngine#{prefix}#{{ Debug: 'Debug' }[build_type]}.a"
  end

  def engine_library(platform, architecture, build_type)
    "Build/#{platform}/#{architecture}/#{compiler}/#{build_type}/#{engine_library_name build_type}"
  end

  def dependency_libraries(platform, architecture = nil)
    CARBON_DEPENDENCIES.map do |dependency, version|
      "Dependencies/#{dependency}-#{version}/Library/#{platform}/#{architecture}/lib#{dependency}.a"
    end
  end

  def create_static_library_for_macosx(build_type)
    inputs = [engine_library(:MacOSX, :x64, build_type), *dependency_libraries(:MacOSX, :x64)]

    merge_static_libraries inputs, "#{@package_root}/Library/#{engine_library_name build_type}"
  end

  def create_static_library_for_ios(build_type)
    inputs = ios_architectures.keys.map { |arch| engine_library :iOS, arch, build_type } + dependency_libraries(:iOS)

    merge_static_libraries inputs, "#{@package_root}/Library/#{engine_library_name build_type, 'iOS'}", sdk: :iphoneos
  end

  def move_documentation
    FileUtils.mv 'Documentation/APIReference', "#{@package_root}/Documentation"
  end

  def copy_scripts
    public_scripts.each { |script, output_name| cp script, "#{@package_root}/Scripts/#{output_name}" }
  end

  def create_xcode_project_template
    %w(Info.plist Info-iOS.plist TemplateInfo.plist TemplateIcon.icns).each do |file|
      cp "SDK/ProjectTemplates/Xcode/#{file}", "#{@package_root}/ProjectTemplate/#{file}"
    end

    create_template_files class_name: '___PACKAGENAMEASIDENTIFIER___',
                          target_without_extension: "#{@package_root}/ProjectTemplate/___PACKAGENAME___"
  end

  def create_sample_application(sample)
    create_installer application: sample, executable: "Build/MacOSX/x64/#{compiler}/Release/#{sample}", create_dmg: false,
                     assets: 'Assets/Samples', icon: 'Source/CarbonEngine/Carbon.icns', output_path: "#{@package_root}/Samples"

    # Include the sample application source code as well
    ["#{sample}.cpp", "#{sample}.h", 'Info.plist', 'Info-iOS.plist'].each do |file|
      cp "Source/#{sample}/#{file}", "#{@package_root}/Samples/Source/#{sample}/#{file}"
    end
  end

  def copy_sample_applications_xcode_project
    cp 'SDK/MacOSX/CarbonSamples.xcodeproj/project.pbxproj',
       "#{@package_root}/Samples/Source/CarbonSamples.xcodeproj/project.pbxproj"

    FileUtils.cp_r 'Assets/Samples', "#{@package_root}/Samples/Source/Assets"
  end

  def copy_licenses
    dependency_licenses { |license, dependency| cp license, "#{@package_root}/Licenses/#{dependency}#{File.basename license}" }
  end

  def sdk_filename
    "SDK/Carbon SDK #{sdk_version}.pkg"
  end

  def create_final_sdk_package
    command = "xcrun pkgbuild --quiet --root #{@package_root} --identifier com.carbon.CarbonSDK --scripts SDK/MacOSX/Scripts " \
              "--install-location \"/Applications/Carbon SDK\" #{sdk_filename.quoted}"

    run command, echo: 'Creating SDK package ...', error: 'Failed creating SDK package'
  end
end
