include "gcli/sshkeys.h";

parser gitlab_sshkey is
object of struct gcli_sshkey with
	("title" => title as string,
	 "id" => id as id,
	 "key" => key as string,
	 "created_at" => created_at as iso8601_time);

parser gitlab_sshkeys is array of struct gcli_sshkey use parse_gitlab_sshkey;
