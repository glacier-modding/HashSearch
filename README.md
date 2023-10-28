# HashSearch
Backend powering [https://hitmandb.glaciermodding.org/](https://hitmandb.glaciermodding.org/)

## Building

### Linux
`g++ hashsearch.cpp -lfcgi++ -lfcgi -O2 -o hashsearch`

## Running

### Linux
`spawn-fcgi -p 8000 -n hashsearch`