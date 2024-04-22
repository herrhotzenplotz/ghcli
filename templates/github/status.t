include "gcli/github/status.h";

parser github_notification_subject is
object of struct gcli_notification with
	("title" => title as string,
	 "type"  => type as github_notification_target_type);

parser github_notification_repository is
object of struct gcli_notification with
	("full_name" => repository as string);

parser github_notification is
object of struct gcli_notification with
	("updated_at" => date as string,
	 "id"         => id as string,
	 "reason"     => reason as string,
	 "subject"    => use parse_github_notification_subject,
	 "repository" => use parse_github_notification_repository);

parser github_notifications is array of struct gcli_notification
	use parse_github_notification;
