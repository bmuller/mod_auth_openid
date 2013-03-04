Summary: The OpenID module for Apache
Name: mod_auth_openid
Version: 0.6
Release: 1
License: GPL
Group: System Environment/Daemons
URL: https://github.com/bmuller/mod_auth_openid
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root
BuildRequires: sqlite-devel httpd-devel pcre-devel libopkele-devel

%description
mod_auth_openid is an authentication module for the Apache 2 webserver.
It handles the functions of an OpenID consumer as specified in the
OpenID 2.0 specification.

%prep
%setup -q

%build
if [ ! -x ./configure ]; then
	./autogen.sh
fi
%configure

make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%makeinstall

%clean
rm -rf %{buildroot}

%files
%{_libdir}/httpd/modules/mod_auth_openid.so
