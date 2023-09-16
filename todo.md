#+TITLE: gcli todos

** TODO Convert Github stuff to generated parsers [90%]
   - [X] Checks
   - [X] Comments
   - [X] Forks
   - [X] Issues
   - [X] Labels
   - [X] Pulls
	 - [X] Retrieve Pulls Info
	 - [X] Get PR merge message
   - [X] Releases
   - [X] Repos
   - [ ] Reviews
	 - [ ] This is GraphQL!...needs to be solved later
   - [X] Status

** DONE Convert Gitlab stuff to generated parers [100%]
   - [X] Pipelines
   - [X] Merge Requests
   - [X] Comments
   - [X] Issues
   - [X] Review
   - [X] Labels
   - [X] Releases
   - [X] Forks
   - [X] Repos
   - [X] Status

** DONE handle errors from the github api
   - [[file:src/curl.c::ghcli_fetch(const char *url, ghcli_fetch_buffer *out)][gcli_fetch]]
   - [[file:src/curl.c::ghcli_curl(FILE *stream, const char *url, const char *content_type)][gcli_curl]]
** TODO find a better way to pass the content type to gcli_curl
** DONE Man page
** DONE fetch PR comments
** DONE leaks of huge buffers [50%]
   We don't care about the little strings we sometimes malloc. But the
   huge buffers are annoying.
   - [X] Valgrind [100%]
	 - [X] issues
	 - [X] issues create
	 - [X] pulls
	 - [X] pulls create
	 - [X] comment
	 - [X] reviews
   - [ ] custom LD_PRELOAD hack
** DONE issues comments doesn't print the first comment
   it seems to reside in the issue data itself
   - solved by printing issue summaries
** TODO we don't handle pagination of the api results
   - [ ] See https://docs.github.com/en/rest/guides/getting-started-with-the-rest-api#pagination
** TODO speed up json_escape
** DONE repo-specific config file
** DONE comment under PR/Issue
** TODO make the use of C strings and string views more consistent
** DONE get rid of mktemp cuz binutils ld bitches about it
   #+begin_example
   src/editor.o: In function `gcli_editor_get_user_message':
   editor.c:(.text+0x108): warning: the use of `mktemp' is
		   dangerous, better use `mkstemp' or `mkdtemp'
   #+end_example
** DONE Check for multiple Github remotes and choose the right one
   Solved in commit 4be0ca8. Leave it up to the user to point at the
   right repo. Also, there are the -o and -r flags.
** DONE Valgrind the new fork stuff
** DONE Ask the user if they want to add a git remote if a fork is created
** DONE repos subcommand fails if -o is a user
** DONE Add docs for gists subcommand
** TODO add flags for sorting
   - [ ] gists
   - [ ] releases
** DONE Creating releases [100%]
   - [X] body
   - [X] choose a git tag
   - [X] attach files to release (aka assets)
   - [X] mark as prerelease or draft
** DONE pulls commit table header is weird
** DONE Check unnecessary includes
** DONE Valgrind again
** DONE write colors test for big-endian machines
** DONE Implement adding/removing labels from github prs
** TODO CI [83%]
   - [X] release resources properly
   - [X] check that we are connecting to github if we ever use the
	 =ci= subcommand [[file:src/gcli.c::if (gcli_config_get_forge_type() != GCLI_FORGE_GITHUB)][see here]]
   - [X] (maybe) integrate ci checks in status subcommand
   - [X] Split =status= and =summary= subcommands:
	 - =summary= should print header and commits
	 - =status= should print summary and checks
   - [X] overflow bug in id
   - [ ] dump logs I dunno whether i really want to implement
	 that. the problem is that github is misbehaving and doesn't give
	 me any association from the checks api to the actions api. maybe
	 I wanna add an actions subcommand that handles this very case for
	 github.
** DONE Unify Gitea and Github code

   Probably we want to make wrappers around the GitHub code for the
   cases where it works. For this to work, we need to mess with
   =github_get_apibase()= to return the right thing if we are looking
   at gitea.

** TODO Optimise =pad()= in [[file:src/table.c][file.c]]

** Label shit

   #+begin_example
   $ gcli labels
   <red>bug</red> - something is broken
   ...
   $ gcli labels create --description 'something is broken' --color FF0000 bug
   $ gcli labels delete bug
   #+end_example

   - for colors see [[https://github.com/git/git/blob/master/color.h][git implementation]]

* On the review API
  - A PR has got reviews (could be none, could be a thousand)
	+ https://api.github.com/repos/zorchenhimer/MovieNight/pulls/156/reviews
  - A review may have a body and comments attached to it
	+ https://api.github.com/repos/zorchenhimer/MovieNight/pulls/156/reviews/611653998
  - A review comment has got a diff hunk and a body attached to it.
	+ https://api.github.com/repos/zorchenhimer/MovieNight/pulls/156/reviews/611653998/comments

* Big refactor for libraryfication
** DONE Fix test suite
   - [X] Move config stuff to cmd and have callbacks that given the
     context give you the account details etc.

     This would allow you to create a mock context that returns only
     values that make sense in test contexts.

   - [X] Add support for a testing gcli context
** DONE Check errx calls if they print "error: "
** DONE Check for calls to errx in submit routines
** TODO [[file:src/github/review.c::github_review_get_reviews(char const *owner)][github_review_get_reviews]] is garbage
** TODO Clean up the generic comments code
** DONE Move printing routines to cmd code
** DONE Build a shared library
** TODO Remove global curl handle
   - [X] Put it into the context
   - [ ] Clean up handle on exit
** DONE Return errors from parsers
** DONE [[file:src/releases.c::gcli_release_push_asset(gcli_new_release *const release,][push_asset]] should not call errx but return an error code
** DONE Make [[file:src/pulls.c::gcli_pull_get_diff(gcli_ctx *ctx, FILE *stream, char const *owner,][getting a pull diff]] return an error code
** TODO Add a real changelog file
** DONE include [[file:docs/pgen.org][docs/pgen.org]] in the distribution tarball
** DONE remove gcli_print_html_url
** TODO Test suite
*** TODO Github [53%]
   - [ ] Checks
   - [X] Comments
   - [X] Forks
   - [ ] Gists
   - [X] Milestones
   - [X] Releases
   - [X] Repos
   - [ ] Reviews
   - [ ] SSH keys
   - [ ] Status
   - [X] Issues
   - [X] Labels
   - [X] Pulls
*** TODO Gitlab [61%]
   - [ ] Comments
   - [X] Forks
   - [X] Milestones
   - [X] Pipelines
   - [X] Releases
   - [X] Repos
   - [ ] Reviews
   - [ ] SSH Keys
   - [X] Snippets
   - [ ] Status
   - [X] Issues
   - [X] Labels
   - [X] Merge Requests
*** TODO Gitea
** TODO add an ATF macro for comparing string views
** TODO Think about making IDs =unsigned long=
** TODO get rid of html_url everywhere
** TODO Check the =Kyuafile.in= for requirements on test input
** https://keepachangelog.com/en/1.0.0/


# Diffs on Github

```diff
From 47b40f51cae6cec9a3132f888fd66c21ecb687fa Mon Sep 17 00:00:00 2001
From: Nico Sonack <nsonack@outlook.com>
Date: Sun, 10 Oct 2021 12:23:11 +0200
Subject: [PATCH] Start submission implementation

---
 include/ghcli/pulls.h |  1 +
 src/ghcli.c           | 55 +++++++++++++++++++++++++++++++++++++++++++
 src/pulls.c           |  9 +++++++
 3 files changed, 65 insertions(+)

diff --git a/include/ghcli/pulls.h b/include/ghcli/pulls.h
index 30a503cf..05d233eb 100644
--- a/include/ghcli/pulls.h
+++ b/include/ghcli/pulls.h
@@ -57,5 +57,6 @@ int  ghcli_get_prs(const char *org, const char *reponame, bool all, ghcli_pull *
 void ghcli_print_pr_table(FILE *stream, ghcli_pull *pulls, int pulls_size);
 void ghcli_print_pr_diff(FILE *stream, const char *org, const char *reponame, int pr_number);
 void ghcli_pr_summary(FILE *stream, const char *org, const char *reponame, int pr_number);

> This is a comment on line 60.
>
> This comment extends over multiple lines.
{
+void ghcli_pr_submit(const char *from, const char *to, int in_draft);
 
}
 #endif /* PULLS_H */
```


- [Gitlab Docs](https://docs.gitlab.com/ee/api/discussions.html#create-a-new-thread-in-the-merge-request-diff)
- [Github Docs](https://docs.github.com/en/rest/pulls/reviews?apiVersion=2022-11-28#create-a-review-for-a-pull-request)


