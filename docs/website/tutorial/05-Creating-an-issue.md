# Creating an issue

**Note**: This assumes you have [configured gcli with an account](./04-Account-Setup.html) for GitHub already.

## Preparation

For this case I have a playground repository that you may as well use
for testing with gcli. It is available at
`herrhotzenplotz/ghcli-playground`.

To see a list of issues, we can run:

    $ gcli -t github issues -o herrhotzenplotz -r ghcli-playground -a
    NUMBER  NOTES  STATE   TITLE
        13      0  open    yet another issue
        12      0  closed  wat
        11      0  closed  blaaaaaaaaaaaaaaaaaaaah
        10      0  closed  "this is the quoted" issue title? anyone?"
         9      0  closed  test
         8      0  closed  foobar
         7      0  closed  foobar
         5      0  closed  test2
         4      0  closed  test
    $

## Invoke gcli
Let's create a bug report where we complain about things not working:

    $ gcli -t github issues create -o herrhotzenplotz -r ghcli-playground \
        "Bug: Doesn't work on my machine"

The message "Bug: doesn't work on my machine" is the title of the
issue.

## Original Post

You will see the default editor come up and instruct you to type in a
message. This message is the "original post" or the body of the issue
ticket that you're about to submit. You can use Markdown Syntax:

     I tried building this code on my machine but unfortunately it errors
     out with the following message:

     ```console
     $ make love
     make: don't know how to make love. Stop

     make: stopped in /tmp/wat
     $
     ```

     What am I doing wrong?

     ! ISSUE TITLE : Bug: Doesn't work on my machine
     ! Enter issue description above.
     ! All lines starting with '!' will be discarded.

## Submit the issue

After you save and exit the editor gcli gives you a chance to check
back and finally submit the issue. Type 'y' and hit enter.

You can check back if the issue was created and also view details
about it as you learned earlier.
