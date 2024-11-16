include "gcli/forks.h";

parser github_fork is
object of struct gcli_fork with
	("full_name"   => full_name as string,
	 "owner"       => owner as user,
	 "created_at"  => date as iso8601_time,
	 "forks_count" => forks as int);

parser github_forks is array of struct gcli_fork
	use parse_github_fork;
