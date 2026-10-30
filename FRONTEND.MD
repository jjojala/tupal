# Frontend functionality

The frontend functionality can be briefly divided into the following categories
1. Management - primarily for creating, deleting and modifying competition founding information before the actual event, suchs as title, date and time, classes; but also to maintain operative data during the competition, such as competitors, start times and results. This is primarily done e.g. with desktop or laptop computed in office-like conditions.
1. Registration - This is the primary function for adding new compeitors especially at on-site registration just before the competition starts. This is typically done outside, e.g. under a canopy, with a light laptop or tablet with a keyboard.
1. Start - This is for officials at start to mark competitors to on-route or DNS. This is typically used through mobile device at field. 
1. Times - This is a tool for time keeping offical. His/hers duty is to take times for competitors while they enter the finishline. This task is done at the field with a mobile device.
1. Numbers - This is a tool for adding comptitor's bib numbers to system in the order they enter the finish line. Numbers are automatically paired with the times. This task is done at the field with a mobile device.
1. Finishline - this is for the officials working literally on finishline for small events, when distinct times or numbers officials are inapproprate or for a supervicing official that "fills the gaps" and helps on rushes, makes potentinal DSQ's. Fundamentally this combines the functions of distinct Times and Numbers functions. Task is carried out at finishline, i.e. very much at the field with a mobile device.

## Epic: Management

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
1. Open = total competitors − competitors with a result − confirmed DNS competitors.
1. The started and finished totals may overlap.
1. DNS suggestions are not shown in Management and do not affect these totals.
1. Let the user open a filtered competitor list from each total.

### MAN-5 — Review timing and number-entry records

1. Show timing and number-entry records in order, with their status: normal, uncertain, incorrect, or missing.
1. Show unmatched time and number entries.
1. Let the user review and correct records without deleting the original entries.

### MAN-6 — Generate results

1. Generate results from timing records, number entries, and competitor data.
1. Flag missing or conflicting data for review.
1. Preserve a DSQ competitor’s time and make it available for review, even though the competitor’s result is recorded as DSQ.

## Epic: Registration

### REG-1 - Register competitors

1. Add, edit or delete a competitor to a specific class.
1. Assign a bib number for the competitor
1. Optimized for mobile device with a keyboard, such as a light laptop or a tablet with a keyboard

## Epic: Start

### START-1 — Open a Start task from a link

1. Open the Start app directly to the correct competition and task from a link.
1. Do not require sign-in or competition selection at this stage.

### START-2 — Display the start list

1. Display bib numbers in start-time order.
1. Show the start time and associated bib numbers on each row.

### START-3 — Mark competitors as started

1. Let the official mark an individual competitor as started.
1. Save the mark and update the competitor’s status.
1. Save the mark and update the competitor’s status.

### START-4 — Create DNS suggestions automatically
1. Automatically create a DNS suggestion when a competitor’s start time has passed and they have not been marked as started.
1. A DNS suggestion is not a confirmed DNS and is not shown outside the Start app.
1. Keep the suggestion visible in Start until it is confirmed or the competitor is marked as started.

### START-5 — Handle DNS suggestions
1. Show DNS suggestions in a compact list or separate view so they do not obscure upcoming starters.
1. Show at least the competitor’s bib number; the start time is not required.
1. Let the official confirm a suggestion as DNS.
1. Let the official mark the competitor as started, which cancels the suggestion.
1. A confirmed DNS updates the DNS and Open totals in Management.

## Epic: Times and Numbers

### TIME/NUM-1 — Open a task-specific mobile app from a link
1. Open the competition-specific Times, Numbers, or combined view directly from a link.
1. Do not require competition selection in the app.
1. Sign-in is out of scope for this phase.

### TIMES-2 — Record a finish time
1. Let the timekeeper record a finish time quickly on a mobile device.
1. Clearly display the most recently recorded time.

### NUMBERS-3 — Record bib numbers in finish order
1. Let the official record bib numbers in the order competitors finish.
1. Allow entry either by selecting a suggested number or by typing one directly.

### NUMBERS-4 — Suggest likely bib numbers
1. Suggest likely next bib numbers based on the start list.
1. By default, hide numbers that have already been entered from the quick-select suggestions.
1. Keep all numbers searchable and available for manual entry.

### TIME/NUM-5 — Mark an entry’s status
1. Let the official mark a time or number entry as uncertain or incorrect.
1. Let the official record a missing time or number as a separate event, preserving the sequence.
1. Changing an entry’s status must not delete the original event.

### TIME/NUM-6 — Show the relationship between time and number queues

1. In Times and Numbers, show how many events remain unmatched between the two queues.
1. For example, show extra time entries as awaiting bib numbers, and extra number entries as awaiting times.
1. Queue matching status and connection status are out of scope for this phase; a stable internet connection is assumed.

## Epic: Finishline

### FINISH-1 — Record times and numbers in one view

1. Let one official record both times and bib numbers in a single mobile view.
1. The combined view must create time and number entries compatible with the same event queues used by the separate apps.
1. Support uncertain, incorrect, and missing entries.

### FINISH-2 — Record a DSQ

1. Let the official assign a DSQ to an identified competitor directly in Finishline.
1. A DSQ is a final decision; there is no separate proposal stage.
1. Require a short, single-line text reason for the DSQ.
1. Preserve the competitor’s time after the DSQ is recorded.
1. Show the DSQ as a result in Management’s totals and results.

## Epic: Event History and Concurrent Use

### DATA-1 — Store competition events

1. Store timing, number-entry, start, DNS, and DSQ events as distinguishable event types.
1. Preserve event order and timestamps.
1. Corrections and cancellations must not delete the original event history.

### DATA-2 — Handle concurrent entries

1. Allow multiple officials to use the same task link at the same time.
1. Prevent conflicting or duplicate entries on the server.
1. If an entry is rejected due to a conflict, tell the official and show the current state.
1. Officials coordinate who records which competitor; the app does not require reserving turns.

