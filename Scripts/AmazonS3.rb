#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# Methods for interacting with Amazon S3. By default the Amazon S3 Access Key ID and Secret Access Key are read out of the
# CARBON_S3_ACCESS_KEY_ID and CARBON_S3_SECRET_ACCESS_KEY environment variables.
module AmazonS3
  module_function

  def upload_file(source_file, options)
    object = s3_object source_file, options

    print "Uploading '#{File.basename source_file}' to S3 bucket '#{options[:bucket]}' ... "

    begin
      object.upload_file source_file, acl: options[:acl]
      puts 'done'

      object.public_url
    rescue Aws::S3::Errors::ServiceError, Aws::S3::Errors::MultipartServiceError => e
      puts "ERROR: #{e.code}"

      nil
    end
  end

  def require_gem
    require 'aws-sdk'

    Aws.config[:ssl_verify_peer] = false if windows?
  rescue LoadError
    error 'The aws-sdk gem is not installed'
  end

  def credentials(options)
    access_key_id = options.fetch :access_key_id, ENV.fetch('CARBON_S3_ACCESS_KEY_ID')
    secret_access_key = options.fetch :secret_access_key, ENV.fetch('CARBON_S3_SECRET_ACCESS_KEY')

    Aws::Credentials.new access_key_id, secret_access_key
  end

  def s3_object(source_file, options)
    require_gem

    s3 = Aws::S3::Resource.new credentials: credentials(options), region: options.fetch(:region, 'us-east-1')

    bucket = s3.bucket options.fetch(:bucket)

    object_name = options.fetch :object, File.basename(source_file)

    bucket.object object_name
  end
end
