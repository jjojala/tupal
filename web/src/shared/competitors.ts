/* Copyright (C) 2026 Jari Ojala (jari.ojala@iki.fi) */
import type { CompetitionClass, Competitor, StartGroup } from "./types";

/** Converts an ISO 8601 duration in seconds, with optional millisecond precision. */
export function durationToSeconds(duration: string) {
  const match = duration.match(/^PT(\d+)(?:\.(\d{1,3}))?S$/);
  if (!match) throw new Error(`Unsupported start-time offset: ${duration}`);
  const fractionalSeconds = match[2] ? Number(`0.${match[2]}`) : 0;
  return Number(match[1]) + fractionalSeconds;
}

/** Converts seconds to the ISO 8601 duration format used by the API. */
export function secondsToDuration(seconds: number) {
  const wholeSeconds = Math.floor(seconds);
  const milliseconds = Math.round((seconds - wholeSeconds) * 1000);
  const normalizedSeconds = wholeSeconds + Math.floor(milliseconds / 1000);
  const normalizedMilliseconds = milliseconds % 1000;
  return `PT${normalizedSeconds}.${String(normalizedMilliseconds).padStart(3, "0")}S`;
}

/** Finds the lowest unused bib at or above the target start group's first bib. */
export function nextAvailableBib(
  competitors: Competitor[],
  classes: CompetitionClass[],
  startGroups: StartGroup[],
  classId: string
) {
  const targetClass = classes.find((item) => item.id === classId);
  const startGroup = startGroups.find(
    (item) => item.id === targetClass?.start_group_id
  );
  if (
    !startGroup ||
    !Number.isInteger(startGroup.first_bib) ||
    startGroup.first_bib < 1
  ) {
    return undefined;
  }

  const usedBibs = new Set(
    competitors.map((competitor) => competitor.bib).filter((value) => value > 0)
  );
  for (let bib = startGroup.first_bib; bib <= 65535; bib += 1) {
    if (!usedBibs.has(bib)) return bib;
  }
  return undefined;
}

/** Calculates the start offset assigned to a bib by its start group. */
export function startOffsetForBib(
  classes: CompetitionClass[],
  startGroups: StartGroup[],
  classId: string,
  bib: number
) {
  const targetClass = classes.find((item) => item.id === classId);
  const startGroup = startGroups.find(
    (item) => item.id === targetClass?.start_group_id
  );
  if (!startGroup || !Number.isInteger(startGroup.first_bib) || startGroup.first_bib < 1) {
    return undefined;
  }

  return (bib - startGroup.first_bib) * durationToSeconds(startGroup.start_interval);
}

/** Checks whether another competitor in the target start group has this bib. */
export function hasDuplicateBib(
  competitors: Competitor[],
  classes: CompetitionClass[],
  bib: number,
  classId: string,
  exceptId?: string
) {
  const classById = new Map(classes.map((item) => [item.id, item]));
  const targetClass = classById.get(classId);
  if (!targetClass) return false;
  return competitors.some((competitor) => {
    if (competitor.id === exceptId || competitor.bib !== bib) return false;
    const existingClass = classById.get(competitor.comp_class_id);
    return existingClass?.start_group_id === targetClass?.start_group_id;
  });
}
