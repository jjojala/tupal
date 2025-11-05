
# Build

Trying to get rid of pre-installed packages (striving toward self-standing build)

```
> sudo apt-get install libicu-dev
> sudo apt-get install libboost-all-dev libsqlite3-dev
> mkdir build
> cmake -S . -B build
> cmake --build build
```

# Run tests

```
> ctest --test-dir build
```

# Run

By default the database `tupald.db` is created/pre-exists to/on current directory.

```
> build/src/tupald
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