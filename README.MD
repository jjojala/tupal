
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
# tupal -tests:
> ctest --test-dir build -L tupal --output-on-failure   # MESSAGE's printed on failure
> ctest --test-dir build -L manager --output-on-failure
> ctest --test-dir build -L rest --verbose               # MESSAGE's and more printed by default
# or all:
> ctest --test-dir build --output-on-failure
```
(tupal includes all test sets)

With command `ctest --test-dir build -V -N` you can see the environment and command for individual tests. With 
that information it is possible to debug tests, e.g.

```
> PATH=... LD_LIBRARY_PATH=... gdb rest_test
```


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
*Swagger UI*
Start with swagger enabled, eg: `LD_LIBRARY_PATH=build/lib build/src/tupal -S`
, then with your browser hit url e.g. `http://localhost:8005/swagger.html`

# Database

Database is created automatically if doesn't already exist. One may see the content with the sqlite3 client.
Some examples:

```
> sqlite3 tupal.db
> sqlite3 tupel.db "select title from competition"
> sqlite3 tupel.db .schema
```
