#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

require 'find'

# Methods for interacting with Git repositories.
module Git
  module_function

  def git(repository, command, options = {})
    error "Invalid git repository: #{repository}" unless repository? repository

    options[:echo] = false unless options.key? :echo
    options[:working_directory] = repository

    lines = []

    succeeded = run "#{executable} #{command}", options do |line|
      line.rstrip!
      lines << line
      yield line if block_given?
    end

    succeeded && lines
  end

  def executable
    return @executable if @executable

    @executable = macos? ? 'xcrun git' : 'git'

    error 'Git is not installed' unless popen "#{@executable} --version"

    @executable
  end

  def repository?(repository)
    File.directory?(File.join(repository, '.git')) && !File.directory?(File.join(repository, '.git', 'svn'))
  end

  def uncommitted_changes?(repository)
    (git(repository, 'status --short --untracked-files=no') || []).any?
  end

  def rev_parse(repository, options)
    (git(repository, 'rev-parse ' + options) || []).first
  end

  def head_commit_hash(repository)
    rev_parse repository, 'HEAD'
  end

  def short_head_commit_hash(repository)
    rev_parse repository, '--short HEAD'
  end

  def active_branch(repository)
    rev_parse repository, '--abbrev-ref HEAD'
  end

  def list_files(repository)
    git repository, 'ls-files'
  end

  def repositories_in_directory(directory)
    Dir.new(directory).entries.map { |dir| "#{directory}/#{dir}" }.select { |candidate| repository? candidate }
  end

  def directory_size(directory)
    size = 0

    Find.find directory do |file|
      size += ((File.stat(file).size + 4095) / 4096) * 4096
    end

    size / (1024 * 1024)
  end

  def compact_repository(repository)
    initial_size = directory_size repository

    git repository, 'gc --aggressive', echo: true, error: 'Failed compacting git repository'

    initial_size - directory_size(repository)
  end

  def compact_repositories_in_directory(directory)
    start_time = Time.now
    total_space_reclaimed = 0

    repositories_in_directory(directory).each do |repository|
      space_reclaimed = compact_repository repository

      puts "Reduced size of #{dir} by #{space_reclaimed}MB\n\n"
      total_space_reclaimed += space_reclaimed
    end

    puts "Git repository compaction reclaimed #{total_space_reclaimed}MB, time: #{format_time(Time.now - start_time)}"
  end

  def update_repository(repository)
    output = []

    git repository, 'pull --rebase' do |line|
      output << line
    end

    output = ['done'] if output.empty?

    output
  end

  def print_update_repository_output(repository, output)
    print "#{File.basename repository} ... "

    if output.size == 1
      output[0] = 'done' if output.first =~ /^Current branch .* is up to date.$/
      puts output.first
    else
      puts ''
      output.each { |line| puts ' ' * 8 + line }
    end
  end

  def update_repositories_in_directory(directory)
    threads = repositories_in_directory(directory).map do |repo|
      Thread.new(repo) do |repository|
        Thread.current[:repository] = File.basename repository

        output = update_repository repository

        print_update_repository_output repository, output
      end
    end

    wait_on_threads(*threads) do |pending_threads|
      puts "Waiting on #{pending_threads.map { |t| t[:repository] }.join ', '}"
    end
  end
end
