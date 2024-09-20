include "gcli/gitlab/comments.h";

parser gitlab_comment is
object of struct gcli_comment with
	("created_at" => date as iso8601_time,
	 "body"       => body as string,
	 "author"     => author as user,
	 "id"         => id as id);

parser gitlab_comments is
array of struct gcli_comment use parse_gitlab_comment;
