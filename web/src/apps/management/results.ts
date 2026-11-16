import { durationToSeconds, wallClockToTimestamp } from "../../shared/competitors";
import type { DerivedFinish } from "../../shared/finishAssignments";
import type { CompetitionClass, Competitor, StartGroup } from "../../shared/types";

export interface ResultRow {
  competitor: Competitor;
  kind: "finished" | "open" | "dnf" | "dsq" | "dns" | "review";
  elapsedMs?: number;
  rank?: number;
  finishTime?: string;
  error?: string;
}

export function classResults(
  classInfo: CompetitionClass,
  competitors: Competitor[],
  startGroups: StartGroup[],
  assignments: DerivedFinish[],
  now: number
): ResultRow[] {
  const group = startGroups.find((item) => item.id === classInfo.start_group_id);
  const finishByCompetitor = new Map(
    assignments.map(({ competitor, timeEntry }) => [competitor.id, timeEntry.finish_time])
  );
  const rows: ResultRow[] = competitors
    .filter((competitor) => competitor.comp_class_id === classInfo.id)
    .map((competitor): ResultRow => {
      const finishTime = finishByCompetitor.get(competitor.id);
      if (competitor.status === 2) return { competitor, kind: "dnf", finishTime };
      if (competitor.status === 3) return { competitor, kind: "dsq", finishTime };
      if (competitor.status === 1) return { competitor, kind: "dns", finishTime };
      if (competitor.status !== 0) {
        return { competitor, kind: "review", error: `Unknown status ${competitor.status}` };
      }
      if (!group || !Number.isFinite(Date.parse(group.first_start_time))) {
        return { competitor, kind: "review", error: "Invalid or missing start group" };
      }
      let offset: number;
      try {
        offset = durationToSeconds(competitor.start_time_offset) * 1000;
      } catch (error) {
        return { competitor, kind: "review", error: error instanceof Error ? error.message : "Invalid start offset" };
      }
      const start = Date.parse(group.first_start_time) + offset;
      if (finishTime) {
        const elapsedMs = Date.parse(finishTime) - start;
        if (!Number.isFinite(elapsedMs) || elapsedMs < 0) {
          return { competitor, kind: "review", finishTime, error: "Finish time is before start or invalid" };
        }
        return { competitor, kind: "finished", elapsedMs, finishTime };
      }
      const localStart = wallClockToTimestamp(group.first_start_time) + offset;
      if (!Number.isFinite(localStart)) {
        return { competitor, kind: "review", error: "Invalid start time" };
      }
      return { competitor, kind: "open", elapsedMs: Math.max(0, now - localStart) };
    });

  const finished = rows.filter((row) => row.kind === "finished")
    .sort((a, b) => a.elapsedMs! - b.elapsedMs! || a.competitor.bib - b.competitor.bib);
  finished.forEach((row, index) => {
    row.rank = index > 0 && row.elapsedMs === finished[index - 1].elapsedMs
      ? finished[index - 1].rank
      : index + 1;
  });
  const timed = rows.filter((row) => row.kind === "finished" || row.kind === "open")
    .sort((a, b) => a.elapsedMs! - b.elapsedMs! || a.competitor.bib - b.competitor.bib);
  return [
    ...timed,
    ...rows.filter((row) => row.kind === "dnf"),
    ...rows.filter((row) => row.kind === "dsq"),
    ...rows.filter((row) => row.kind === "dns"),
    ...rows.filter((row) => row.kind === "review")
  ];
}

export function formatElapsed(milliseconds: number): string {
  const total = Math.floor(milliseconds);
  const hours = Math.floor(total / 3600000);
  const minutes = Math.floor(total / 60000) % 60;
  const seconds = Math.floor(total / 1000) % 60;
  return `${hours}:${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}.${String(total % 1000).padStart(3, "0")}`;
}
