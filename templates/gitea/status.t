include "gcli/status.h";

parser gitea_notification_repository is
object of struct gcli_notification with
	("full_name" => repository as string);

parser gitea_notification_status is
object of struct gcli_notification with
	("title" => title as string,
	 "type" => type as gitea_notification_target_type);

parser gitea_notification is
object of struct gcli_notification with
	("id" => id as int_to_string,
	 "repository" => use parse_gitea_notification_repository,
	 "subject" => use parse_gitea_notification_status,
	 "updated_at" => date as string);

parser gitea_notifications is
array of struct gcli_notification use parse_gitea_notification;
