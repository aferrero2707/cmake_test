#!/bin/bash

# transfer.sh
transfer() 
{ 
	if [ $# -eq 0 ]; then 
		echo "No arguments specified. Usage:\necho transfer /tmp/test.md\ncat /tmp/test.md | transfer test.md"; 		
		return 1; 
	fi
	tmpfile=$( mktemp -t transferXXX ); 
	if tty -s; then 
		basefile=$(basename "$1" | sed -e 's/[^a-zA-Z0-9._-]/-/g'); 
		curl --progress-bar --upload-file "$1" "https://transfer.sh/$basefile" >> $tmpfile; 
	else 
		curl --progress-bar --upload-file "-" "https://transfer.sh/$1" >> $tmpfile ; 
	fi; 
	cat $tmpfile; 
	rm -f $tmpfile; 
}

package_name=vipstest
arch=$1
package_version="${arch}-$(date +%Y%m%d)"
#package_version=0.2.7
#package_version=$(cat checkout/PhotoFlow/VERSION | head -n 1)


# stuff is in here
basedir=`pwd`
 
# download zips to here
packagedir=packages

# unzip to here
installdir=$HOME/.local/share/crossroad/roads/w64/vipstest-build

# jhbuild will download sources to here 
checkoutdir=source

if [ x"$arch" = "xw32" ]; then
	mingw_prefix=i686-w64-mingw32
fi
if [ x"$arch" = "xw64" ]; then
	mingw_prefix=x86_64-w64-mingw32
fi

repackagedir=$package_name-$package_version

ls -l $installdir/bin


echo copying install area $installdir

rm -rf $repackagedir
cp -r $installdir $repackagedir
rm -rf $repackagedir/bin
rm -rf $repackagedir/wine
mkdir $repackagedir/bin
cp -L $installdir/bin/* $repackagedir/bin
cp -L $installdir/lib/*.dll $repackagedir/bin

echo cleaning build $repackagedir

if [ ! -e $repackagedir/bin ]; then echo "$repackagedir/bin not found."; exit; fi
if [ ! -e $repackagedir/lib ]; then echo "$repackagedir/lib not found."; exit; fi

if [ x"$arch" = "xw32" ]; then
	(cd $repackagedir/bin; wget ftp://ftp.equation.com/gdb/32/gdb.exe)
fi
if [ x"$arch" = "xw64" ]; then
	(cd $repackagedir/bin; wget ftp://ftp.equation.com/gdb/64/gdb.exe)
fi
echo "Before cleaning $repackagedir/bin"
#read dummy

( cd $repackagedir/bin ; mkdir poop ; mv *vipstest* gdb.exe phf_stack.exe gdk-pixbuf-query-loaders.exe update-mime-database.exe camconst.json gmic_def.gmic poop ; mv *.dll poop ; rm -f * ; mv poop/* . ; rmdir poop )

( cd $repackagedir/bin ; rm -f libvipsCC-15.dll run-nip2.sh *-vc100-*.dll *-vc80-*.dll *-vc90-*.dll  )

#( cd $repackagedir/bin ; strip --strip-unneeded *.exe )

# for some reason we can't strip zlib1
( cd $repackagedir/bin ; mkdir poop ; mv zlib1.dll poop ; strip --strip-unneeded *.dll ; mv poop/zlib1.dll . ; rmdir poop )


( cd $repackagedir/share ; rm -rf aclocal applications doc glib-2.0 gtk-2.0 gtk-doc ImageMagick-* info jhbuild man mime pixmaps xml goffice locale icons)

( cd $repackagedir ; rm -rf include )

# we need some lib stuff at runtime for goffice and the theme
( cd $repackagedir/lib ; mkdir ../poop ; mv goffice gtk-2.0 gdk-pixbuf-2.0 ../poop ; rm -rf * ; mv ../poop/* . ; rmdir ../poop )

# we don't need a lot of it though
( cd $repackagedir/lib/gtk-2.0 ; find . -name "*.la" -exec rm {} \; )
( cd $repackagedir/lib/gtk-2.0 ; find . -name "*.a" -exec rm {} \; )
( cd $repackagedir/lib/gtk-2.0 ; find . -name "*.h" -exec rm {} \; )

( cd $repackagedir ; rm -rf make )

( cd $repackagedir ; rm -rf man )

( cd $repackagedir ; rm -rf manifest )

( cd $repackagedir ; rm -rf src )

# we need to copy the C++ runtime dlls in there
gccmingwlibdir=/usr/lib/gcc/$mingw_prefix/4.8
mingwlibdir=/usr/$mingw_prefix/lib
cp -L $gccmingwlibdir/*.dll $repackagedir/bin
cp -L $mingwlibdir/*.dll $repackagedir/bin

#exit

#echo creating $package_name-$package_version.zip
#rm -f $package_name-$package_version.zip
#zip -r -qq $package_name-$package_version.zip $package_name-$package_version

rm -f $package_name-$package_version.zip
zip -r $package_name-$package_version.zip $package_name-$package_version

transfer $package_name-$package_version.zip

exit

# have to make in a subdir to make sure makensis does not grab other stuff
echo building installer nsis/$package_name-$package_version-setup.exe
( cd nsis ; rm -rf $package_name-$package_version ; 
#unzip -qq -o ../$package_name-$package_version.zip ;
rm -rf $package_name-$package_version
mv ../$package_name-$package_version .
#makensis -DVERSION=$package_version $package_name.nsi > makensis.log 
)
cd nsis
rm -f $package_name-$package_version.zip
zip -r $package_name-$package_version.zip $package_name-$package_version
rm -rf $package_name-$package_version
rm -f $package_name-$package_version-setup.zip
#zip $package_name-$package_version-setup.zip $package_name-$package_version-setup.exe
