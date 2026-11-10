# Tupal

Tupal is a competition management and results system. The name "TUPAL" stands for finish word "tulospalvelu", meaning boringly "result service". Tupal consists of centralized backend service "tupald" and (will eventually) the following web-based interfaces:
* Competition Management - Results and the founding competition information
* Competitor's Registration - Adding new competitors
* Start - For officials at start line
* Times - For timekeeper
* Numbers - For typing bib numbers as they reach the finish line
* Finishline - Combines Times & Numbers plus for handling DSQ's

Tupal backend is a C++ daemon with a REST API, while the apps are web apps, written with TypeScript and Svelte. The database is defaulted to SQLite, but utilizes the SOCI that enables switching to centralized DMBS such as PostgreSQL.

## Quick start

### Build

Requirements: CMake, a C++17 compiler, Boost, and Node.js/npm for the web
frontend. The expected Node.js version is recorded in `web/.nvmrc`.

```sh
cmake -S . -B build
cmake --build build --parallel
```

### Run all tests

The full CTest suite includes the Chromium-based system smoke test. On Linux,
install Chromium and its system dependencies once before running the suite:

```sh
cd web
npx playwright install --with-deps chromium
cd ..
```

```sh
ctest --test-dir build --output-on-failure
```

### Run the system smoke test

The smoke test starts a temporary daemon, seeds data through the REST API, and
checks competitor registration and editing in Chromium. Install Chromium and its
Linux dependencies once:

```sh
cd web
npx playwright install --with-deps chromium
cd ..
ctest --test-dir build -L system --output-on-failure
```

### Run Tupal with sample data

Build or refresh the generated frontend assets from the repository root:

```sh
cmake --build build --target web_frontend
```

Start the daemon from its build directory so its default `./web` root resolves
to the generated frontend assets. Start with a fresh database; the sample script
uses fixed IDs. This example stores data in `test.db`:

```sh
cd build/src
./tupald --db sqlite3://test.db
```

In another terminal, from the repository root, populate that running daemon:

```sh
./script/make-test-data.sh
```

Open the sample competition in the registration app at
<http://localhost:8085/registration/comp-1> or in the Times app at
<http://localhost:8085/times/comp-1>. Both apps use the test data for `comp-1`.
The server defaults to port 8085; use `./tupald --help` to see its options.

Create finish-line observations with the CLI. The first command prints the
generated time-entry ID, which can be linked to a bib entry:

```sh
time_id=$(./script/tupal create finish-time-entry --competition comp-1 \
  --finish-time 2024-08-04T18:40:00.000Z)
./script/tupal create finish-bib-entry --competition comp-1 \
  --sequence-no 14 --bib 8 --confirmed-time-entry-id "$time_id"
```

Use `./script/tupal --help` for the full list of create commands and options.

## Build reference

Configure the project with:

```sh
cmake -S . -B build [options]
```

Useful CMake options:

| Option | Default | Description |
|---|---:|---|
| `BUILD_PROJECT_TESTS` | `ON` | Build and register Tupal's tests. |
| `BUILD_DEPENDENCY_TESTS` | `OFF` | Allow dependencies to build their own tests. |

Build the default target or a specific target:

```sh
cmake --build build --parallel
cmake --build build --target tupald
```

Project targets:

| Target | Description |
|---|---|
| `all` | Default build: Tupal libraries, daemon, tests (when enabled), and frontend assets. |
| `tupald` | Tupal HTTP/WebSocket daemon. |
| `tupal_core` | Core competition and data-management library. |
| `web_frontend` | Checks and builds the web frontend into `build/src/web`. Included in the default build. |
| `web_docs` | Generates the web TypeScript/JavaScript reference with Doxygen, when Doxygen is available at CMake configure time. |
| `manager_tests`, `model_tests`, `rest_tests` | Native C++ test executables. |

To see all targets, including dependency targets, run:

```sh
cmake --build build --target help
```

## Test reference

CTest tests and labels:

| Test | Labels | Description |
|---|---|---|
| `manager_tests` | `tupal`, `manager` | Competition, start-group, class, and competitor manager tests. |
| `model_tests` | `tupal`, `model` | Data model and serialization tests. |
| `rest_tests` | `tupal`, `rest` | REST, WebSocket, and generic static-file server tests. |
| `web_system_smoke` | `tupal`, `web`, `system` | Starts the daemon, seeds via REST, and exercises registration UI in Chromium. |

Run all tests, a test by name, or tests matching a label:

```sh
ctest --test-dir build --output-on-failure
ctest --test-dir build -R manager_tests --output-on-failure
ctest --test-dir build -L rest --output-on-failure
ctest --test-dir build -L system --output-on-failure
```

CTest labels are regular expressions. The `tupal` label selects all four
Tupal test groups, including the browser smoke test. List discovered tests with:

```sh
ctest --test-dir build -N
ctest --test-dir build -N -V
```

## Web API documentation

The TypeScript and JavaScript modules under `web/src` are documented with
Doxygen:

```sh
cd web
doxygen Doxyfile
```

Open `web/docs/html/index.html`. Alternatively, if Doxygen was found during
CMake configuration, run `cmake --build build --target web_docs`.
