# First steps

## Listing issues

Let's start off by listing some issues - here for the curl project
which is hosted on GitHub under curl/curl. To list issues for it one
would run:

    $ gcli -t github issues -o curl -r curl

You will see the list of the 30 most recent open issue tickets. The
command above does the following:

  - invoke gcli
  - as a global option we switch it into GitHub-Mode
  - invoke the issues subcommand
  - operate on the repository owner curl (`-o curl`)
  - operate on the repository curl (`-r curl`)

Note that the `-t github` option goes before the issues subcommand
because it is a global option for gcli that affects how all the
following things like subcommands operate.

However, now I also want to see closed issues:

    $ gcli -t github issues -o curl -r curl -a

The `-a` option will disregard the status of the issue.

Oh and the screen is a bit cluttered by all these tickets - let's only
fetch the first 10 issues:

    $ gcli -t github issues -o curl -r curl -n10

## Examining issues

As of now we only produced lists of issues. However, we may also want
to look at the details of an issue such as:

  - the original post
  - labels
  - comments
  - assignees of the issue (that is someone who is working on the bug)

Let's get a good summary of issue `#11268` in the curl project:

    $ gcli -t github issues -o curl -r curl -i 11268 all

As you can see most of the options are the same, however now we tell
gcli with the `-i 11268` option that we want to work with a single
issue. Then we tell gcli what actions to perform on the issue. Another
important action is `comments`. Guess what it does:

    $ gcli -t github issues -o curl -r curl -i 11268 comments

I know a person that likes to post long verbose traces. Let's search
for an issue authored by them on the OpenSSL GitHub page:

    $ gcli -t github issues -o openssl -r openssl -A blastwave -a
    NUMBER  STATE   TITLE
     20379  open    test "80-test_ssl_new.t" fails on Solaris 10 SPARCv9
     10547  open    Strict C90 CFLAGS results in sha.h:91 ISO C90 does not support long long
      8048  closed  OPENSSL_strnlen SIGSEGV in o_str.c line 76
    $

The `-A` option lets you filter for specific authors.

Let's look at the issue state of `#10547`:

    $ gcli -t github issues -o openssl -r openssl -i 10547 status
         NAME : 10547
        TITLE : Strict C90 CFLAGS results in sha.h:91 ISO C90 does not support long long
      CREATED : 2019-12-01T04:35:23Z
       AUTHOR : blastwave
        STATE : open
     COMMENTS : 9
       LOCKED : no
       LABELS : triaged: bug
    ASSIGNEES : none
    $

That's nine comments - let's read the original post and the comments
in our favourite pager `less`:

    $ gcli -t github issues -o openssl -r openssl -i 10547 op comments | less

As you can see gcli will accept multiple actions for an issue and
executes them sequentially.
