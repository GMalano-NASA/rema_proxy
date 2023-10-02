# REMA Proxy

An interface for a Cartesian XYZ robot for Heat Exchangers Inspection


### Installing

It is fairly easy to install the project, all you need to do is clone if from
[GitHub](https://github.com/gustavojm/rema_proxy)

```bash
git clone https://github.com/gustavojm/rema_proxy
```

After finishing getting a copy of the project...

## Building the project

To build the project, all you need to do

```bash
mkdir build
cmake ./build
cmake --build ./build/
```

```bash
cmake --build ./build --target install --config Release

# a more general syntax for that command is:
cmake --build <build_directory> --target install --config <desired_config>
```
## Generating the documentation

In order to generate documentation for the project, you need to configure the build
to use Doxygen. This is easily done, by modifying the workflow shown above as follows:

```bash
ccmake .
cmake --build . --target doxygen-docs
```

> ***Note:*** *This will generate a `docs/` directory in the **project's root directory**.*
