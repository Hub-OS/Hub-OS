# Installing CMake command line tools for MacOSX
Run the following commands in order

```bash
curl https://cmake.org/files/v3.8/cmake-3.8.1-Darwin-x86_64.tar.gz -O
tar -xzf cmake-3.8.1-Darwin-x86_64.tar.gz
sudo mv cmake-3.8.1-Darwin-x86_64/CMake.app /Applications
sudo /Applications/CMake.app/Contents/bin/cmake-gui --install
```

You should now be able to use cmake and follow the rest of the setup.
