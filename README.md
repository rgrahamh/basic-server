# basic-server
Just a simple C server that I had to cook up for Data Com; figured I might as well post it on GitHub!

## Making
In order to make, you can choose:
- `make dev` - Makes the server applicaiton
- `make server` - Makes the server application with some extra information printing to terminal (like the parsed request and the generated response)
- `make clean` - Gets rid of `./server`

## Running
In order to run, please use the following format:
```
./server <file-dir> <port>
```
Where `<file-dir>` is the directory in which the files are stored and `<port>` is the port on which the server is being hosted. As an example:
```
./server ./files 8080
```
To use the directory `./files` and host on port `8080`.
