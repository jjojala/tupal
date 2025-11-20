
# Build

Trying to get rid of pre-installed packages (striving toward self-standing build)

```
> sudo apt-get install libicu-dev
> sudo apt-get install libboost-all-dev libsqlite3-dev libsqlite3-0
> mkdir build
> cmake -S . -B build
> cmake --build build --parallel
```

# Run tests

```
> ctest --test-dir build -L tupal
> ctest --test-dir build -L manager
> ctest --test-dir build -L rest
```
(tupal includes all test sets)

# Run

*Help:*

```
> build/src/tupald --help
```

*Start with embedded sqlite3 database*
```
> LD_LIBRARY_PATH=build/lib build/src/tupald --db sqlite3://tupal.db
```
(soci backend location need to be pointed out with LD_LIBRARY_PATH)

*Testing:*
```
> curl -X GET http://localhost:8085/rest/competition/
> curl -X GET http://localhost:8085/rest/competition/my-comp-id
> curl -X POST http://localhost:8085/rest/competition/ -d @my-new-comp.json
> ...
```

# Database

Database is created automatically if doesn't already exist. One may see the content with the sqlite3 client.
Some examples:

```
> sqlite3 tupal.db
> sqlite3 tupel.db "select title from competition"
> sqlite3 tupel.db .schema
```