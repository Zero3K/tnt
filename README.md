# tnt
`tnt` is a simple console torrent client. **It is currently not finished and can only download torrents that consist of one file.**

### Building
```sh
mkdir build
cd build
cmake ..
make
```

### Usage
```sh
./tnt filename
```

### Specifics
- Self-written bencode decoding/encoding library.
- `cpr` is used for making HTTP requests to trackers.

### Features to implement
- [x] Accept parameters from input args
- [x] Save downloaded pieces to disk
- [ ] Don't download already saved parts of file
- [x] Reconnect to peers after spontaneous disconnect
- [ ] Keep track of peers pieces availabilty 
- [ ] Rework piece downloading strategy
- [ ] Dynamic piece wait time? (it's the time which storage piece waits before returning piece to pool)
- [ ] Move piece acquiring logic to separate class? 
