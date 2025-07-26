# tnt
`tnt` is a simple console torrent client.

![demogif](https://github.com/alt-qi/tnt/blob/main/demo.gif)

### Building
```sh
mkdir build
cd build
cmake ..
make
```

### Usage
```sh
tnt [OPTION...] torrent_file

  -h, --help     Print usage
  -q, --quiet    Minimal output
  -o, --out arg  Output directory (working directory by default)
```

### Features
- Self-written bencode decoding/encoding library and torrent file parser.
- `cpr` is used for communicating with HTTP trackers.
- Endgame mode is implemented.
- Multifile torrents are supported.

### Features to implement
- [x] Accept parameters from input args
- [x] Save downloaded pieces to disk
- [ ] Don't download already saved parts of file (load saved data)
- [x] Implement endgame mode
- [x] Reconnect to peers after spontaneous disconnect
- [x] Keep track of peers pieces availabilty 
- [ ] Add more command line options (like timeouts or announce override)
- [x] Rework piece downloading strategy
- [x] Add support for multifile torrents.
- [x] Move console graphics logic to separate classes
- [x] Move piece acquiring logic to separate class? 
- [ ] Add support for UDP trackers?
