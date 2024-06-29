# Other forges and bugtrackers

gcli is capable of not only interacting with Github. It also currently supports:

- GitLab
- Gitea
- Bugzilla

## Bugzilla

### Notes

Bugzilla is commonly used as a bug tracker in various large open-source
projects such as FreeBSD, the Linux Kernel, Mozilla and Gentoo.

### Searching

Suppose you want to search for bug reports containing `sparc` in
the Gentoo Bugzilla.

In this case you need to configure an account that points at the
correct URL in `$HOME/.config/gcli/config` by adding:

	gentoo {
		forge-type=bugzilla
		apibase=https://bugs.gentoo.org/
	}

Now you can search the Gentoo Bugs:

	$ gcli -a gentoo issues sparc
	NUMBER  NOTES  STATE        TITLE
	924443      0  UNCONFIRMED  Add keyword ~sparc for app-misc/fastfetch
	924430      0  RESOLVED     media-libs/assimp-5.3.1 fails tests on sparc
	924215      0  CONFIRMED    dev-libs/libbson dev-libs/mongo-c-driver: alpha arm ia64 mips ppc ppc64 s390 sparc keyword req
	924191      0  CONFIRMED    media-libs/exempi: unaligned access causes dev-python/python-xmp-toolkit-2.0.2 to fails tests on sparc (test_file_to_dict (test.test_core_unit.UtilsTestCase.test_file_to_dict) ... Bus error)
	924180      0  CONFIRMED    dev-python/psycopg-3.1.17[native-extensions] fails tests on sparc: tests/test_copy_async.py::test_read_rows[asyncio-names-1] Fatal Python error: Bus error
	924031      0  IN_PROGRESS  sys-apps/bfs: ~arm ~arm64 ~ppc ~ppc64 ~sparc keywording
	923968      0  CONFIRMED    dev-python/pyarrow-15.0.0 fails to configure on sparc: CMake Error at cmake_modules/SetupCxxFlags.cmake:42 (message): Unknown system processor
	921245      0  CONFIRMED    media-video/rav1e-0.6.6 fails to compile on sparc: Assertion `DT.dominates(RHead, LHead) && "No dominance between recurrences used by one SCEV?"' failed.
	920956      0  CONFIRMED    dev-python/pygame-2.5.2: pygame.tests.font_test SIGBUS on sparc
	920737      0  CONFIRMED    sparc64-solaris Prefix no longer supported

	<snip>

### Issue details

Furthermore we can look at single issues:

	$ gcli -a gentoo issues -i 920737 all comments
	   NUMBER : 920737
	    TITLE : sparc64-solaris Prefix no longer supported
	  CREATED : 2023-12-26T19:20:58Z
	  PRODUCT : Gentoo Linux
	COMPONENT : Profiles
	   AUTHOR : Tom Williams
	    STATE : CONFIRMED
	   LABELS : none
	ASSIGNEES : prefix

	ORIGINAL POST

	    Resurrecting a Prefix install on Solaris 11.4 SPARC. It was working rather well
	    for me; after a hiatus I had hoped to use it again but my first emerge --sync
	    has removed the profile needed to merge any updates or new packages.

	    I note commit 8e006b67e06a19fae10c6059c7fc5ede88834601 in May 2023 removed the
	    profile and keywording for prefixed installs.

	    There is no associated comment. There doesn't seem to be a bug report in regards
	    to the change (I'm quite sure almost nobody uses it, so probably fair enough)

	    Any easy way to restore the profile for now? Eventually Solaris/SPARC and thus
	    Prefix will be gone anyway, but useful for now.

	    Thanks for your continued efforts.
	AUTHOR : sam
	DATE   : 2023-12-26T19:21:33Z
	         I think at the very least, when removing Prefix support in future, a 'deprecated'
	         file should be added to the relevant profiles asking if anyone is using
	         it to step forward.

	AUTHOR : grobian
	DATE   : 2023-12-26T22:58:12Z
	         Solaris 11.4 itself is a problem.  I doubt you ever had it "working".

	AUTHOR : grobian
	DATE   : 2023-12-26T22:59:57Z
	         Linux sparc team is not relevant here

	$

