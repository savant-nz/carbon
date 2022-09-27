#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# Methods for interacting with the SCons build system.
module SCons
  module_function

  # The supported key=value arguments for SCons builds.
  ARGUMENTS = [:architecture, :carbonroot, :compiler, :externalapp, :externalappsourceroot, :gccversion, :platform,
               :slim, :static, :strict, :type].freeze

  # Runs SCons with the passed options and returns an array of built executables, one for each entry in
  # options[:targets].
  def scons(options = {}, &block)
    run_options = if options[:clean]
                    { echo_prefix: 'Cleaning ... ', error: 'Clean failed' }
                  else
                    { echo_prefix: 'Building ... ', error: 'Build failed' }
                  end

    run_options[:echo] = false unless options[:verbose]

    run command(options), run_options.merge(options.merge(working_directory: nil)), &block

    Array(options[:targets]).map { |target| target_executable_path target, options }
  end

  # Returns the list of commands that will be run by SCons when passed the given options.
  def build_commands(options = {})
    result = []

    run command(options.merge(verbose: true)) + ' --dry-run', echo: false, error: 'Failed running SCons' do |line|
      result << line.strip
    end

    result
  end

  # Returns the default arguments for the specified platform.
  def default_arguments(platform)
    {
      Android:      { architecture: :ARMv7,             compiler: :GCC },
      iOS:          { architecture: :ARM64,             compiler: :Clang },
      iOSSimulator: { architecture: :ARM64,             compiler: :Clang },
      Linux:        { architecture: linux_architecture, compiler: :GCC },
      macOS:        { architecture: :x64,               compiler: :Clang },
      Windows:      { architecture: :x86,               compiler: :VisualStudio2015 }
    }.fetch(platform).merge(
      platform: platform, type: :Release, strict: false
    )
  end

  # Returns the executable that should be used in order to invoke SCons on this platform.
  def executable
    return @executable if @executable

    @executable = 'scons'
    @executable += '.bat' if windows?

    run("#{@executable} -v", echo: false, error: 'SCons is either not installed or is not on the path') {}

    @executable
  end

  # Returns the SCons invocation command for the passed options.
  def command(options)
    [
      executable,
      directory_param(options),
      jobs_param(options),
      verbose_param(options),
      clean_param(options),
      arguments_hash_to_string(merge_default_arguments(options[:arguments])),
      Array(options[:targets])
    ].flatten.compact.join ' '
  end

  def directory_param(options)
    "--directory #{options[:working_directory].quoted}" if options.key? :working_directory
  end

  def jobs_param(options)
    "--jobs #{options[:jobs] || cpu_count}"
  end

  def verbose_param(options)
    options.fetch(:verbose, false) ? nil : '--silent'
  end

  def clean_param(options)
    options.fetch(:clean, false) ? '--clean' : nil
  end

  # Merges the default arguments onto the passed arguments hash.
  def merge_default_arguments(arguments)
    arguments ||= {}
    default_arguments(arguments[:platform] || platform).merge arguments
  end

  # Converts the passed arguments hash into a valid SCons arguments 'key=value' string.
  def arguments_hash_to_string(arguments)
    arguments.select! { |argument| ARGUMENTS.include? argument }

    arguments.map do |argument, value|
      value.nil? ? nil : "#{argument}=#{value.to_s.quoted}"
    end.compact.join ' '
  end

  # Returns the path to the executable built by SCons for the given target.
  def target_executable_path(target, options)
    arguments = merge_default_arguments options.fetch :arguments

    [
      options[:working_directory] || '.',
      'Build',
      arguments[:platform],
      arguments[:architecture],
      arguments[:compiler],
      arguments[:type],
      "#{target}#{target_executable_suffix}"
    ].join '/'
  end

  def target_executable_suffix
    windows? ? '.exe' : ''
  end
end
