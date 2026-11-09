
# Build

Trying to get rid of pre-installed packages (striving toward self-standing build)

```
> sudo apt-get install libicu-dev
> sudo apt-get install libboost-all-dev libsqlite3-dev libsqlite3-0
> mkdir build
> cmake -S . -B build
> cmake --build build --parallel
```

The default CMake build also builds the frontend. This requires Node.js and npm;
the expected Node.js version (`22.15.0`) is recorded in `web/.nvmrc`. CMake
installs the locked frontend dependencies and writes the production assets to
the build directory.

# Web code documentation

The TypeScript and JavaScript modules under `web/src` are documented with
Doxygen, using the same tool as the C++ API. Install Doxygen if it is not already
available, then generate the web reference from the repository root:

```
sudo apt-get install doxygen
cd web
doxygen Doxyfile
```

Open `web/docs/html/index.html` to browse the generated documentation. The output
is generated locally and is not included in the application build. If Doxygen
was installed when CMake was configured, the same documentation can be generated
with `cmake --build build --target web_docs`.

# Run tests

The system smoke test starts a temporary in-memory daemon, seeds competition
data through the REST API, and uses Chromium to register and update a competitor
through the real UI. Install the browser once with
`cd web && npx playwright install --with-deps chromium` on Linux; then run it with
`ctest --test-dir build -L system --output-on-failure`. GitHub Actions installs
Chromium and runs this test as part of CI.

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
> PATH=... gdb rest_test
```


# Run

*Help:*

```
> cd build/src
> ./tupald --help
```

*Start with embedded sqlite3 database*
```
> cd build/src
> ./tupald --db sqlite3://tupal.db
```

The default static web root is `./web` relative to the current working
directory. The CMake build puts the executable and generated web assets under
`build/src/`. Use `--web-root <path>` to override the default.

*Open competitor registration*
```
> http://localhost:8085/registration/my-comp-id
```
Replace `my-comp-id` with the competition id. The competition must already have
at least one class configured.

*Testing:*
```
> curl -X GET http://localhost:8085/rest/competition/
> curl -X GET http://localhost:8085/rest/competition/my-comp-id
> curl -X POST http://localhost:8085/rest/competition/ -d @my-new-comp.json
> ...
```
*Swagger UI:*

Swagger is by default supported. Just hite the browser with url, e.g. `http://localhost:8005/swagger.html`

# Database

Database is created automatically if doesn't already exist. One may see the content with the sqlite3 client.
Some examples:

```
> sqlite3 tupal.db
> sqlite3 tupel.db "select title from competition"
> sqlite3 tupel.db .schema
```
By default, sqlite3 is used as a database, but also others such as PostreSQL, MySQL and others are supported in code.
