#! /bin/bash

for counter in $(seq 1 10); do
echo "try $counter"
$HOME/inst/bin/crossroad install libtiff5 libtiff-devel libpng16-16 libpng-devel libjpeg8 libjpeg8-devel libgtkmm-2_4-1 gtkmm2-devel liborc-0_4-0 liborc-devel libexiv2 libexiv2-devel liblcms2-2 liblcms2-devel libxml2-2 libxml2-devel libxml2-tools libfftw3-3 fftw3-devel libexif12 libexif-devel  
#$HOME/inst/bin/crossroad install libxml2-devel libxml2-tools
if [ $? -eq 0 -o $counter -eq 10 ]; then 
	break
fi
done

if [ $? -ne 0 ]; then 
	exit 1
fi
exit 0
