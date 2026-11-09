# Frontend functionality

The frontend functionality can be briefly divided into the following categories:
1. Competition Management - primarily for creating, deleting and modifying competition founding information before the actual event, suchs as title, date and time, and classes; but also to maintain operative data during the competition, such as competitors, start times and results. This is primarily done with a desktop or laptop computer in an office-like conditions.
1. Competitor's Registration - This is the primary function for adding new competitors at on-site registration just before the competition starts. This is typically done outside, e.g. under a canopy, with a light laptop computer or a tablet with a keyboard.
1. Start - This is for officials at the startline to mark competitors to on-route or DNS. This is typically done with a mobile device at the field.
1. Times - This is a tool for time keeping offical (timekeeper). His/hers duty is to take times for competitors while they enter the finishline. This task is done at the field with a mobile device.
1. Numbers - This is a tool for adding comptitor's bib numbers to system in the order they enter the finish line. Numbers are automatically paired with the times. This task is done at the field with a mobile device.
1. Finishline - this is for the officials working literally on finishline for a small events when a distinct times or numbers officials are inapproprate or for a supervicing official that "fills the gaps" and helps on rushes, makes potentinal DSQ's. Fundamentally this combines the functions of distinct Times and Numbers functions. Task is carried out at the finishline, i.e. very much at the field with a mobile device.

## General

### GEN-1 - Sign-on and user identification

1. No user identification is, at least primarily required
1. Sing-on may be based on PIN, tag etc, which is generally known by the officials during their tasks

### GEN-2 - Concurrent use

1. Many of the functions are intended to be used simulaneously and many instances of a single function may co-exist simultaneously
1. So the system must handle conflicting changes, and report them accordingly for the user
1. However, the usage is mostly distributed so, that the manipulated data gets naturally distributed in time or space, or by concentrated to a different data item.

## Competition Management

### MAN-1 — Browse and select competitions

1. Show a list of competitions and let the user open a competition in Management.
1. Optimize the Management interface primarily for desktop use.

### MAN-2 — Manage classes, start groups, and competitors

1. Add, edit, and delete competition classes and start groups.
1. Add, edit, and delete competitors, and assign them to classes and start groups.
1. Allow editing a competitor’s bib number and start time.
1. Require confirmation before deletion; preserve the history of recorded events.

### MAN-3 — Change a start group’s start time

1. Let the user change the start time for an entire start group in one action.
1. Reflect the change in the Start app.

### MAN-4 — Show competition status totals

1. Show totals for started, not started, confirmed DNS, finished/resulted, and open competitors.
1. Finished/resulted includes competitors with either an accepted result or a DSQ.
1. Open = total competitors, minus the competitors with a result (DSQ or acceptable result), minus the confirmed DNS competitors.
1. DNS is a competitor, intentionally not started (no-show), while not started is a competitor that hasn't started _yet_ but is atm expected to compete.
1. DNS suggestions are not shown in Management and do not affect these totals (see "Start")

### MAN-5 — Review timing and number-entry records

1. Show timing and number-entry records in order, with their status: normal (confirmed), uncertain, incorrect (known to be wrong), or missing (know to be missing)
1. Show unmatched time and number entries, i.e. sync status, such "expecting 5 numbers" (more times, than numbers), "expecting 3 times" (more numbers, than times)
1. Let the user review and correct records without deleting the original entries.

### MAN-6 — Generate results

1. Generate results from timing records, number entries, and competitor data.
1. Flag missing or conflicting data for review
1. Flag class status (result, open) for review
1. Indicate, when a competion class is ready; indicate the best possible position
1. Preserve a DSQ competitor’s time and make it available for review, even though the competitor’s result is recorded as DSQ.

## Epic: Registration

### REG-1 - Open a Registration task from a link

1. Open the Registration app directly to the correct competition and task from a link.

### REG-2 - Register competitors

1. Add, edit or delete a competitor to a specific class. Classes are pre-defined with other tools. They're not modifiable with registration app.
1. Assign a unique bib number for the competitor
1. Optimized for mobile device with a keyboard, such as a light laptop or a tablet with a keyboard

### REG-3 - Bib number assignment

1. Typically the registration official has literally on his/hers reach a pile of physical bib numbers, that are given for the registrees as they are registering on-site.
1. The bib number are typically be pre-arranged to a ascending order in the start-group specific piles. Note, that the bib number shall be unique in scope of the whole competition - each start-group uses a pre-allocated range of bib's.
1. The registration app should suggests in ascending order available bib numbers when entering competitor's data to the registration app. Ideally, the bib suggested by app matches with the top most physical pile of bibs, therefore greatly helping the registration official. In practice the official pick the physical bib from the pile, then fix - if needed - the bib in the registration form, and accepts it before actually handing the bib to the registree.

### REG-4 - Automatic start time assigment

1. The start time will be automatically picked by the registration app, in accordance of the chosen bib, and the start stime interval defined by the start-group. For example, if start time interval is 30seconds, the competitor's start time offset for bib 1 is 0, bib 2 is 30secs, 3 - 60secs, etc. Start time interval 0, refers to a mass start, i.e. all competitors within that start group will get the same start time.

### REG-4 - List of registrations

1. The registration app shall show a searchable list of registrations
1. The list shall show competitor's name, class and start time
1. By default the list shows the registrations in the reversed registration order, i.e. the most recently added registration is at the top of the list. This function applies to current registration-app usage session, i.e. it is done locally within the client. The registrations done before the session, are shown in reversed bib order below latest registrations.
1. Additionally the list can be filtered by start-group or class

### REG-5 - Shortcut for class assignment

1. Small competition will have 6-8 classes, and even a big one will have less than 30 classes
1. For selecting a class, a e.g. toggle button or some other "one-click" shortcut shall be provided, for the most common classes. The "most common" can be bound to current registration app usage session, i.e. without support from the backend.
1. Less common classes, if unfeasible to have fitted to the registration app as shortcuts, can be selected e.g. by typing or picking from the list.

### REG-6 - Auto-completion for competitor's name fragments

1. It is common, that registration takes place by families, i.e. e.g. one person of family "Smith" registers all familiy members at once.
1. The registration app should assist such scenario by auto-completing the name fragments (such as "Sm" --> "Smith", "Jo" --> "John") as entered before. The auto-completion shall emphasize on the recent fragments, i.e. if previous registration was "Smith", but before that was three times "Smyth", suggest "Smith", because it is more recent.

## Epic: Start

### START-1 — Open a Start task from a link

1. Open the Start app directly to the correct competition and task from a link.

### START-2 — Display the start list

1. Display bib numbers in start-time order.
1. Show the start time and associated bib numbers on each row.

### START-3 — Mark competitors as started

1. Let the official mark an individual competitor as started ("on-route")
1. The marking can be done in advance (i.e. the competitor is waiting at the start line to start)
1. When competitor is marked as started, and the start time passes, the competitor's entry disappears from the list

### START-4 — Create DNS suggestions automatically

1. Automatically create a DNS suggestion when a competitor’s start time has passed, they have not been marked as started in advance.
1. A DNS suggestion is not a confirmed DNS and is not shown outside the Start app.
1. Keep the suggestion visible in Start until it is confirmed or the competitor is marked as started.

### START-5 — Handle DNS suggestions
1. Show DNS suggestions in a compact list or separate view so they do not obscure upcoming starters.
1. Show at least the competitor’s bib number; the start time is not required.
1. Let the official confirm a suggestion as DNS; a confirmed DNS disappears on the start list.
1. Let the official mark the competitor as started, which cancels the suggestion; confirmed starts disappear from the list after start-time
1. A confirmed DNS updates the DNS and Open totals in Management.

## Epic: Times

### TIMES-1 — Open a Times mobile app from a link
1. Open the competition-specific Times directly from a link.
1. Do not require competition selection in the app.

### TIMES-2 — Record a finish time
1. Let the timekeeper record a finish time quickly on a mobile device.
1. Clearly display the most recently recorded times.

### TIMES-3 — Mark an entry’s status
1. By default, a time entry's status is "confirmed", but entry's status may be also "missing" (time entry at specific sequence known to be missing) or "incorrect" (known to be incorrect time)
1. Let the official mark the status of time entry quickly
1. Let the official record a missing time (automaticlly "missing") quickly to a specific location at the ordered time-entry (e.g. "0:09:21;31", "0:09:21:39", <missing>, "0:09:21:54"..)

### TIMES-4 — Show the relationship between time and number queues

1. Show the most recent bib entries as they are entered (in the same order)
1. Show the most recent paired time-entries and the bib-entries (in the time order)
1. For example, show extra time entries as awaiting bib numbers, and extra number entries as awaiting times.

## Epic: Numbers

### NUMBERS-1 — Open a Times mobile app from a link

1. Open the competition-specific Numbers directly from a link.
1. Do not require competition selection in the app.

### NUMBERS-2 — Record bib numbers in finish order

1. Let the official record bib numbers in the order competitors finish.
1. Allow entry either by selecting a suggested number or by typing one directly.

### NUMBERS-3 — Suggest likely bib numbers

1. Suggest likely next bib numbers based on the start list.
1. By default, hide numbers that have already been entered, have not started or have DSN from the quick-select suggestions.
1. Keep all numbers searchable and available for manual entry.

## Epic: Finishline

### FINISH-1 — Open a Times mobile app from a link
1. Open the competition-specific Times directly from a link.
1. Do not require competition selection in the app.

### FINISH-2 — Record times and numbers in one view

1. Let one official record both times and bib numbers in a single mobile view.
1. The combined view must create time and number entries compatible with the same event queues used by the separate apps.
1. Support uncertain, incorrect, and missing entries.

### FINISH-2 — Record a DSQ

1. Let the official assign a DSQ to an identified competitor directly in Finishline.
1. A DSQ is a final decision; there is no separate proposal stage.
1. An optional short, single-line text reason for the DSQ is provided.
1. Preserve the competitor’s time after the DSQ is recorded.
1. DSQ status is possible to remove, in which case the original finish time becomes active.
1. Show the DSQ as a result in Management’s totals and results.
