include "gcli/milestones.h";

parser github_milestone is
object of struct gcli_milestone with
	("number" => id as id,
	 "title" => title as string,
	 "created_at" => created_at as iso8601_time,
	 "state" => state as string,
	 "updated_at" => updated_at as iso8601_time,
	 "description" => description as string,
	 "open_issues" => open_issues as int,
	 "closed_issues" => closed_issues as int);

parser github_milestones is
array of struct gcli_milestone use parse_github_milestone;
