# megash

`megash` - one binary exec for everything like BusyBox

# USAGE

     megash [ put | ls | dl | get | rm | mkdir | copy | reg | df ] @ARGS

or via symlinks `megaXXX` where 
    
    X = [ put | ls | dl | get | rm | mkdir | copy | reg | df ]

## COMPILE 
 
    ./autogen.sh
    ./configure --disable-docs
    make
    ./utils/make_megash

ok!

## FILES

```
bin/megash
bin/megacopy	-> megash
bin/megadf	-> megash
bin/megadl	-> megash
bin/megaget	-> megash
bin/megals	-> megash
bin/megamkdir	-> megash
bin/megaput	-> megash
bin/megareg	-> megash
bin/megarm	-> megash
```

## INSTALL

    cp bin/mega* /usr/bin

## Config from STDIN

now we can read config direct from STDIN, not only from files via `-c -` option

### READ CONFIG FROM STDIN EXAMPLE

    cat CONFIG | megash CMD -c - @ARGS

