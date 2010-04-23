Summary: A library for accessing deltacloud
Name: libdcloudapi
Version: 0.2
Release: 1%{?dist}
License: LGPLv2+
Group: System Environment/Libraries
URL: http://people.redhat.com/clalance/dcloudapi
Source0: http://people.redhat.com/clalance/dcloudapi/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
Libdcloudapi is a library for accessing deltacloud via a
convenient C API.

%package devel
Summary: Header files for libdcloudapi library
License: LGPLv2+
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
The libdcloudapi-devel package contains the files needed for developing
applications that need to use the libdcloudapi library.

%prep
%setup -q

%build
%configure --libdir=/%{_lib}
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR="${RPM_BUILD_ROOT}" install

# Move the symlink
rm -f $RPM_BUILD_ROOT/%{_lib}/%{name}.so
mkdir -p $RPM_BUILD_ROOT%{_libdir}
VLIBNAME=$(ls $RPM_BUILD_ROOT/%{_lib}/%{name}.so.*.*.*)
LIBNAME=$(basename $VLIBNAME)
ln -s ../../%{_lib}/$LIBNAME $RPM_BUILD_ROOT%{_libdir}/%{name}.so

# Move the pkgconfig file
mv $RPM_BUILD_ROOT/%{_lib}/pkgconfig $RPM_BUILD_ROOT%{_libdir}

# Remove a couple things so they don't get picked up
rm -f $RPM_BUILD_ROOT/%{_lib}/libdcloudapi.la
rm -f $RPM_BUILD_ROOT/%{_lib}/libdcloudapi.a

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc COPYING
%attr(0755,root,root) /%{_lib}/libdcloudapi.so.*

%files devel
%defattr(-,root,root,-)
#% attr(0644,root,root) %{_mandir}/man3/*
%attr(0644,root,root) %{_includedir}/dcloudapi/dcloudapi.h
%attr(0644,root,root) %{_includedir}/dcloudapi/link.h
%attr(0644,root,root) %{_includedir}/dcloudapi/instance.h
%attr(0644,root,root) %{_includedir}/dcloudapi/realm.h
%attr(0644,root,root) %{_includedir}/dcloudapi/image.h
%attr(0644,root,root) %{_includedir}/dcloudapi/instance_state.h
%attr(0644,root,root) %{_includedir}/dcloudapi/storage_volume.h
%attr(0644,root,root) %{_includedir}/dcloudapi/storage_snapshot.h
%attr(0755,root,root) %{_libdir}/libdcloudapi.so
#% attr(0644,root,root) %{_datadir}/aclocal/cap-ng.m4
%{_libdir}/pkgconfig/libdcloudapi.pc

%changelog
* Fri Apr 23 2010 Chris Lalancette <clalance@redhat.com> 0.2-1
- Bump version for new API (removed flavors)
* Mon Mar 08 2010 Chris Lalancette <clalance@redhat.com> 0.1-1
- Initial build.

