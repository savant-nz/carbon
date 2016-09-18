#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# This file contains shared code and helper methods used by other scripts.

STDOUT.sync = true

require 'English'
require 'fileutils'
require 'rbconfig'

CARBON_SHARED_SCRIPT_START_TIME = Time.now unless defined? CARBON_SHARED_SCRIPT_START_TIME

CARBON_DEPENDENCIES = {
  AngelScript:     '2.31.1',
  Bullet:          '2.83.7',
  FreeImage:       '3.17.0',
  FreeType:        '2.6.5',
  OpenALSoft:      '1.17.2',
  OpenAssetImport: '3.2',
  OculusRift:      '0.8',
  PhysX:           '3.3.2',
  Vorbis:          '1.3.5',
  ZLib:            '1.2.8'
}.freeze unless defined? CARBON_DEPENDENCIES

# Returns whether the rbconfig identifier for this platform contains the passed identifier.
def host_os?(identifier)
  RbConfig::CONFIG.fetch('host_os').downcase.include? identifier.downcase
end

# Returns whether this script is running on Windows.
def windows?
  host_os?('mswin') || host_os?('mingw')
end

# Returns whether this script is running on Mac OS X.
def macosx?
  host_os? 'darwin'
end

# Returns whether this script is running on Linux.
def linux?
  host_os? 'linux'
end

# Returns the name of the current platform as a symbol, will return :Windows, :MacOSX, :Linux or nil.
def platform
  return :Windows if windows?
  return :MacOSX if macosx?
  return :Linux if linux?
  nil
end

# Returns an array of the supported platforms.
def supported_platforms
  [:Android, :iOS, :Linux, :MacOSX, :Windows]
end

# Returns the supported build architectures on iOS, the values indicate the corresponding toolchain architecture.
def ios_architectures
  { ARMv7: :armv7, ARM64: :arm64, x86: :i386, x64: :x86_64 }
end

# Returns a path inside the specified dependency.
def dependency_path(dependency, *paths)
  File.join 'Dependencies', "#{dependency}-#{CARBON_DEPENDENCIES.fetch(dependency)}", *paths
end

# Formats the given number of seconds in the format 'm:ss'.
def format_time(seconds)
  "#{seconds.to_i / 60}:#{(seconds.to_i % 60).to_s.rjust(2, '0')}"
end

# Returns the amount of time elasped since this script was run.
def elapsed_script_time
  Time.now - CARBON_SHARED_SCRIPT_START_TIME
end

# Shows the passed message, the running time of the script, waits for the user to confirm, and then exits.
def halt(message, exit_code = 0)
  message = "Error: #{message}" if exit_code.nonzero?

  message = "\n\n#{message}"

  # Only show the running time if the script took more than five seconds
  message += ", time: #{format_time(elapsed_script_time)}" if elapsed_script_time.to_i > 5

  puts message

  STDIN.gets if windows? && !ENV.include?('CARBON_DISABLE_SCRIPT_WAIT')

  exit exit_code
end

# Shows an error message and then exits.
def error(message)
  halt message, 1
end

# Returns the number of CPUs present on the currently running machine.
def cpu_count
  if windows?
    ENV['NUMBER_OF_PROCESSORS'].to_i
  elsif macosx?
    IO.popen('sysctl -n hw.ncpu').readline.to_i
  elsif linux?
    IO.popen('grep -c ^processor /proc/cpuinfo').readline.to_i
  else
    2
  end
end

# On Linux this returns the name of the build machine's architecture.
def linux_architecture
  linux? && IO.popen('uname -m').readlines.join.strip.to_sym
end

# Returns a list of all the sample applications.
def sample_applications
  Dir.glob(File.join(File.expand_path(File.dirname(__FILE__)), '../Source/*Sample')).map { |dir| File.basename dir }
end

# Prepends `cd <working-directory>` to the passed command
def prepend_cd(command, working_directory)
  if working_directory
    error "Working directory does not exist: #{working_directory}" unless File.directory? working_directory

    "cd #{File.expand_path(working_directory).quoted} && #{command}"
  else
    command
  end
end

# Runs the passed command with IO.popen and yields for each output line. Returns success status.
def popen(command)
  IO.popen "#{command} 2>&1" do |pipe|
    loop do
      line = pipe.readline
      yield line if block_given?
    end
  end
rescue EOFError
  $CHILD_STATUS.exitstatus.zero?
end

# Executes the passed command and returns a success flag. If a block is given then it is called for every line output by
# the command, otherwise each line of output is printed to stdout. Supported options:
#
#   :echo                   Whether to print 'command' to stdout prior to executing it. Defaults to true. If :echo is
#                           set to a string then that string is output instead of 'command'.
#   :echo_prefix            When :echo is true (or omitted) then :echo_prefix is prefixed to the default echo output.
#   :error                  If the command fails and :error is a callable object then it is called, otherwise error() is
#                           called with :error as its message parameter.
#   :working_directory      The working directory in which to execute the command.
def run(command, options = {})
  command = prepend_cd command, options[:working_directory]

  echo = options.fetch :echo, true

  if echo.is_a? String
    puts echo
  elsif echo
    puts "#{options[:echo_prefix]}#{command}"
  end

  result = popen(command) { |line| block_given? ? yield(line) : print(line) }

  handle_run_error options unless result

  result
end

def handle_run_error(options)
  if options[:error].respond_to? :call
    options[:error].call
  elsif options[:error]
    error options[:error]
  end
end

# Add a String#quoted method
class String
  def quoted
    self =~ / / ? %("#{self}") : dup
  end
end

# On Mac OS X, creates a .dmg disk image from the specified options
def create_macosx_dmg(options)
  source_folder = options.fetch :source_folder
  volume_name = options.fetch :volume_name
  target = options.fetch :target

  command = <<-END
    hdiutil create -fs HFS+ -format UDBZ -srcfolder #{source_folder.quoted} -volname #{volume_name.quoted} #{target.quoted} &&
    hdiutil internet-enable -yes #{target.quoted}
END

  run command, echo: false, error: 'Failed creating DMG file'
end

# Opens the specified file with the default application for its file type on the current platform.
def open_file_with_default_application(file)
  return if file.nil?

  if windows?
    command = "start #{file.split('/').map(&:quoted).join('\\')}"
  elsif macosx?
    command = "open #{file.quoted}"
  elsif linux?
    command = "xdg-open \"file://#{file}\""
  end

  run command, echo: "Opening #{file} ..."
end

# Waits on the specified array of threads. Yields to an optional block if the user presses enter while waiting.
def wait_on_threads(*threads)
  while threads.any?
    sleep 0.1
    threads.select!(&:alive?)

    begin
      yield threads if block_given? && STDIN.read_nonblock(1)
    rescue Errno::EAGAIN
      next
    end
  end
end

# For use on Mac OS X and iOS, this method merges the passed static libraries into a single output static library.
def merge_static_libraries(inputs, output, options = {})
  inputs = Array(inputs).select { |input| File.file? input }

  FileUtils.mkpath File.dirname(output)

  options[:sdk] ||= :macosx

  command = "xcrun -sdk #{options[:sdk]} libtool -static -o #{output.quoted} #{inputs.map(&:quoted).join ' '}"

  run command, { echo: false, error: 'Failed merging static libraries' }.merge(options) do |line|
    print line unless line =~ /has no symbols/
  end

  output
end

# Copies the source file to the destination and ensures the destination path is created if needed.
def cp(source, destination)
  FileUtils.mkpath File.dirname(destination)
  FileUtils.cp source, destination
end
