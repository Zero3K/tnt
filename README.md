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
- [ ] Accept parameters from input args
- [ ] Save downloaded pieces to disk
- [ ] Don't download already saved parts of file
- [ ] Reconnect to peers after spontaneous disconnect (with dynamic retry time)
- [ ] Keep track of peers pieces availabilty 
- [ ] Dynamic piece wait time? (it's the time which storage piece waits before returning piece to pool)
- [ ] Move piece acquiring logic to separate class? 