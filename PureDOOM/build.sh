# build PureDOOM c object
clang -mmacosx-version-min=10.9 -std=c99 -Wno-deprecated-non-prototype -Wno-parentheses -g -c PureDOOM_impl.c

# build chugin object
# clang++ -mmacosx-version-min=10.9 -I../chuck/include/  -fPIC -O3 -c -o PureDOOM.o PureDOOM.cpp
clang++ -mmacosx-version-min=10.9 -I../chuck/include/  -fPIC -g -c -o PureDOOM.o PureDOOM.cpp

# link
clang++ -mmacosx-version-min=10.9 -shared -lc++  -o PureDOOM.chug PureDOOM.o PureDOOM_impl.o

# install
mkdir -p /usr/local/lib/chuck
cp PureDOOM.chug /usr/local/lib/chuck
chmod 755 /usr/local/lib/chuck/PureDOOM.chug
