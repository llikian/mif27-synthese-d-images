# mif27-synthese-d-images
## Project Description


## Setup
### Dependencies
On a Debian system you can install the needed dependencies using:
```shell
sudo apt install g++ cmake make
```

### Clone Repository
```shell
git clone https://github.com/llikian/mif27-synthese-d-images
cd mif27-synthese-d-images
```

### Build
To build you can simply run the `release.sh` script at the root of the project using:
```shell
bash release.sh
```

You can also manually build it using:
```shell
cmake -B build && \
cmake --build build -j
```

Then you can run it using:
```shell
bin/mif27-synthese-d-images
```

## Credits
