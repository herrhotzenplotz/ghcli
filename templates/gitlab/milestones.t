include "gcli/milestones.h";

parser gitlab_milestone is
object of struct gcli_milestone with
	("title" => title as string,
	 "id" => id as id,
	 "state" => state as string,
	 "created_at" => created_at as iso8601_time,
	 "description" => description as string,
	 "updated_at" => updated_at as iso8601_time,
	 "due_date" => due_date as iso8601_time,
	 "expired" => expired as bool);

parser gitlab_milestones is array of struct gcli_milestone use parse_gitlab_milestone;
