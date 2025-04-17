## Building Boost with Boost.Python
### Ubuntu (or Debian-based systems)
On Ubuntu, Boost's build system can usually detect the system-installed Python automatically.  
You do **not** need a `user-config.jam`.
Make sure the necessary development packages are installed:

```bash
sudo apt update
sudo apt install python3-dev
./bootstrap.sh --prefix="$HOME/work/software/boost_1_88_0" --includedir=headers --libdir=dist --with-python=python3
./b2 --clean-all
./b2 install 
```
### Homebrew (macOS)
Boost.Python requires a `user-config.jam` file to build correctly when using Python installed via Homebrew.
See `FlightOps/Documents/boost-user-config-homebrew.jam` for an example.

```bash
cp -a $(GRAMS_LIB_SOURCE)/FlightOps/Documents/boost-user-config-homebrew.jam user-config.jam
vim user-config.jam
./bootstrap.sh --prefix="$HOME/work/software/boost_1_88_0" --includedir=headers --libdir=dist --with-python=python3
./b2 --clean-all
./b2 install --user-config=./user-config.jam
```

Find Python headers and library paths on macOS with:

```bash
python3 -c "from sysconfig import get_paths; print(get_paths())"
```
