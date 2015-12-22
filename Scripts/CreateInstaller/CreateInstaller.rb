#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

#
# This script provides a standard interface for Carbon applications to easily package themselves for redistribution on both the
# Windows and Mac OS X platforms. On Windows a .exe installer will be created and on Mac OS X a .dmg disk image will be created.
#
# Users of this script should access it through the CARBON_CREATE_INSTALLER_SCRIPT environment variable:
#
#       require ENV['CARBON_CREATE_INSTALLER_SCRIPT']
#
# The primary public method is create_installer() which takes a hash of options that describe the installer to create. The
# options that can be passed to create_installer() are:
#
#   :application    The displayable name of the application. Required.
#
#   :version        Version string to attach to the generated installer. Optional.
#
#   :executable     If the application is not configured to use the cross-platform build system then this must specify the
#                   application's already built executable file.
#
#   :build          If the application is configured to use the cross-platform build system then :build should be a hash where
#                   :root is the directory that contains the project's SConstruct file, and :target is the name of the SCons
#                   target for the application. When :build is specified the installer creation process will run a build of the
#                   project and then package the resulting executable automatically. :build can optionally contain an :arguments
#                   hash which can be used to specify SCons arguments when running the build. E.g.
#
#                     build: { root: '.', target: 'MyApplication', arguments: { architecture: :x64, strict: true } }
#
#   :icon           The installer icon to use. On Windows this is an ICO file and on Mac OS X it is an ICNS file. Optional.
#
#                     icon: 'MyApplication.ico',
#
#   :output_path    The directory in which to put the created installer. The specified directory will be created if it doesn't
#                   already exist. By default the created installer is placed in the working directory.
#
#   :assets         This specifies the application's assets to include in the installer, e.g. textures and meshes. It should be
#                   an array of hashes where each hash contains a :root key which is the path to a base asset directory (often
#                   'Assets/'), and :items is an array describing the files and directories underneath the :root which should be
#                   included. Entries in the :items array are all relative to :root.
#
#                   Each item can be the name of a file to include, the name of a directory to include all the files in, or a
#                   hash where :root is a subdirectory of interest, :patterns holds one or more file pattern matches such as
#                   '*.jpg' or '*.txt', and :recursive specifies whether to search recursively for files (defaults to true).
#
#                   If :items is omitted then everything under the specified :root will be included, and in that situation the
#                   root can be specified as a string directly instead of writing { root: '...' }.
#
#                   Files and folders starting with a period character will always be ignored.
#
#                     assets: [{
#                       root: 'Assets',
#                       items: [
#                         'MyFile.txt',                                             # Just this single text file
#                         'Animations',                                             # All files under the Animations/
#                         { name: 'Sounds', patterns: '*.wav', recursive: false },  # Only .wav files directly under Sounds/
#                         { name: 'Textures', patterns: %w(*.dds *.jpg) }           # All .dds and .jpg files under Textures/
#                       ]
#                     }]
#
#                     assets: 'Assets'                # All files under Assets/
#
#                     assets: ['Assets', 'Assets2']   # All files under Assets/ and Assets2/
#

THIS_DIRECTORY = File.expand_path File.dirname(__FILE__)

require 'erb'
require 'find'
require 'tmpdir'

require "#{THIS_DIRECTORY}/../Shared.rb"
require "#{THIS_DIRECTORY}/../NSIS.rb"
require "#{THIS_DIRECTORY}/../SCons.rb"

# This class contains shared functionality for creating installers, it is subclassed separately for Windows and Mac OS X
class InstallerCreatorBase
  attr_accessor :application, :version, :executable, :build, :assets, :icon, :output_path

  def create
    validate_attributes

    FileUtils.mkpath output_path if output_path

    self.executable ||= build_application

    error "Application executable '#{executable}' does not exist" unless File.exist? executable

    create_installer
  end

  protected

  def display_name
    [application, version].compact.join ' '
  end

  def full_output_path(file)
    output_path ? File.join(output_path, file) : file
  end

  def asset_files
    Array(assets).each_with_object({}) do |asset_entry, result_hash|
      root = asset_entry.is_a?(String) ? asset_entry : asset_entry.fetch(:root)

      items = asset_entry[:items] if asset_entry.is_a? Hash
      items ||= all_entries_in_directory root

      items.each do |item|
        result_hash.merge! AssetEntry.new(item, root).files
      end
    end
  end

  def read_local_file(name)
    File.read File.join(THIS_DIRECTORY, name)
  end

  private

  def validate_attributes
    error 'An application name must be specified' unless application
    error 'Build information must be specified' unless build || executable
    error "Icon does not exist: #{icon}" unless icon.nil? || File.exist?(icon)
  end

  def build_application
    error 'Build configuration is missing' unless build

    build[:arguments] ||= {}
    build[:arguments][:static] = true if macosx?

    SCons.scons(scons_options).first
  end

  def scons_options
    {
      echo: true,
      verbose: build.fetch(:verbose, false),
      arguments: SCons.merge_default_arguments(build[:arguments]),
      targets: build.fetch(:target),
      working_directory: build.fetch(:working_directory),
      jobs: build.fetch(:jobs, cpu_count)
    }
  end

  def all_entries_in_directory(directory)
    Dir[File.join directory, '*'].map do |entry|
      entry[(directory.length + 1)..-1]
    end
  end
end

# This class takes a single (root, item) entry from the 'assets' attribute of InstallerCreatorBase and works out which file(s)
# should be included based on what it specifies.
class AssetEntry
  def initialize(input, root)
    input = { name: input } if input.is_a? String

    @root = root
    @name = input.fetch :name
    @recursive = input.fetch :recursive, true
    @patterns = Array(input.fetch :patterns, '*')
  end

  def files
    if File.file? full_name
      { full_name => "Assets/#{@name}" }
    elsif File.directory? full_name
      assets_in_directory.each_with_object({}) do |file, hash|
        hash[file] = "Assets/#{without_root file}"
      end
    end
  end

  private

  def full_name
    File.join @root, @name
  end

  def without_root(file)
    file[(@root.length + 1)..-1]
  end

  def assets_in_directory
    result = []

    Find.find full_name do |file|
      if File.basename(file)[0] == '.'
        Find.prune
      else
        result << file if valid_asset?(file)
      end
    end

    result
  end

  def valid_asset?(candidate_file)
    File.exist?(candidate_file) &&
      !File.directory?(candidate_file) &&
      @patterns.any? { |pattern| File.fnmatch? pattern, File.basename(candidate_file) } &&
      (@recursive || File.dirname(candidate_file) == full_name)
  end
end

# Class for creating installers for Windows
class WindowsInstallerCreator < InstallerCreatorBase
  private

  def create_installer
    NSIS.nsis_with_script_content ERB.new(read_local_file 'Installer.nsi.erb').result(binding)

    "#{display_name}.exe"
  end

  def sdk_path
    ENV.fetch 'CARBON_SDK_PATH'
  end

  def scons_options
    super.tap do |options|
      options[:arguments][:static] = false
    end
  end

  def architecture
    scons_options[:arguments].fetch :architecture
  end
end

# Class for creating installers for Mac OS X
class MacOSXInstallerCreator < InstallerCreatorBase
  attr_accessor :create_dmg

  def initialize
    self.create_dmg = true
  end

  private

  def dmg_name
    full_output_path "#{display_name}.dmg"
  end

  def create_installer
    app_bundle_create

    create_dmg_from_app_bundle if create_dmg

    dmg_name
  end

  def app_bundle_create
    puts "Creating #{File.basename app_bundle_path} ..."

    app_bundle_clean
    app_bundle_copy_executable
    app_bundle_copy_icon
    app_bundle_create_info_plist
    app_bundle_copy_assets
  end

  def app_bundle_path
    full_output_path "#{application}.app"
  end

  def app_bundle_clean
    FileUtils.rm_rf app_bundle_path
    FileUtils.mkpath "#{app_bundle_path}/Contents/MacOS"
    FileUtils.mkpath "#{app_bundle_path}/Contents/Resources"
  end

  def app_bundle_copy_executable
    FileUtils.cp executable, "#{app_bundle_path}/Contents/MacOS/#{application}"
  end

  def app_bundle_copy_icon
    FileUtils.cp icon, "#{app_bundle_path}/Contents/Resources" if icon
  end

  def app_bundle_create_info_plist
    File.write "#{app_bundle_path}/Contents/Info.plist", ERB.new(read_local_file 'Info.plist.erb').result(binding)
  end

  def app_bundle_copy_assets
    asset_files.each do |source, destination|
      target = "#{app_bundle_path}/Contents/Resources/#{destination}"

      FileUtils.mkpath File.dirname(target)

      run "cp #{source.quoted} #{target.quoted}", echo: false, error: 'File copy failed'
    end
  end

  def create_dmg_from_app_bundle
    puts "Creating #{File.basename dmg_name} ..."

    FileUtils.rm_f dmg_name

    Dir.mktmpdir do |dir|
      FileUtils.mv app_bundle_path, dir
      File.symlink '/Applications', "#{dir}/Applications"

      create_macosx_dmg source_folder: dir, target: dmg_name, volume_name: application
    end
  end
end

def create_installer(options)
  if windows?
    klass = WindowsInstallerCreator
  elsif macosx?
    klass = MacOSXInstallerCreator
  else
    error 'Not supported on this platform'
  end

  instance = klass.new

  options.each { |key, value| instance.send "#{key}=", value }

  instance.create
end
