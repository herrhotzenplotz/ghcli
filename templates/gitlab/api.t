include "gcli/gitlab/api.h";

parser gitlab_get_error is
object of struct gitlab_error_data with
	("error_description" => error_description as string,
	 "error" => error as string,
	 "message" => message as string);
