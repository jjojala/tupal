import type { Competitor, FinishBibEntry, FinishTimeEntry } from "./types";

export interface DerivedFinish {
  competitor: Competitor;
  bibEntry: FinishBibEntry;
  timeEntry: FinishTimeEntry;
}

export interface FinishAssignments {
  pairs: DerivedFinish[];
  blocker: string | null;
  waitingFor: "bib" | "time" | null;
}

/** Derive only the unambiguous prefix of the two finish queues. */
export function deriveFinishAssignments(
  competitors: Competitor[],
  bibEntries: FinishBibEntry[],
  timeEntries: FinishTimeEntry[]
): FinishAssignments {
  const bibs = [...bibEntries].sort((a, b) => a.sequence_no - b.sequence_no);
  const pairs: DerivedFinish[] = [];
  const assignedBibs = new Set<number>();
  const blocked = (message: string): FinishAssignments => ({
    pairs, blocker: message, waitingFor: null
  });
  if (timeEntries.some((time) => !Number.isFinite(Date.parse(time.finish_time)))) {
    return blocked("A time entry has an invalid timestamp.");
  }
  const times = [...timeEntries].sort((a, b) =>
    Date.parse(a.finish_time) - Date.parse(b.finish_time) || a.id.localeCompare(b.id)
  );

  for (let i = 0; i < Math.max(bibs.length, times.length); i++) {
    const bib = bibs[i];
    const time = times[i];
    if (bib && (bib.status !== 0 || bib.bib === null || bib.bib <= 0)) {
      return blocked(`Bib entry ${bib.sequence_no} needs review.`);
    }
    if (time && time.status !== 0) {
      return blocked(`Time entry ${i + 1} needs review.`);
    }
    if (time && times[i + 1] &&
        Date.parse(times[i + 1].finish_time) === Date.parse(time.finish_time)) {
      return blocked(`Time entry ${i + 1} has the same timestamp as another entry.`);
    }
    if (bib?.bib != null) {
      const matches = competitors.filter((competitor) => competitor.bib === bib.bib);
      if (matches.length !== 1 || assignedBibs.has(bib.bib)) {
        return blocked(`Bib ${bib.bib} does not identify a unique unassigned competitor.`);
      }
      if (time) {
        pairs.push({ competitor: matches[0], bibEntry: bib, timeEntry: time });
        assignedBibs.add(bib.bib);
      }
    }
    if (!bib || !time) {
      return { pairs, blocker: null, waitingFor: bib ? "time" : "bib" };
    }
  }
  return { pairs, blocker: null, waitingFor: null };
}
