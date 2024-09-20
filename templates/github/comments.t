include "gcli/github/comments.h";

parser github_comment is
object of struct gcli_comment with
	("id"         => id as id,
	 "created_at" => date as iso8601_time,
	 "body"       => body as string,
	 "user"       => author as user);

parser github_comments is array of struct gcli_comment
	use parse_github_comment;
