/*
 * Copyright 2021, 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
 * Copyright 2022 Aritra Sarkar <aritra1911@yahoo.com>
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

#include <ctype.h>
#include <string.h>

#include <gcli/curl.h>
#include <gcli/forges.h>
#include <gcli/json_util.h>

#include <curl/curl.h>
#include <sn/sn.h>
#include <pdjson/pdjson.h>

/* Hack for NetBSD's and Oracle Solaris broken isalnum implementation */
#if defined(__NetBSD__) || (defined(__SVR4) && defined(__sun))
#  ifdef isalnum
#    undef isalnum
#  endif
#  define isalnum gcli_curl_isalnum

/* TODO: this is fucked in case we are working on an EBCDIC machine
 * (wtf are you doing anyways?) */
static int
gcli_curl_isalnum(char const c)
{
	return ('A' <= c && c <= 'Z')
		|| ('a' <= c && c <= 'z')
		|| ('0' <= c && c <= '9');
}

#endif /* __NetBSD and Oracle Solaris */

/* XXX move to gcli_ctx destructor */
void
gcli_curl_ctx_destroy(struct gcli_ctx *ctx)
{
	if (ctx->curl)
		curl_easy_cleanup(ctx->curl);

	ctx->curl = NULL;

	free(ctx->curl_useragent);
	ctx->curl_useragent = NULL;
}

/* Ensures a clean cURL handle. Call this whenever you wanna use the
 * ctx->curl */
static int
gcli_curl_ensure(struct gcli_ctx *ctx)
{
	if (ctx->curl) {
		curl_easy_reset(ctx->curl);
	} else {
	    ctx->curl = curl_easy_init();
	    if (!ctx->curl)
		    return gcli_error(ctx, "failed to initialise curl context");
	}

	if (!ctx->curl_useragent) {
		curl_version_info_data const *ver;

		ver = curl_version_info(CURLVERSION_NOW);
		ctx->curl_useragent = sn_asprintf("curl/%s", ver->version);
	}

	return 0;
}

/* Check the given curl code for an OK result. If not, print an
 * appropriate error message and exit */
static int
gcli_curl_check_api_error(struct gcli_ctx *ctx, CURLcode code, char const *url,
                          struct gcli_fetch_buffer *const result)
{
	long status_code = 0;

	if (code != CURLE_OK) {
		return gcli_error(ctx, "request to %s failed: curl error: %s",
		                  url, curl_easy_strerror(code));
	}

	curl_easy_getinfo(ctx->curl, CURLINFO_RESPONSE_CODE, &status_code);

	if (status_code >= 300L) {
		return gcli_error(ctx,
		                  "request to %s failed with code %ld: API error: %s",
		                  url, status_code,
		                  gcli_forge(ctx)->get_api_error_string(ctx, result));
	}

	return 0;
}

/* Callback for writing data into the struct gcli_fetch_buffer passed by
 * calling routines */
static size_t
fetch_write_callback(char *in, size_t size, size_t nmemb, void *data)
{
	/* the user may have passed null indicating that we do not care
	 * about the result body of the request. */
	if (data) {
		struct gcli_fetch_buffer *out = data;

		out->data = realloc(out->data, out->length + size * nmemb);
		memcpy(&(out->data[out->length]), in, size * nmemb);
		out->length += size * nmemb;
	}

	return size * nmemb;
}

/* Plain HTTP get request.
 *
 * pagination_next returns the next url to query for paged results.
 * Results are placed into the struct gcli_fetch_buffer. */
int
gcli_fetch(struct gcli_ctx *ctx, char const *url, char **const pagination_next,
           struct gcli_fetch_buffer *out)
{
	return gcli_fetch_with_method(ctx, "GET", url, NULL, pagination_next, out);
}

static int
gcli_report_progress(void *_ctx, double dltotal, double dlnow,
                     double ultotal, double ulnow)
{
	struct gcli_ctx *ctx = _ctx;

	(void) dltotal;
	(void) dlnow;
	(void) ultotal;
	(void) ulnow;

	/* not done */
	ctx->report_progress(false);

	return 0;
}

/* Check the given url for a successful query */
int
gcli_curl_test_success(struct gcli_ctx *ctx, char const *url)
{
	CURLcode ret;
	struct gcli_fetch_buffer buffer = {0};
	long status_code;
	bool is_success = true;
	int rc = 0;

	if ((rc = gcli_curl_ensure(ctx)) < 0)
		return rc;

	curl_easy_setopt(ctx->curl, CURLOPT_URL, url);
	curl_easy_setopt(ctx->curl, CURLOPT_BUFFERSIZE, 102400L);
	curl_easy_setopt(ctx->curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(ctx->curl, CURLOPT_MAXREDIRS, 50L);
	curl_easy_setopt(ctx->curl, CURLOPT_USERAGENT, ctx->curl_useragent);
#if defined(CURL_HTTP_VERSION_2TLS)
	curl_easy_setopt(
		ctx->curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
#endif
	curl_easy_setopt(ctx->curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, &buffer);
	curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, fetch_write_callback);
	curl_easy_setopt(ctx->curl, CURLOPT_FAILONERROR, 0L);
	curl_easy_setopt(ctx->curl, CURLOPT_FOLLOWLOCATION, 1L);

	if (ctx->report_progress) {
		curl_easy_setopt(ctx->curl, CURLOPT_XFERINFOFUNCTION,
		                 gcli_report_progress);
		curl_easy_setopt(ctx->curl, CURLOPT_XFERINFODATA, ctx);
		curl_easy_setopt(ctx->curl, CURLOPT_NOPROGRESS, 0L);
	}

	ret = curl_easy_perform(ctx->curl);

	if (ret != CURLE_OK) {
		is_success = false;
	} else {
		curl_easy_getinfo(ctx->curl, CURLINFO_RESPONSE_CODE, &status_code);

		if (status_code >= 300L)
			is_success = false;
	}

	if (ctx->report_progress)
		ctx->report_progress(true);

	gcli_fetch_buffer_free(&buffer);

	return is_success;
}

/* Perform a GET request to the given URL and print the results to the
 * STREAM.
 *
 * content_type may be NULL. */
int
gcli_curl(struct gcli_ctx *ctx, FILE *stream, char const *url,
          char const *content_type)
{
	CURLcode ret;
	struct curl_slist *headers;
	struct gcli_fetch_buffer buffer = {0};
	char *auth_header = NULL;
	int rc = 0;

	headers = NULL;

	if ((rc = gcli_curl_ensure(ctx)) < 0)
		return rc;

	if (content_type)
		headers = curl_slist_append(headers, content_type);

	auth_header = gcli_get_authheader(ctx);
	if (auth_header)
		headers = curl_slist_append(headers, auth_header);

	curl_easy_setopt(ctx->curl, CURLOPT_URL, url);
	curl_easy_setopt(ctx->curl, CURLOPT_BUFFERSIZE, 102400L);
	curl_easy_setopt(ctx->curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(ctx->curl, CURLOPT_MAXREDIRS, 50L);
	curl_easy_setopt(ctx->curl, CURLOPT_FTP_SKIP_PASV_IP, 1L);
	curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(ctx->curl, CURLOPT_USERAGENT, ctx->curl_useragent);
#if defined(CURL_HTTP_VERSION_2TLS)
	curl_easy_setopt(
		ctx->curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
#endif
	curl_easy_setopt(ctx->curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, &buffer);
	curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, fetch_write_callback);
	curl_easy_setopt(ctx->curl, CURLOPT_FAILONERROR, 0L);
	curl_easy_setopt(ctx->curl, CURLOPT_FOLLOWLOCATION, 1L);

	if (ctx->report_progress) {
		curl_easy_setopt(ctx->curl, CURLOPT_XFERINFOFUNCTION,
		                 gcli_report_progress);
		curl_easy_setopt(ctx->curl, CURLOPT_XFERINFODATA, ctx);
		curl_easy_setopt(ctx->curl, CURLOPT_NOPROGRESS, 0L);
	}

	ret = curl_easy_perform(ctx->curl);
	rc = gcli_curl_check_api_error(ctx, ret, url, &buffer);

	if (ctx->report_progress)
		ctx->report_progress(true);

	if (rc == 0)
		fwrite(buffer.data, 1, buffer.length, stream);

	gcli_fetch_buffer_free(&buffer);

	curl_slist_free_all(headers);

	free(auth_header);

	return rc;
}

/* Callback to extract the link header for pagination handling. */
static size_t
fetch_header_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	char **out = userdata;

	size_t sz = size * nmemb;
	sn_sv buffer = sn_sv_from_parts(ptr, sz);
	sn_sv header_name = sn_sv_chop_until(&buffer, ':');

	/* Despite what the documentation says, this header is called
	 * "link" not "Link". Webdev ftw /sarc */
	if (sn_sv_eq_to(header_name, "link")) {
		buffer.data += 1;
		buffer.length -= 1;
		buffer = sn_sv_trim_front(buffer);
		*out = sn_strndup(buffer.data, buffer.length);
	}

	return sz;
}

/* Parse the link http header for pagination */
static char *
parse_link_header(char *_header)
{
	sn_sv header = SV(_header);
	sn_sv entry  = {0};

	/* Iterate through the comma-separated list of link relations */
	while ((entry = sn_sv_chop_until(&header, ',')).length > 0) {
		entry = sn_sv_trim(entry);

		/* the entries have semicolon-separated fields like so:
		 * <url>; rel=\"next\"
		 *
		 * This chops off the url and then looks at the rest.
		 *
		 * We're making lots of assumptions about the input data here
		 * without sanity checking it. If it fails, we will know. Most
		 * likely a segfault. */
		sn_sv almost_url = sn_sv_chop_until(&entry, ';');

		if (sn_sv_eq_to(entry, "; rel=\"next\"")) {
			/* Skip the triangle brackets around the url */
			almost_url.data += 1;
			almost_url.length -= 2;
			almost_url = sn_sv_trim(almost_url);
			return sn_sv_to_cstr(almost_url);
		}

		/* skip the comma if we have enough data */
		if (header.length > 0) {
			header.length -= 1;
			header.data += 1;
		}
	}

	return NULL;
}

/* Perform a HTTP Request with the given method to the url
 *
 * - data may be NULL.
 * - pagination_next may be NULL.
 *
 * Results are placed in the gcli_fetch_buffer.
 *
 * All requests will be done with authorization through the
 * gcli_config_get_authheader function.
 *
 * If pagination_next is non-null a URL that can be queried for more
 * data (pagination) is placed into it. If there is no more data, it
 * will be set to NULL. */
int
gcli_fetch_with_method(
	struct gcli_ctx *ctx,
	char const *method,                  /* HTTP method. e.g. POST, GET, DELETE etc. */
	char const *url,                     /* Endpoint */
	char const *data,                    /* Form data */
	char **const pagination_next,        /* Next URL for pagination */
	struct gcli_fetch_buffer *const out) /* output buffer */
{
	CURLcode ret;
	struct curl_slist *headers;
	struct gcli_fetch_buffer tmp = {0}; /* used for error codes when out is NULL */
	struct gcli_fetch_buffer *buf = NULL;
	char *link_header = NULL;
	int rc = 0;

	if ((rc = gcli_curl_ensure(ctx)) < 0)
		return rc;

	char *auth_header = gcli_get_authheader(ctx);

	if (sn_verbose())
		fprintf(stderr, "info: cURL request %s %s...\n", method, url);

	headers = NULL;
	headers = curl_slist_append(
		headers,
		"Accept: application/vnd.github.v3+json");
	headers = curl_slist_append(
		headers,
		"Content-Type: application/json");
	if (auth_header)
		headers = curl_slist_append(headers, auth_header);

	/* Only clear the output buffer if we have a pointer to it. If the
	 * user is not interested in the result we use a temporary buffer
	 * for proper error reporting. */
	if (out) {
		*out = (struct gcli_fetch_buffer) {0};
		buf = out;
	} else {
		buf = &tmp;
	}

	curl_easy_setopt(ctx->curl, CURLOPT_URL, url);

	if (data)
		curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDS, data);

	curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(ctx->curl, CURLOPT_USERAGENT, ctx->curl_useragent);
	curl_easy_setopt(ctx->curl, CURLOPT_CUSTOMREQUEST, method);
	curl_easy_setopt(ctx->curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, buf);
	curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, fetch_write_callback);
	curl_easy_setopt(ctx->curl, CURLOPT_FAILONERROR, 0L);
	curl_easy_setopt(ctx->curl, CURLOPT_HEADERFUNCTION, fetch_header_callback);
	curl_easy_setopt(ctx->curl, CURLOPT_HEADERDATA, &link_header);
	curl_easy_setopt(ctx->curl, CURLOPT_FOLLOWLOCATION, 1L);

	if (ctx->report_progress) {
		curl_easy_setopt(ctx->curl, CURLOPT_XFERINFOFUNCTION,
		                 gcli_report_progress);
		curl_easy_setopt(ctx->curl, CURLOPT_XFERINFODATA, ctx);
		curl_easy_setopt(ctx->curl, CURLOPT_NOPROGRESS, 0L);
	}

	ret = curl_easy_perform(ctx->curl);
	rc = gcli_curl_check_api_error(ctx, ret, url, buf);

	if (ctx->report_progress)
		ctx->report_progress(true);

	/* only parse these headers and continue if there was no error */
	if (rc == 0) {
		if (link_header && pagination_next)
			*pagination_next = parse_link_header(link_header);
	} else if (out) { /* error happened and we have an output buffer */
		gcli_fetch_buffer_free(out);
	}

	free(link_header);

	curl_slist_free_all(headers);
	headers = NULL;

	/* if the user is not interested in the result, free the temporary
	 * buffer */
	if (!out)
		gcli_fetch_buffer_free(&tmp);

	free(auth_header);

	return rc;
}

/* Perform a POST request to the given URL and upload the buffer to it.
 *
 * Results are placed in out.
 *
 * content_type may not be NULL.
 */
int
gcli_post_upload(struct gcli_ctx *ctx, char const *url, char const *content_type,
                 void *buffer, size_t const buffer_size,
                 struct gcli_fetch_buffer *const out)
{
	CURLcode ret;
	struct curl_slist *headers;
	int rc = 0;
	char *auth_header, *contenttype_header, *contentsize_header;

	if ((rc = gcli_curl_ensure(ctx)) < 0)
		return rc;

	auth_header = gcli_get_authheader(ctx);
	contenttype_header = sn_asprintf("Content-Type: %s",
	                                 content_type);
	contentsize_header = sn_asprintf("Content-Length: %zu",
	                                 buffer_size);

	if (sn_verbose())
		fprintf(stderr, "info: cURL upload POST %s...\n", url);

	headers = NULL;
	headers = curl_slist_append(
		headers,
		"Accept: application/vnd.github.v3+json");

	if (auth_header)
		headers = curl_slist_append(headers, auth_header);

	headers = curl_slist_append(headers, contenttype_header);
	headers = curl_slist_append(headers, contentsize_header);

	curl_easy_setopt(ctx->curl, CURLOPT_URL, url);
	curl_easy_setopt(ctx->curl, CURLOPT_POST, 1L);
	curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDS, buffer);
	curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDSIZE, (long)buffer_size);

	curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(ctx->curl, CURLOPT_USERAGENT, ctx->curl_useragent);
	curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, out);
	curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, fetch_write_callback);

	if (ctx->report_progress) {
		curl_easy_setopt(ctx->curl, CURLOPT_XFERINFOFUNCTION,
		                 gcli_report_progress);
		curl_easy_setopt(ctx->curl, CURLOPT_XFERINFODATA, ctx);
		curl_easy_setopt(ctx->curl, CURLOPT_NOPROGRESS, 0L);
	}

	ret = curl_easy_perform(ctx->curl);
	rc = gcli_curl_check_api_error(ctx, ret, url, out);

	if (ctx->report_progress)
		ctx->report_progress(true);

	curl_slist_free_all(headers);
	headers = NULL;

	free(auth_header);
	free(contentsize_header);
	free(contenttype_header);

	return rc;
}

/** gcli_gitea_upload_attachment:
 *
 *  Upload the given file to the given url. This is gitea-specific
 *  code.
 */
int
gcli_curl_gitea_upload_attachment(struct gcli_ctx *ctx, char const *url,
                                  char const *filename,
                                  struct gcli_fetch_buffer *const out)
{
	CURLcode ret;
	curl_mime *mime;
	curl_mimepart *contentpart;
	struct curl_slist *headers;
	int rc = 0;
	char *auth_header;

	if ((rc = gcli_curl_ensure(ctx)) < 0)
		return rc;

	auth_header = gcli_get_authheader(ctx);

	if (sn_verbose())
		fprintf(stderr, "info: cURL upload POST %s...\n", url);

	headers = NULL;
	headers = curl_slist_append(
		headers,
		"Accept: application/json");

	if (auth_header)
		headers = curl_slist_append(headers, auth_header);

	/* The docs say we should be using this mime thing. */
	mime = curl_mime_init(ctx->curl);
	contentpart = curl_mime_addpart(mime);

	/* Attach the file. It will be read when curl_easy_perform is
	 * called. This allows us to upload large files without reading or
	 * mapping them into memory in one chunk. */
	curl_mime_name(contentpart, "attachment");
	ret = curl_mime_filedata(contentpart, filename);
	if (ret != CURLE_OK) {
		errx(1, "error: could not set attachment for upload: %s",
		     curl_easy_strerror(ret));
	}

	curl_easy_setopt(ctx->curl, CURLOPT_URL, url);
	curl_easy_setopt(ctx->curl, CURLOPT_MIMEPOST, mime);

	curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, out);
	curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, fetch_write_callback);

	if (ctx->report_progress) {
		curl_easy_setopt(ctx->curl, CURLOPT_XFERINFOFUNCTION,
		                 gcli_report_progress);
		curl_easy_setopt(ctx->curl, CURLOPT_XFERINFODATA, ctx);
		curl_easy_setopt(ctx->curl, CURLOPT_NOPROGRESS, 0L);
	}

	ret = curl_easy_perform(ctx->curl);
	rc = gcli_curl_check_api_error(ctx, ret, url, out);

	if (ctx->report_progress)
		ctx->report_progress(true);

	/* Cleanup */
	curl_slist_free_all(headers);
	headers = NULL;
	curl_mime_free(mime);
	free(auth_header);

	return rc;
}

sn_sv
gcli_urlencode_sv(sn_sv const _input)
{
	size_t input_len;
	size_t output_len;
	size_t i;
	char *output;
	char *input;

	input = _input.data;
	input_len = _input.length;
	output = calloc(1, 3 * input_len + 1);
	output_len = 0;

	for (i = 0; i < input_len; ++i) {
		if (!isalnum(input[i]) && input[i] != '-' && input[i] != '_') {
			unsigned val = (input[i] & 0xFF);
			snprintf(output + output_len, 4, "%%%2.2X", val);
			output_len += 3;
		} else {
			output[output_len++] = input[i];
		}
	}

	return sn_sv_from_parts(output, output_len);
}

char *
gcli_urlencode(char const *input)
{
	sn_sv encoded = gcli_urlencode_sv(SV((char *)input));
	return encoded.data;
}

char *
gcli_urldecode(struct gcli_ctx *ctx, char const *input)
{
	char *curlresult, *result;

	if (gcli_curl_ensure(ctx) < 0)
		return NULL;

	curlresult = curl_easy_unescape(ctx->curl, input, 0, NULL);
	if (!curlresult) {
		gcli_error(ctx, "could not urldecode");
		return NULL;
	}

	result = strdup(curlresult);

	curl_free(curlresult);

	return result;
}

/* Convenience function for fetching lists.
 *
 * listptr must be a double-pointer (pointer to a pointer to the start
 * of the array). e.g.
 *
 *    struct foolist { struct foo *foos; size_t foos_size; } *out = ...;
 *
 *    listptr = &out->foos;
 *    listsize = &out->foos_size;
 *
 * If max is -1 then everything will be fetched. */
int
gcli_fetch_list(struct gcli_ctx *ctx, char *url, struct gcli_fetch_list_ctx *fl)
{
	char *next_url = NULL;
	int rc;

	do {
		struct gcli_fetch_buffer buffer = {0};

		rc = gcli_fetch(ctx, url, &next_url, &buffer);
		if (rc == 0) {
			struct json_stream stream = {0};

			json_open_buffer(&stream, buffer.data, buffer.length);
			rc = fl->parse(ctx, &stream, fl->listp, fl->sizep);
			if (fl->filter)
				fl->filter(fl->listp, fl->sizep, fl->userdata);

			json_close(&stream);
		}

		gcli_fetch_buffer_free(&buffer);
		free(url);

		if (rc < 0)
			break;

	} while ((url = next_url) && (fl->max == -1 || (int)(*fl->sizep) < fl->max));

	free(next_url);

	return rc;
}

void
gcli_fetch_buffer_free(struct gcli_fetch_buffer *const buffer)
{
	if (!buffer)
		return;

	free(buffer->data);
	buffer->data = NULL;
	buffer->length = 0;
}
