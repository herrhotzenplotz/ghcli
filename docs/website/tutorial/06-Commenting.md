# Commenting

Discussions on GitHub and the like are done through comments. You can
comment on issues and pull requests.

## Reviewing a discussion

Say you were looking at an issue in curl/curl:

    $ gcli issues -o curl -r curl -i 11461 comments

## Create the comment

And now you wish to respond to this thread:

    $ gcli comment -o curl -r curl -i 11461

This will now open the editor and lets you type in your message.
After saving and exiting gcli will ask you to confirm. Type 'y' and
hit enter:

    $ gcli -t github comment -o curl -r curl -i 11461
    You will be commenting the following in curl/curl #11461:

    Is this okay? [yN] y
    $

## Commenting on pull requests

When you want to comment under a pull request use the `-p` flag
instead of the `-i` flag to indicate the PR number.
