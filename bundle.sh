rm -rf mmfm.app
mkdir mmfm.app
mkdir -p mmfm.app/bin
mkdir -p mmfm.app/lib
mkdir -p mmfm.app/res
ldd build/mmfm | awk '{print $3}' | xargs -I {} cp {} mmfm.app/lib/
rm mmfm.app/lib/libc.so.*
rm mmfm.app/lib/libm.so.*
cp -R build/mmfm mmfm.app/bin/
cp -R res/html mmfm.app/res/
cp -R res/img mmfm.app/res/
cp res/mmfm.desktop mmfm.app/
cp res/mmfm.png mmfm.app/
echo 'PTH=$(dirname $(readlink -f $0))' > mmfm.app/mmfm
echo "LD_LIBRARY_PATH=\$PTH/lib \$PTH/bin/mmfm -r \$PTH/res" >> mmfm.app/mmfm
chmod +x mmfm.app/mmfm
tar czf mmfm.app.tar.gz mmfm.app
