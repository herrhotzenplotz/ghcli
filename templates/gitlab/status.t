include "gcli/gitlab/status.h";

parser gitlab_project is
object of struct gcli_notification with
	("path_with_namespace" => repository as string);

parser gitlab_target is
object of struct gcli_notification with
	("iid" => target_id as id);

parser gitlab_todo is
object of struct gcli_notification with
	("updated_at"  => date as string,
	 "action_name" => reason as string,
	 "id"          => id as int_to_string,
	 "body"        => title as string,
	 "target_type" => type as gitlab_notification_target_type,
	 "project"     => use parse_gitlab_project,
	 "target"      => use parse_gitlab_target);

parser gitlab_todos is
array of struct gcli_notification use parse_gitlab_todo;
