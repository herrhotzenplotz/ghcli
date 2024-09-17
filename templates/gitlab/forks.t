include "gcli/gitlab/forks.h";

parser gitlab_fork_namespace is
object of struct gcli_fork with
	("full_path" => owner as string);

parser gitlab_fork is
object of struct gcli_fork with
	("path_with_namespace" => full_name as string,
	 "namespace"           => use parse_gitlab_fork_namespace,
	 "created_at"          => date as iso8601_time,
	 "forks_count"         => forks as int);

parser gitlab_forks is
array of struct gcli_fork use parse_gitlab_fork;
