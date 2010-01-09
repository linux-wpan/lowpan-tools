# lowpan-tools.spec
#
# Copyright (c) 2009 Dmitry Eremin-Solenikov <dbaryshkov@gmail.com>
#
%define name lowpan-tools
%define version 0.1+0.2rc4
%define srcver 0.2-rc4
%define release 1

# required items
Name: %{name}
Version: %{version}
Release: %{release}
License: GPL
Group: Applications/System

# optional items
#Vendor: Dmitry Eremin-Solenikov
#Distribution:
#Icon:
URL: http://linux-zigbee.sf.net/
Packager: Dmitry Eremin-Solenikov <dbaryshkov@gmail.com>

# source + patches
Source: %{name}-%{srcver}.tar.gz
#Source1:
#Patch:
#Patch1:

# RPM info
#Provides:
#Requires:
#Conflicts:
#Prereq:
BuildRequires: libnl-devel, python

Prefix: /usr
#BuildRoot: /var/tmp/%{name}-%{srcver}

Summary: Base programs for LoWPAN in Linux

%description
Base programs for LoWPAN in Linux

This package provides all basic prorgrams needed for setting up, monitoring
and tuning LoWPAN networks.

Notice that upstream still flags these tools as experimental software and
says that there is still a number of known bugs and issues. The
software is, however, in productive use at a number of sites and is
working reliably.

%package tests
Summary: Testing utilities for LoWPAN in Linux
Group: Development/Tools
%description tests
Testing utilities for LoWPAN in Linux

This package provides several programs for testing various aspects of
Linux IEEE 802.15.4 stack.

Normal users don't need to install this package.

%package devel
Summary: Include files and examples for writing programming for LoWPAN
Group: Development/Libraries

%description devel
Include files and examples for writing programming for LoWPAN

This package contains several include files to help in development of new
programs for LoWPAN.

%prep
%setup -q -n %{name}-%{srcver}
#%patch0 -p1

%build
%configure
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/var/lib/%{name}
%makeinstall

#%pre
#%post
#%preun
#%postun

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/bin
/usr/sbin
/usr/lib/ip
/usr/share/man
%doc README NEWS AUTHORS
#%docdir
%dir /var/lib/lowpan-tools
#%config

%files tests
/usr/lib*/%{name}
/usr/lib/python?.?

%files devel
/usr/include

%changelog
