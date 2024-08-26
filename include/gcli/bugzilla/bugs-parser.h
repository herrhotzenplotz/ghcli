/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GCLI_BUGZILLA_BUGS_PARSER_H
#define GCLI_BUGZILLA_BUGS_PARSER_H

#include <config.h>

#include <gcli/comments.h>
#include <gcli/gcli.h>
#include <gcli/issues.h>

#include <pdjson/pdjson.h>

int parse_bugzilla_bug_comments_dictionary_skip_first(struct gcli_ctx *const ctx,
                                                      struct json_stream *stream,
                                                      struct gcli_comment_list *out);

int parse_bugzilla_comments_array_skip_first(struct gcli_ctx *ctx,
                                             struct json_stream *stream,
                                             struct gcli_comment_list *out);

int parse_bugzilla_bug_comments_dictionary_only_first(struct gcli_ctx *const ctx,
                                                      struct json_stream *stream,
                                                      char **out);

int parse_bugzilla_comments_array_only_first(struct gcli_ctx *ctx,
                                             struct json_stream *stream,
                                             char **out);

int parse_bugzilla_assignee(struct gcli_ctx *ctx, struct json_stream *stream,
                            struct gcli_issue *out);

int parse_bugzilla_bug_attachments_dict(struct gcli_ctx *ctx,
                                        struct json_stream *stream,
                                        struct gcli_attachment_list *out);

int parse_bugzilla_attachment_content_only_first(struct gcli_ctx *ctx,
                                                 struct json_stream *stream,
                                                 struct gcli_attachment *out);

int parse_bugzilla_single_comments_array_only_first(struct gcli_ctx *ctx,
                                                    struct json_stream *stream,
                                                    struct gcli_comment *out);

#endif /* GCLI_BUGZILLA_BUGS_PARSER_H */
