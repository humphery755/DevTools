# electron-quick-start

cnpm install -g node-gyp
cnpm install --global --production windows-build-tools
cnpm config set python python2.7
cnpm config set msvs_version 2017

# Go into the repository
cd electron-quick-start
# Install dependencies
cnpm install

cd node_modules/ffi
node-gyp rebuild -target="1.8.4" -arch=x64 -dist-url="https://atom.io/download/atom-shell"

cd node_modules/ref
node-gyp rebuild -target="1.8.4" -arch=x64 -dist-url="https://atom.io/download/atom-shell"

# set vcvars64.bat
vi compile.bat
# make dll
# run compile.bat
nmake

# Run the app
cnpm start
