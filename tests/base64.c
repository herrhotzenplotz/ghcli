#include <atf-c.h>

#include <gcli/base64.h>

ATF_TC_WITHOUT_HEAD(simple_decode);
ATF_TC_BODY(simple_decode, tc)
{
	char const input[] = "aGVsbG8gd29ybGQ=";
	char output[sizeof("hello world")] = {0};

	int rc = gcli_decode_base64(NULL, input, output, sizeof(output));
	ATF_REQUIRE(rc == 0);
	ATF_CHECK_STREQ(output, "hello world");
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, simple_decode);

	return atf_no_error();
}
