#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

require 'tempfile'

# Methods for using NSIS on Windows.
module NSIS
  module_function

  # Runs NSIS on the specified script file, options are the same as for run().
  def nsis(script_file, options = {})
    options[:echo] ||= 'Building installer ...'
    options[:error] ||= 'NSIS failed'

    run "#{executable.quoted} /V2 /NOCD #{script_file.quoted}", options
  end

  # Runs NSIS on a temporary script file with the specified contents.
  def nsis_with_script_content(script_content, options = {})
    temp_file = Tempfile.new 'content.nsi'
    temp_file.write script_content
    temp_file.close

    nsis temp_file.path, options
  ensure
    temp_file.close
    temp_file.unlink
  end

  # Returns the path to the main NSIS executable.
  def executable
    nsis = installation_details

    fail if nsis[:version][:major] < 3

    File.join nsis[:install_directory], 'makensis.exe'
  rescue
    error 'NSIS 3 or later is required to build installers, download it from nsis.sourceforge.net'
  end

  # Returns NSIS' installation details from the Windows registry.
  def installation_details
    require 'win32/registry'

    Win32::Registry::HKEY_LOCAL_MACHINE.open('Software\NSIS') do |reg|
      {
        version: { major: reg.read('VersionMajor')[1], minor: reg.read('VersionMinor')[1] },
        install_directory: reg.read('')[1]
      }
    end
  end

  # NSIS scripts can't handle forward slashes so this method is used in the script templates to enforce backslashes
  def path(incompatible_path)
    incompatible_path.tr '/', '\\'
  end
end
