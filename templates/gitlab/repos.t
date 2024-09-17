include "gcli/gitlab/repos.h";

parser gitlab_repo is
object of struct gcli_repo with
	("path_with_namespace" => full_name as string,
	 "name"                => name as string,
	 "owner"               => owner as user,
	 "created_at"          => date as iso8601_time,
	 "visibility"          => visibility as string,
	 "fork"                => is_fork as bool,
	 "id"                  => id as id);

parser gitlab_repos is
array of struct gcli_repo use parse_gitlab_repo;
