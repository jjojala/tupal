#!/bin/sh

# usage: make_competition_data <comp-id> <date> <title>
# stdout: competition json
make_competition_data() {
cat -<< EOF
    {
        "id": "$1",
        "date": "$2",
        "title": "$3"
    }
EOF
}

# usage:make_start_group_data <sg-id> <title> <1st start> <1st bib in sg>
# stdout: start group json
make_start_group_data() {
cat -<<EOF
    {
        "id": "$1",
        "title": "$2",
        "first_start_time": "$3",
        "first_bib": $4
    }
EOF
}

# usage: make_competition_class_data <cc-id> <title> <sg-id>
# stdout: competition class json
make_competition_class_data() {
cat -<<EOF
    {
        "id": "$1",
        "title": "$2",
        "start_group_id": "$3"
    }
EOF
}

# usage: make_competitor_data <ctor-id> <cc-id> <bib> <start-time-offset> <name>
make_competitor_data() {
cat -<<EOF
    {
        "id": "$1",
        "comp_class_id": "$2",
        "bib": $3,
        "start_time_offset": "$4",
        "finish_time": "",
        "status": 0,
        "name": "$5"
    }
EOF
}

endpoint="http://127.0.0.1:8085/rest"

# Competition "Pirkkalan Hölkkä" on 4.8.2024
curl -X POST -d "$(make_competition_data comp-1 "2024-08-04" "Pirkkalan Hölkkä")" \
    ${endpoint}/competition/

# Start group "Children, under 16y" with first bib 300
curl -X POST -d "$(make_start_group_data sg-1 "Children, under 16y" "2024-08-04T17:30:00.000Z" 401)" \
    ${endpoint}/competition/comp-1/start_group/

# Start group "Adults, 16y and oler" with first bib 300
curl -X POST -d "$(make_start_group_data sg-2 "Adults, 16y and older" "2024-08-04T18:00:00.000Z" 1)" \
    ${endpoint}/competition/comp-1/start_group/

# Boys, under 10y
curl -X POST -d "$(make_competition_class_data cc-1 P10 sg-1)" \
    ${endpoint}/competition/comp-1/competition_class/

curl -X POST -d "$(make_competitor_data ctor-1 cc-1 401 PT0.000S "McLeod Archie")" \
    ${endpoint}/competition/comp-1/competitor/

curl -X POST -d "$(make_competitor_data ctor-2 cc-1 402 PT0.000S "Jones Benjamin")" \
    ${endpoint}/competition/comp-1/competitor/

curl -X POST -d "$(make_competitor_data ctor-3 cc-1 403 PT0.000S "Williamson George")" \
    ${endpoint}/competition/comp-1/competitor/

# Girls, under 14y
curl -X POST -d "$(make_competition_class_data cc-2 T14 sg-1)" \
    ${endpoint}/competition/comp-1/competition_class/

curl -X POST -d "$(make_competitor_data ctor-4 cc-2 404 PT0.000S "Brown Charlotte")" \
    ${endpoint}/competition/comp-1/competitor/

curl -X POST -d "$(make_competitor_data ctor-5 cc-2 405 PT0.000S "Taylor Amelia")" \
    ${endpoint}/competition/comp-1/competitor/

curl -X POST -d "$(make_competitor_data ctor-6 cc-2 406 PT0.000S "Wilson Mia")" \
    ${endpoint}/competition/comp-1/competitor/

# Men, main class
curl -X POST -d "$(make_competition_class_data cc-3 MYL sg-2)" \
    ${endpoint}/competition/comp-1/competition_class/

curl -X POST -d "$(make_competitor_data ctor-7 cc-3 1 PT0.000S "Shaw Oscar")" \
    ${endpoint}/competition/comp-1/competitor/

curl -X POST -d "$(make_competitor_data ctor-8 cc-3 2 PT0.000S "Graig Ethan")" \
    ${endpoint}/competition/comp-1/competitor/

curl -X POST -d "$(make_competitor_data ctor-9 cc-3 3 PT0.000S "Murray Sebastian")" \
    ${endpoint}/competition/comp-1/competitor/

curl -X POST -d "$(make_competitor_data ctor-10 cc-3 4 PT0.000S "Robertson Musa")" \
    ${endpoint}/competition/comp-1/competitor/

# Woman, above 40y
curl -X POST -d "$(make_competition_class_data cc-4 N40 sg-2)" \
    ${endpoint}/competition/comp-1/competition_class/

curl -X POST -d "$(make_competitor_data ctor-11 cc-4 5 PT0.000S "Smith Emily")" \
    ${endpoint}/competition/comp-1/competitor/

curl -X POST -d "$(make_competitor_data ctor-12 cc-4 6 PT0.000S "Johnson Olivia")" \
    ${endpoint}/competition/comp-1/competitor/

curl -X POST -d "$(make_competitor_data ctor-13 cc-4 7 PT0.000S "Davis Ava")" \
    ${endpoint}/competition/comp-1/competitor/