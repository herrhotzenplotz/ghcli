# Changelog

This changelog does not follow semantic versioning.

## UNRELEASED

### Added

- Added a `-R` flag to the comment subcommand that allows you to
  reply to a comment with the given ID.

### Fixed

- In various configuration places and environment variables where
  boolean values are accepted you can now specify `true` as a truthy
  value.

  Submitted by: Gavin-John Noonan <mail@gjnoonan.co.uk>

- The configure script now exits gracefully whenever a required
  program couldn't be found.

  Reported by: Alexey Ugnichev <alexey.ugnichev@gmail.com>

### Changed

- The pipelines subcommand has been refactored to accept actions
  for pipelines. This allows cases where a pipeline triggers child
  pipelines to be handled properly.

  See `gcli-pipelines(1)` for documentation.

  Reported by: Bence Ferdinandy <bence@ferdinandy.com>

### Removed

## 2.4.0 (2024-June-28)

### Added

- Added a convenience flag `-V` or `--version` that prints the
  version of gcli.

- Added an option to disable the spinner to indicate network activity

  You can now either set `disable-spinner=yes` in your default
  section in the config file, specify `--no-spinner` as a command
  line flag or set the environment variable `GCLI_NOSPINNER` to
  disable the spinner.  This is useful for dumb terminal environments
  like acme where the `\r` used by the spinner is not interpreted.
  Submitted by: Gavin-John Noonan

- Added an interactive status feature

  Running `gcli status` will now drop you into a prompt-based
  interactive TODO/notification menu. Currently this only supports
  issues however this will be extended in future releases.
  The old behaviour of `gcli status` showing a list of notifications
  can be achieved by supplying the `-l` or `--list` option.

  See also https://gitlab.com/herrhotzenplotz/gcli/-/issues/224

- Experimental support for code review has been added

  When enabling experimental features through either:

  - setting `GCLI_ENABLE_EXPERIMENTAL` in the environment to `yes`
  - setting `enable-experimental` in the default section of the
    gcli config file to yes

  you will get a action `review` on the pull request subcommand.

  This subcommand will pull down the diff of the given pull request
  and loads it into your editor. You can then annotate the diff,
  save and quit. gcli will then parse the diff, extract comments
  and allows you to submit them as a review.

  Please refer to the new `gcli-pulls-review(1)` manual page that
  has been added for this feature.

  Also note that this is experimental and may be buggy. However,
  testing is very much appreciated and I will gladly discuss ideas
  and usability issues.

  Please check in on IRC (#gcli on Libera.Chat) for these matters
  and/or post to our mailing list at
  https://lists.sr.ht/~herrhotzenplotz/gcli-discuss.

### Fixed

- Fixed resolving true .git directory when working inside a git
  worktree. This used to work before, however newer versions of git
  changed how these worktrees pointing to the true git directory.
  Reported by: Robert Clausecker

- Fixed bad error message extraction when API errors occured on
  GitLab. We now check for common error message fields in the
  returned payload from the GitLab API and choose the (probably)
  most useful one.

- Fixed segmentation fault when displaying certain pull requests
  on Gitea.

- Fixed some spelling mistakes in various manual pages

### Changed

- The GitLab API returns the list of comments in reverse chronological
  order. We now reverse this list to print the comments in the correct
  order such that a logical timeline of a conversation can be seen.

- The build system of gcli has been migrated away from Autotools.
  The GNU autotools have been complicating and slowing down the
  gcli builds by a huge amount. Using autotools prevented easy
  cross-compilation of gcli because it would have required the use
  of GNU make. Instead of autotools we now use a hand-written
  configure script and a hand-written Makefile that gets pre-processed
  by the configure script. You can still invoke the configure script
  as expected however some options aren't available anymore.

  Packagers are likely interested in the `--release` flag of the
  configure script. Most other compilation environment flags
  are controlled by environment variables. Please refer to
  `./configure --help` for more information. The `HACKING.md` file
  also includes valuable information including examples.

  Out-of-tree builds are still fully supported.

  If you encounter bugs / other difficulties due to these changes
  please report back!

## 2.3.0 (2024-Mar-25)

### Added

- It is now possible to build gcli against libgcli as a DLL on cygwin.
  Submitted by: Daisuke Fujimura

- The pulls subcommand now allows searching for pull requests with
  a given search term. The search terms can be appended to the
  regular pull subcommand for listing PRs:

  ```console
  $ gcli pulls -L bug segmentation fault
  ```

  The above will search for pull requests containing »segmentation
  fault« and the label »bug«.

- An interactive mode for creating both PRs and issues has been added.
  You can now interactively create pull requests and issues by omitting their title:

  ```console
  $ gcli issues create
  Owner [herrhotzenplotz]:
  Repository [gcli]:
  Title: foo
  The following issue will be created:

  TITLE   : foo
  OWNER   : herrhotzenplotz
  REPO    : gcli
  MESSAGE :
  No message

  Do you want to continue? [yN]
  ```

### Fixed

- gcli was incorrectly using an environment variable *XDG_CONFIG_DIR*.
  This variable has now been fixed to be *XDG_CONFIG_HOME*.
  Submitted by: Jakub Wilk

- Fixed a segmentation fault when listing forks

- Fixed error when submitting a comment on Gitlab issues

- The build on Haiku has been fixed. GCLI can now be compiled and
  used on this platform.

## 2.2.0 (2024-Feb-05)

### Added

- Preliminary (and thus experimental) support for Bugzilla has been
  added. For this a new yet undocumented `attachments` subcommand
  has been introduced.
  Currently if no account has been specified it will default to the
  FreeBSD Bugzilla - this may however change in the future.

- A search feature has been added to the issues subcommand. You can
  now optionally provide trailing text to the issues subcommand
  which will be used as a search term:

  ```console
  $ gcli issues -A herrhotzenplotz Segfault
  ```

  This will search for tickets authored by herrhotzenplotz containing
  "Segfault".

- Added partial support for auto-merge. When creating a pull request
  on Gitlab and Github you can set an automerge flag. Whenever this
  automerge flag is set a pull request will be merged once all the
  pipelines/checks on the pull request pass.

  This feature is not fully documented yet as there are bugs in it,
  especially on Gitlab there are flaws. Please consider this feature
  unstable and experimental.

### Fixed

- Fixed a segmentation fault when getting a 404 on Gitlab. This bug
  occured on Debian Linux when querying pipelines at the KiCad project.
  The returned 404 contained unparsable data which then lead to the
  error message to be improperly initialised.
  Reported by: Simon Richter

- Fixed missing URL-encode calls in Gitlab Pipelines causing 404 errors
  when using subprojects on Gitlab. You're now not forced anymore
  to manually urlencode slashes as %2F in the repos.
  Reported by: Simon Richter

- Fixed the patch generator for Gitlab Merge Requests to produce
  patches that can be applied with `git am`.
  Previously the patches were invalid when new files were created
  or deleted.

- Fixed Segmentation fault when the editor was opened and closed
  without changing the file. Several subcommands have been updated
  to also account for empty user messages.

- Fixed incorrect colour when creating labels. In any forge the
  provided colour code was converted incorrectly and always producing
  the wrong colour.

- Fixed a segmentation fault when listing Github gists

- Fixed possible JSON escape bug when creating a Github Gist

- Fixed gcli reporting incorrect libcurl version in the User-Agent
  header when performing HTTP requests.

- Fixed possible segmentation fault when no token was configured in
  gcli configuration file.

### Changed

- Internally a lot of code was using string views. Maintaining this
  was a bit cumbersome and required frequent reallocations.
  A lot of these uses have been refactored to use plain C-Strings
  now. This also involved changing some code to use the new
  `gcli_jsongen` set of routines.
  Due to these changes there may be regressions that are only visible
  during use. If you encounter such regressions where previously
  working commands suddenly fail due to malformed requests please
  report immediately.

### Removed

## 2.1.0 (2023-Dec-08)

### Added

- Added a little spinner to indicate network activity
- Added Windows 10 MSYS2 to list of confirmed-to-work platforms
- Added a new action `set-visibility` to the repos subcommand that
  allows updating the visibility level of a repository.
- Added a new action `request-review` to the pulls subcommand that
  allows requesting a review of a pull request from a given user.
- One can now define custom aliases in the alias section of the
  config file. Aliases are very primitive as of now. This means they
  are just different names for subcommands. Aliases may reference
  other aliases.
- Added a new `-M` flag to both the pulls and the issues subcommand
  to allow filtering by milestones.
- Added a new `patch` action to the pulls subcommand. This allows
  you to print the entire patch series for a given pull request.
  Also added the missing implementations for this feature for Github
  and Gitea.
- Added a new `title` action to both the issues and the pulls
  subcommand that allows updating their titles.

### Fixed

- Fixed incorrect internal help message of the `repos` subcommand.
- Worked around ICE with xlC 16 on ppc64le Debian Linux, gcli now
  compiles using xlC and works too.
- Fixed various memory leaks.
- Spelling fixes in manual pages (submitted by Jakub Wilk
  https://github.com/herrhotzenplotz/gcli/pull/121)
- Wired up alread existing implementation for forking on Gitea to
  gcli command.
- The `status` subcommand now works properly on Gitea.

### Changed

- Subcommands can now be abbreviated by providing an unambiguous
  prefix that matches the subcommand.

### Removed


## 2.0.0 (2023-Sep-21)

### Added

- This changelog has been added
- gcli is now built as a shared or static library which the gcli tool links against
  This implied so many changes that the major version number was bumped.
- Added a package-config file for libgcli
- Added a `-L` flag to the `issues` and `pulls` subcommand to allow
  filtering by label
- A work-in-progress tutorial has been added and is available at
  [the GCLI directory](https://herrhotzenplotz.de/gcli/tutorial) on
  my website.
- Gitlab jobs now show coverage information

### Fixed

- Parallel builds in autotools have been re-enabled
- Improved error messages in various places
- Bad roff syntax in manual pages has been fixed

### Changed

- the `gcli pulls create` subcommand does not print the URL to the
  created release anymore.
- The test suite is now using [atf-c](https://github.com/jmmv/atf)
  and [kyua](https://github.com/jmmv/kyua). These are dependencies
  if you want to run the tests. These tools are installed out of
  the box on most BSDs.
- A newly introduced dependency is the `sys/queue.h` header. On
  GNU/Linux systems you might need to install it as part of libbsd.

### Removed

- The reviews subcommand has been removed because it was generally useless
  This feature will be reimplemented as a WIP of
  [#189](https://gitlab.com/herrhotzenplotz/gcli/-/issues/189)
