/*
 * Copyright 2021, 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/github/config.h>
#include <gcli/config.h>
#include <sn/sn.h>

static sn_sv
github_default_account_name(gcli_ctx *ctx)
{
	sn_sv section_name;

	section_name = gcli_config_get_override_default_account(ctx);

	if (sn_sv_null(section_name)) {
		section_name = gcli_config_find_by_key(
			ctx,
			SV("defaults"),
			"github-default-account");

		if (sn_sv_null(section_name))
			warnx("Config file does not name a default GitHub account name.");
	}

	return section_name;
}

char *
github_get_apibase(gcli_ctx *ctx)
{
	sn_sv account_name = github_default_account_name(ctx);
	if (sn_sv_null(account_name))
		goto default_val;

	sn_sv api_base = gcli_config_find_by_key(ctx, account_name, "apibase");

	if (sn_sv_null(api_base))
		goto default_val;

	return sn_sv_to_cstr(api_base);

default_val:
	return "https://api.github.com";
}

char *
github_get_authheader(gcli_ctx *ctx)
{
	sn_sv const account = github_default_account_name(ctx);
	if (sn_sv_null(account))
		return NULL;

	sn_sv const token = gcli_config_find_by_key(ctx, account, "token");
	if (sn_sv_null(token))
		errx(1, "Missing Github token");
	return sn_asprintf("Authorization: token "SV_FMT, SV_ARGS(token));
}

int
github_get_account(gcli_ctx *ctx, sn_sv *out)
{
	sn_sv const section = github_default_account_name(ctx);
	if (sn_sv_null(section))
		return gcli_error(ctx, "no default github account");

	sn_sv const account = gcli_config_find_by_key(ctx, section, "account");
	if (!account.length)
		return gcli_error(ctx, "Missing Github account name");

	*out = account;

	return 0;
}
