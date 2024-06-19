#include <err.h>
#include <string.h>

#include <gcli/gcli.h>
#include <gcli/ctx.h>
#include <gcli/status.h>

#include <pdjson/pdjson.h>

#include "gcli_tests.h"

#include <templates/gitea/status.h>

static gcli_forge_type
get_gitea_forge_type(struct gcli_ctx *ctx)
{
	(void) ctx;
	return GCLI_FORGE_GITEA;
}

static struct gcli_ctx *
test_context(void)
{
	struct gcli_ctx *ctx;
	ATF_REQUIRE(gcli_init(&ctx, get_gitea_forge_type, NULL, NULL) == NULL);
	return ctx;
}

static FILE *
open_sample(char const *const name)
{
	FILE *r;
	char p[4096] = {0};

	snprintf(p, sizeof p, "%s/samples/%s", TESTSRCDIR, name);

	ATF_REQUIRE(r = fopen(p, "r"));

	return r;
}

ATF_TC_WITHOUT_HEAD(gitea_simple_notification);
ATF_TC_BODY(gitea_simple_notification, tc)
{
	struct gcli_notification notification = {0};
	FILE *sample;
	struct json_stream stream = {0};
	struct gcli_ctx *ctx;

	ctx = test_context();
	sample = open_sample("gitea_simple_notification.json");

	json_open_stream(&stream, sample);
	ATF_REQUIRE(parse_gitea_notification(ctx, &stream, &notification) == 0);

	ATF_CHECK_STREQ(notification.id, "511579");
	ATF_CHECK_STREQ(notification.title, "Remove register from C++ sources");
	ATF_CHECK(notification.reason == NULL);
	ATF_CHECK_STREQ(notification.date, "2023-11-24T21:01:50Z");
	ATF_CHECK_STREQ(notification.repository, "schilytools/schilytools");

	fclose(sample);
	gcli_free_notification(&notification);
	gcli_destroy(&ctx);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, gitea_simple_notification);
	return atf_no_error();
}
