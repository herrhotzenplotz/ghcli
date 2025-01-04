include "gcli/gitlab/releases.h";

parser gitlab_release_asset is
object of struct gcli_release_asset with
	("url" => url as string);

parser gitlab_release_assets is
object of struct gcli_release with
	("sources" => assets as array of gcli_release_asset
	              use parse_gitlab_release_asset);

parser gitlab_release is
object of struct gcli_release with
	("name"             => name as string,
	 "tag_name"         => id as string,
	 "description"      => body as string,
	 "assets"           => use parse_gitlab_release_assets,
	 "author"           => author as user,
	 "created_at"       => date as iso8601_time,
	 "upcoming_release" => prerelease as bool);

parser gitlab_releases is
array of struct gcli_release use parse_gitlab_release;
