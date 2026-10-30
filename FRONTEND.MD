# Epic: Management
1. Save the mark and update the competitor’s status.

## START-4 — Create DNS suggestions automatically
1. Automatically create a DNS suggestion when a competitor’s start time has passed and they have not been marked as started.
1. A DNS suggestion is not a confirmed DNS and is not shown outside the Start app.
1. Keep the suggestion visible in Start until it is confirmed or the competitor is marked as started.

## START-5 — Handle DNS suggestions
1. Show DNS suggestions in a compact list or separate view so they do not obscure upcoming starters.
1. Show at least the competitor’s bib number; the start time is not required.
1. Let the official confirm a suggestion as DNS.
1. Let the official mark the competitor as started, which cancels the suggestion.
1. A confirmed DNS updates the DNS and Open totals in Management.

# Epic: Times and Numbers

## TIME/NUM-1 — Open a task-specific mobile app from a link
1. Open the competition-specific Times, Numbers, or combined view directly from a link.
1. Do not require competition selection in the app.
1. Sign-in is out of scope for this phase.

## TIMES-2 — Record a finish time
1. Let the timekeeper record a finish time quickly on a mobile device.
1. Clearly display the most recently recorded time.

## NUMBERS-3 — Record bib numbers in finish order
1. Let the official record bib numbers in the order competitors finish.
1. Allow entry either by selecting a suggested number or by typing one directly.

## NUMBERS-4 — Suggest likely bib numbers
1. Suggest likely next bib numbers based on the start list.
1. By default, hide numbers that have already been entered from the quick-select suggestions.
1. Keep all numbers searchable and available for manual entry.

## TIME/NUM-5 — Mark an entry’s status
1. Let the official mark a time or number entry as uncertain or incorrect.
1. Let the official record a missing time or number as a separate event, preserving the sequence.
1. Changing an entry’s status must not delete the original event.

## TIME/NUM-6 — Show the relationship between time and number queues

1. In Times and Numbers, show how many events remain unmatched between the two queues.
1. For example, show extra time entries as awaiting bib numbers, and extra number entries as awaiting times.
1. Queue matching status and connection status are out of scope for this phase; a stable internet connection is assumed.

# Epic: Finishline

## FINISH-1 — Record times and numbers in one view

1. Let one official record both times and bib numbers in a single mobile view.
1. The combined view must create time and number entries compatible with the same event queues used by the separate apps.
1. Support uncertain, incorrect, and missing entries.

## FINISH-2 — Record a DSQ

1. Let the official assign a DSQ to an identified competitor directly in Finishline.
1. A DSQ is a final decision; there is no separate proposal stage.
1. Require a short, single-line text reason for the DSQ.
1. Preserve the competitor’s time after the DSQ is recorded.
1. Show the DSQ as a result in Management’s totals and results.

# Epic: Event History and Concurrent Use

## DATA-1 — Store competition events

1. Store timing, number-entry, start, DNS, and DSQ events as distinguishable event types.
1. Preserve event order and timestamps.
1. Corrections and cancellations must not delete the original event history.

## DATA-2 — Handle concurrent entries

1. Allow multiple officials to use the same task link at the same time.
1. Prevent conflicting or duplicate entries on the server.
1. If an entry is rejected due to a conflict, tell the official and show the current state.
1. Officials coordinate who records which competitor; the app does not require reserving turns.
Out of scope for this phase: offline storage and later synchronization. The apps assume a stable internet connection.
