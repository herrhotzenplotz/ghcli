include "gcli/comments.h";
include "gcli/issues.h";
include "gcli/bugzilla/bugs.h";
include "gcli/bugzilla/bugs-parser.h";

parser bugzilla_bug_creator is
object of struct gcli_issue with
	("real_name" => author as string);

parser bugzilla_assigned_to_detail is
object of struct gcli_issue with
	("name" => use parse_bugzilla_assignee);

parser bugzilla_bug_item is
object of struct gcli_issue with
	("id" => number as id,
	 "summary" => title as string,
	 "creation_time" => created_at as iso8601_time,
	 "creator_detail" => use parse_bugzilla_bug_creator,
	 "status" => state as string,
	 "product" => product as string,
	 "component" => component as string,
	 "status" => state as string,
	 "product" => product as string,
	 "component" => component as string,
	 "assigned_to_detail" => use parse_bugzilla_assigned_to_detail,
	 "url" => url as string);

parser bugzilla_bugs is
object of struct gcli_issue_list with
	("bugs" => issues as array of gcli_issue use parse_bugzilla_bug_item);

parser bugzilla_comment is
object of struct gcli_comment with
	("id" => id as id,
	 "text" => body as string,
	 "creation_time" => date as iso8601_time,
	 "creator" => author as string);

parser bugzilla_comments_internal_skip_first is
object of struct gcli_comment_list with
	("comments" => use parse_bugzilla_comments_array_skip_first);

parser bugzilla_comments is
object of struct gcli_comment_list with
	("bugs" => use parse_bugzilla_bug_comments_dictionary_skip_first);

parser bugzilla_comment_text is
object of char* select "text" as string;

parser bugzilla_comments_internal_only_first is
object of char* with
	("comments" => use parse_bugzilla_comments_array_only_first);

parser bugzilla_bug_op is
object of char* with
	("bugs" => use parse_bugzilla_bug_comments_dictionary_only_first);

parser bugzilla_bug_attachments is
object of struct gcli_attachment_list with
	("bugs" => use parse_bugzilla_bug_attachments_dict);

parser bugzilla_bug_attachment is
object of struct gcli_attachment with
	("id" => id as id,
	 "summary" => summary as string,
	 "file_name" => file_name as string,
	 "creation_time" => created_at as iso8601_time,
	 "creator" => author as string,
	 "content_type" => content_type as string,
	 "is_obsolete" => is_obsolete as bool_relaxed,
	 "data" => data_base64 as string);

parser bugzilla_bug_attachments_internal is
array of struct gcli_attachment use parse_bugzilla_bug_attachment;

parser bugzilla_attachment_content is
object of struct gcli_attachment with
	("attachments" => use parse_bugzilla_attachment_content_only_first);

parser bugzilla_bug_creation_result is
object of gcli_id select "id" as id;

parser bugzilla_single_comment is
object of struct gcli_comment with
	("comments" => use parse_bugzilla_single_comments_array_only_first);
