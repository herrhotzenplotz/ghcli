echo "installing dependencies"

dnf install -y https://download.copr.fedorainfracloud.org/results/patrickl/libcurl-gnutls/fedora-40-x86_64/07356121-libcurl-gnutls/libcurl-gnutls-8.6.0-8.fc40.x86_64.rpm
dnf install -y libcurl-devel bison flex gcc make pkg-config
dnf install -y openssl-devel
dnf install -y libedit-devel
dnf install -y readline-devel
dnf install -y lowdown
dnf install -y kyua
dnf install -y ccache
dnf install -y byacc
dnf install -y rpmdevtools rpmlint
dnf install -y libxml2-devel
dnf install -y tree

echo "config gcli"
rpmdev-setuptree
echo """
Name:           gcli
Version:        $VERSION
Release:        1%{?dist}
Summary:        gcli-rpm

License:        MIT
Source0:        %{name}-%{version}.tar.gz

%description
gcli rpm compilation.

%prep
%setup -q

%build
./configure
make %{?_smp_mflags}

%install
rm -rf \$RPM_BUILD_ROOT
make install DESTDIR=\$RPM_BUILD_ROOT

%files
/usr/local/bin/gcli
/usr/local/share/man/man1/gcli-api.1
/usr/local/share/man/man1/gcli-comment.1
/usr/local/share/man/man1/gcli-config.1
/usr/local/share/man/man1/gcli-forks.1
/usr/local/share/man/man1/gcli-gists.1
/usr/local/share/man/man1/gcli-issues.1
/usr/local/share/man/man1/gcli-labels.1
/usr/local/share/man/man1/gcli-milestones.1
/usr/local/share/man/man1/gcli-pipelines.1
/usr/local/share/man/man1/gcli-pulls-review.1
/usr/local/share/man/man1/gcli-pulls.1
/usr/local/share/man/man1/gcli-releases.1
/usr/local/share/man/man1/gcli-repos.1
/usr/local/share/man/man1/gcli-snippets.1
/usr/local/share/man/man1/gcli-status.1
/usr/local/share/man/man1/gcli.1
/usr/local/share/man/man5/gcli.5

%changelog
* Wed Aug 28 2024 herrhotzenplotz <email@example.com> - %{version}
- gcli package new version.
""" > ~/rpmbuild/SPECS/gcli.spec

curl "https://herrhotzenplotz.de/gcli/releases/gcli-$VERSION/gcli-2.5.0.tar.gz" --output "~/rpmbuild/SOURCES/gcli-$VERSION.tar.gz"

echo "build gcli"
rpmbuild -bb ~/rpmbuild/SPECS/gcli.spec

# the compiled rpm is in ~/rpmbuild/RPMS/x86_64/gcli-$VERSION-1.fc34.x86_64.rpm
