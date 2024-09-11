include "gcli/issues.h";
include "gcli/labels.h";

include "templates/github/labels.h";

parser github_issue_milestone is
object of struct gcli_issue with
	("title" => milestone as string);

parser github_issue is
object of struct gcli_issue with
	("title"        => title as string,
	 "state"        => state as string,
	 "body"         => body as string,
	 "created_at"   => created_at as iso8601_time,
	 "number"       => number as id,
	 "comments"     => comments as int,
	 "user"         => author as user,
	 "locked"       => locked as bool,
	 "labels"       => labels as array of github_label
	                   use parse_github_label_text,
	 "assignees"    => assignees as array of char* use get_user,
	 "pull_request" => is_pr as github_is_pr,
	 "milestone"    => use parse_github_issue_milestone);

parser github_issues is array of struct gcli_issue use parse_github_issue;

parser github_issue_search_result is
object of struct gcli_issue_list with
	("items" => issues as array of gcli_issue use parse_github_issue);
