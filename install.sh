# !/bin/bash
mkdir -p build
cd build
if [ -d "/usr/local/include/dzIPC" ] || [ -d "/usr/local/lib/ipc_msg" ]||[ -d "/usr/local/lib/ipc_srv" ]||[ -d "/usr/local/lib/libipc" ]; then
    sudo make uninstall
fi
sudo cmake .. -DCMAKE_BUILD_TYPE=Release
sudo make -j10
sudo make install
echo "Message and Service updated"