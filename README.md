# REMA Proxy

An interface for a Cartesian XYZ robot for Heat Exchangers Inspection

## Requirements

Restbed library should be located in `/usr/local/include/` all the headers from `restbed/distribution/include`
                                     `/usr/local/lib/` the shared library from `restbed/distribution/librestbed.a`

### Installing

It is fairly easy to install the project, all you need to do is clone if from
[GitHub](https://github.com/gustavojm/rema_proxy)

```bash
git clone https://github.com/GMalano-NASA/rema_proxy
```

After finishing getting a copy of the project...

## Building the project

To build the project, all you need to do

```bash
cmake -S . -B ./build/
cmake --build ./build/
```

Change project settings, specify CMAKE_INSTALL_PREFIX 
```bash
ccmake ./build
```

```bash
cmake --build ./build/ --target install
```

Run the installed project  
```bash
cd ~/REMA_Proxy
./REMA_Proxy
```

Open a web browser 

http://127.0.0.1:4321/static/index.html#


## Generating the documentation

In order to generate documentation for the project, you need to configure the build
to use Doxygen. This is easily done, by modifying the workflow shown above as follows:

```bash
ccmake ./build
cmake --build . --target doxygen-docs
```

> ***Note:*** *This will generate a `docs/` directory in the **project's root directory**.*
