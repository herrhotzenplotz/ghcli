include "gcli/releases.h";

parser github_release_asset is
object of struct gcli_release_asset with
	("browser_download_url" => url as string,
	 "name"                 => name as string);

parser github_release is
object of struct gcli_release with
	("name"       => name as string,
	 "body"       => body as string,
	 "id"         => id as int_to_string,
	 "author"     => author as user,
	 "created_at" => date as iso8601_time,
	 "draft"      => draft as bool,
	 "prerelease" => prerelease as bool,
	 "assets"     => assets as array of gcli_release_asset
	                 use parse_github_release_asset,
	 "upload_url" => upload_url as string);

parser github_releases is array of struct gcli_release
	use parse_github_release;
