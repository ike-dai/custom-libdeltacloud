Summary: A library for accessing deltacloud
Name: libdeltacloud
Version: 0.3
Release: 1%{?dist}
License: LGPLv2+
Group: System Environment/Libraries
URL: http://people.redhat.com/clalance/libdeltacloud
Source0: http://people.redhat.com/clalance/libdeltacloud/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Obsoletes: libdcloudapi

%description
Libdeltacloud is a library for accessing deltacloud via a
convenient C API.

%package devel
Summary: Header files for libdeltacloud library
License: LGPLv2+
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
The libdeltacloud-devel package contains the files needed for developing
applications that need to use the libdeltacloud library.

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
rm -f $RPM_BUILD_ROOT/%{_lib}/libdeltacloud.la
rm -f $RPM_BUILD_ROOT/%{_lib}/libdeltacloud.a

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc COPYING
%attr(0755,root,root) /%{_lib}/libdeltacloud.so.*

%files devel
%defattr(-,root,root,-)
%attr(0644,root,root) %{_includedir}/libdeltacloud/libdeltacloud.h
%attr(0644,root,root) %{_includedir}/libdeltacloud/link.h
%attr(0644,root,root) %{_includedir}/libdeltacloud/instance.h
%attr(0644,root,root) %{_includedir}/libdeltacloud/realm.h
%attr(0644,root,root) %{_includedir}/libdeltacloud/image.h
%attr(0644,root,root) %{_includedir}/libdeltacloud/instance_state.h
%attr(0644,root,root) %{_includedir}/libdeltacloud/storage_volume.h
%attr(0644,root,root) %{_includedir}/libdeltacloud/storage_snapshot.h
%attr(0755,root,root) %{_includedir}/libdeltacloud/hardware_profile.h
%attr(0755,root,root) %{_libdir}/libdeltacloud.so
%{_libdir}/pkgconfig/libdeltacloud.pc

%changelog
* Thu Jul 08 2010 Chris Lalancette <clalance@redhat.com> 0.3-1
- Bump version for API breakage (replace - with _, move id to parent XML)
- Rename the library from dcloudapi to libdeltacloud
* Fri Apr 23 2010 Chris Lalancette <clalance@redhat.com> 0.2-1
- Bump version for new API (removed flavors, added hardware profiles)
* Mon Mar 08 2010 Chris Lalancette <clalance@redhat.com> 0.1-1
- Initial build.

