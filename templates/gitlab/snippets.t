include "gcli/json_util.h";
include "gcli/gitlab/snippets.h";

parser gitlab_snippet is
object of struct gcli_gitlab_snippet with
	("title"      => title as string,
	 "id"         => id as id,
	 "raw_url"    => raw_url as string,
	 "created_at" => date as string,
	 "file_name"  => filename as string,
	 "author"     => author as user,
	 "visibility" => visibility as string);

parser gitlab_snippets is
array of struct gcli_gitlab_snippet use parse_gitlab_snippet;
