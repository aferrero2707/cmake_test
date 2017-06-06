
# VIPS
wget http://www.vips.ecs.soton.ac.uk/supported/8.4/vips-8.4.5.tar.gz
if [ $? -ne 0 ]; then exit 1; fi

tar xzvf vips-8.4.5.tar.gz
if [ $? -ne 0 ]; then exit 1; fi

cd vips-8.4.5 && $HOME/inst/bin/crossroad configure --disable-gtk-doc --disable-gtk-doc-html --disable-introspection --enable-debug=yes --without-python --without-magick --without-libwebp && make -j 3 && make install && cd ..
if [ $? -ne 0 ]; then exit 1; fi


$HOME/inst/bin/crossroad cmake -DCMAKE_BUILD_TYPE=Release .. && make VERBOSE=1 && make install
if [ $? -ne 0 ]; then exit 1; fi

exit 0
