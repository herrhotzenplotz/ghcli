include "gcli/milestones.h";

parser gitea_milestone is
object of struct gcli_milestone with
       ("id" => id as id,
        "title" => title as string,
	"created_at" => created_at as iso8601_time,
	"description" => description as string,
	"state" => state as string,
	"updated_at" => updated_at as iso8601_time,
	"open_issues" => open_issues as int,
	"due_on" => due_date as iso8601_time,
	"closed_issues" => closed_issues as int);

parser gitea_milestones is
array of struct gcli_milestone use parse_gitea_milestone;
