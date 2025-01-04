include "gcli/gitlab/issues.h";

parser gitlab_user is object of char* select "username" as string;

parser gitlab_issue_milestone is
object of struct gcli_issue with
	("title" => milestone as string);

parser gitlab_issue is
object of struct gcli_issue with
	("title"             => title as string,
	 "state"             => state as string,
	 "description"       => body as string,
	 "created_at"        => created_at as iso8601_time,
	 "iid"               => number as id,
	 "user_notes_count"  => comments as int,
	 "author"            => author as user,
	 "discussion_locked" => locked as bool,
	 "labels"            => labels as array of char* use get_string,
	 "assignees"         => assignees as array of gitlab_user
	                        use parse_gitlab_user,
	 "milestone"         => use parse_gitlab_issue_milestone);

parser gitlab_issues is array of struct gcli_issue use parse_gitlab_issue;
