#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
tupal="$script_dir/tupal"

# Competition "Pirkkalan Hölkkä" on 4.8.2024
"$tupal" create competition --id comp-1 \
    --date 2024-08-04 --title "Pirkkalan Hölkkä" >/dev/null

# Start group "Children, under 16y" with first bib 401 and a 30-second start interval
"$tupal" create start-group --id sg-1 --competition comp-1 \
    --title "Children, under 16y" \
    --first-start-time 2024-08-04T17:30:00.000Z \
    --first-bib 401 --start-interval PT30S >/dev/null

# Start group "Adults, 16y and older" with a mass start
"$tupal" create start-group --id sg-2 --competition comp-1 \
    --title "Adults, 16y and older" \
    --first-start-time 2024-08-04T18:00:00.000Z \
    --first-bib 1 --start-interval PT0S >/dev/null

# Create classes for the later start group first; live clients may refresh the
# joined class list after every create.
"$tupal" create class --id cc-3 --competition comp-1 \
    --title MYL --start-group sg-2 >/dev/null
"$tupal" create class --id cc-4 --competition comp-1 \
    --title N40 --start-group sg-2 >/dev/null

# Then create classes for the first group so the start-group join has no gaps.
"$tupal" create class --id cc-1 --competition comp-1 \
    --title P10 --start-group sg-1 >/dev/null
"$tupal" create class --id cc-2 --competition comp-1 \
    --title T14 --start-group sg-1 >/dev/null

# Populate competitors in reverse class order so live roster refreshes don't
# encounter unpopulated classes after populated ones.
# Woman, above 40y
"$tupal" create competitor --id ctor-11 --competition comp-1 --class cc-4 \
    --bib 5 --start-time-offset PT0.000S --name "Smith Emily" >/dev/null
"$tupal" create competitor --id ctor-12 --competition comp-1 --class cc-4 \
    --bib 6 --start-time-offset PT0.000S --name "Johnson Olivia" >/dev/null
"$tupal" create competitor --id ctor-13 --competition comp-1 --class cc-4 \
    --bib 7 --start-time-offset PT0.000S --name "Davis Ava" >/dev/null

# Men, main class
"$tupal" create competitor --id ctor-7 --competition comp-1 --class cc-3 \
    --bib 1 --start-time-offset PT0.000S --name "Shaw Oscar" >/dev/null
"$tupal" create competitor --id ctor-8 --competition comp-1 --class cc-3 \
    --bib 2 --start-time-offset PT0.000S --name "Graig Ethan" >/dev/null
"$tupal" create competitor --id ctor-9 --competition comp-1 --class cc-3 \
    --bib 3 --start-time-offset PT0.000S --name "Murray Sebastian" >/dev/null
"$tupal" create competitor --id ctor-10 --competition comp-1 --class cc-3 \
    --bib 4 --start-time-offset PT0.000S --name "Robertson Musa" >/dev/null

# Girls, under 14y
"$tupal" create competitor --id ctor-4 --competition comp-1 --class cc-2 \
    --bib 404 --start-time-offset PT0.000S --name "Brown Charlotte" >/dev/null
"$tupal" create competitor --id ctor-5 --competition comp-1 --class cc-2 \
    --bib 405 --start-time-offset PT0.000S --name "Taylor Amelia" >/dev/null
"$tupal" create competitor --id ctor-6 --competition comp-1 --class cc-2 \
    --bib 406 --start-time-offset PT0.000S --name "Wilson Mia" >/dev/null

# Boys, under 10y
"$tupal" create competitor --id ctor-1 --competition comp-1 --class cc-1 \
    --bib 401 --start-time-offset PT0.000S --name "McLeod Archie" >/dev/null
"$tupal" create competitor --id ctor-2 --competition comp-1 --class cc-1 \
    --bib 402 --start-time-offset PT0.000S --name "Jones Benjamin" >/dev/null
"$tupal" create competitor --id ctor-3 --competition comp-1 --class cc-1 \
    --bib 403 --start-time-offset PT0.000S --name "Williamson George" >/dev/null
