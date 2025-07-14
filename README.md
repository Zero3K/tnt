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
./tnt -o output_file torrent_file
```

### Specifics
- Self-written bencode decoding/encoding library.
- `cpr` is used for making HTTP requests to trackers.

### Features to implement
- [x] Accept parameters from input args
- [x] Save downloaded pieces to disk
- [ ] Don't download already saved parts of file
- [x] Reconnect to peers after spontaneous disconnect
- [x] Keep track of peers pieces availabilty 
- [ ] Add more command line options (like timeouts)
- [x] Rework piece downloading strategy
- [ ] Add support for multifile torrents.
- [ ] Move console graphics logic to separate classes
- [ ] Dynamic piece wait time? (it's the time which storage piece waits before returning piece to pool)
- [x] Move piece acquiring logic to separate class? 
