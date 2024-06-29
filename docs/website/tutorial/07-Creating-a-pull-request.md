# Creating a pull request

Creating a pull request with gcli is usually as simple as running

	$ gcli pulls create

## Preparation

Suppose you have a git repository forked and cloned:

	$ git clone git@github.com:contour-terminal/contour
	$ cd contour
	$ gcli forks create --into herrhotzenplotz

Then you do some work on whatever feature you're planning to submit:

	$ git checkout -b my-amazing-feature
	<hackhackhack>
	$ git add -p
	$ git commit

## Push your changes

You then push your changes to your fork:

	$ git push origin my-amazing-feature

## Create the pull request

Now you can run the command to create the pull request:

	$ gcli pulls create
	From (owner:branch) [herrhotzenplotz:my-amazing-feature]:
	Owner [contour-terminal]:
	Repository [contour]:
	To Branch [master]:
	Title: Add new amazing feature
	Enable automerge? [yN]:

Most of the defaults you should be able to simply accept. This
assumes that you have the source branch checked out locally and
remotes are configured appropriately.

Otherwise you can just change the defaults by entering the correct
values.

## Enter original post

After you entered all the meta data of the pull request gcli will
drop you into your editor and lets you enter a message for the pull
request.

## Submit

After you saved and exit type `y` and hit enter to submit the pull
request.
