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

#ifndef GCLI_BUGZILLA_BUGS_H
#define GCLI_BUGZILLA_BUGS_H

#include <config.h>

#include <gcli/comments.h>
#include <gcli/curl.h>
#include <gcli/issues.h>

#include <pdjson/pdjson.h>

int bugzilla_get_bugs(struct gcli_ctx *ctx, struct gcli_path const *path,
                      struct gcli_issue_fetch_details const *details,
                      int const max, struct gcli_issue_list *out);

int bugzilla_get_bug(struct gcli_ctx *ctx, struct gcli_path const *path,
                     struct gcli_issue *out);

int bugzilla_bug_get_comments(struct gcli_ctx *const ctx,
                              struct gcli_path const *bug_path,
                              struct gcli_comment_list *out);

int bugzilla_bug_get_comment(struct gcli_ctx *ctx,
                             struct gcli_path const *target,
                             enum comment_target_type target_type,
                             gcli_id comment_id, struct gcli_comment *out);

int bugzilla_bug_get_attachments(struct gcli_ctx *ctx,
                                 struct gcli_path const *bug_path,
                                 struct gcli_attachment_list *const out);

int bugzilla_bug_submit(struct gcli_ctx *ctx,
                        struct gcli_submit_issue_options *opts,
                        struct gcli_issue *out);

#endif /* GCLI_BUGZILLA_BUGS_H */
