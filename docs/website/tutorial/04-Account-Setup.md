# Setting up gcli for use with an account

Creating issues on GitHub requires an account which we need to
generate an authentication token for gcli.

Log into your GitHub account and click on your account icon in the top
right corner. Then choose the `Settings` option. Scroll down and
choose `Developer settings` on the bottom of the left column. Under
`Personal access tokens` choose `Tokens (classic)`.

Click on `Generate new token (classic)`.

Set a useful name such as `gcli` in the Note field, set the expiration
to `No expiration` and allow the following `Scopes`:

  - `repo`
  - `workflow`
  - `admin:public_key`
  - `gist`

Then create the token. It'll be printed in green. Do not share it!

Now we need to tell gcli about this new token. To do this, create
a configuration file for gcli - on Windows you need to do this from
the MSYS2 Shell:

    $ mkdir -p ${HOME}/.config/gcli
    $ vi ${HOME}/.config/gcli/config

Obviously, you can choose any other editor of your choice. Put the
following into this file:


    defaults {
        editor=vi
        github-default-account=my-github-account
    }

    my-github-account {
        token=<token-goes-here>
        account=<account-name>
        forge-type=github
    }

Replace the `<token-goes-here>` with the previously generated token
and the `<account>` with your account name.

If you now run

    $ gcli -t github repos

you should get a list of your repos. If not, check again that you did
all the steps above correctly.
