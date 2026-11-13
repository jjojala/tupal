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

/** Calculates the start clock time (as entered in setup) for a competitor offset, to whole seconds. */
export function startClockForOffset(
  classes: CompetitionClass[],
  startGroups: StartGroup[],
  classId: string,
  offset: string
) {
  const targetClass = classes.find((item) => item.id === classId);
  const startGroup = startGroups.find(
    (item) => item.id === targetClass?.start_group_id
  );
  if (!startGroup) return undefined;

  const firstStartTime = Date.parse(startGroup.first_start_time);
  if (!Number.isFinite(firstStartTime)) {
    throw new Error("The start group has an invalid first start time.");
  }
  const timestamp = Math.round(
    (firstStartTime + durationToSeconds(offset) * 1000) / 1000
  ) * 1000;
  const date = new Date(timestamp);
  return `${pad(date.getUTCHours())}:${pad(date.getUTCMinutes())}:${pad(date.getUTCSeconds())}`;
}

/** Calculates the stored start offset from a setup-style clock time. */
export function startOffsetForStartClock(
  classes: CompetitionClass[],
  startGroups: StartGroup[],
  classId: string,
  startClock: string
) {
  const targetClass = classes.find((item) => item.id === classId);
  const startGroup = startGroups.find(
    (item) => item.id === targetClass?.start_group_id
  );
  if (!startGroup) return undefined;

  const firstStartTime = Date.parse(startGroup.first_start_time);
  const clockMatch = startClock.match(/^([01]\d|2[0-3]):([0-5]\d):([0-5]\d)$/);
  if (!Number.isFinite(firstStartTime) || !clockMatch) {
    throw new Error("Enter a valid start time.");
  }
  const firstStart = new Date(firstStartTime);
  let absoluteStart = Date.UTC(
    firstStart.getUTCFullYear(),
    firstStart.getUTCMonth(),
    firstStart.getUTCDate(),
    Number(clockMatch[1]),
    Number(clockMatch[2]),
    Number(clockMatch[3])
  );
  if (absoluteStart < firstStartTime) {
    absoluteStart += 24 * 60 * 60 * 1000;
  }
  return (absoluteStart - firstStartTime) / 1000;
}

/**
 * Converts a stored wall-clock timestamp (local time written with a "Z"
 * suffix) to a real epoch time in the browser's timezone, so that it can be
 * compared with `Date.now()`. Returns NaN for invalid input.
 */
export function wallClockToTimestamp(value: string) {
  const utc = Date.parse(value);
  if (!Number.isFinite(utc)) return Number.NaN;
  const d = new Date(utc);
  return new Date(
    d.getUTCFullYear(),
    d.getUTCMonth(),
    d.getUTCDate(),
    d.getUTCHours(),
    d.getUTCMinutes(),
    d.getUTCSeconds(),
    d.getUTCMilliseconds()
  ).getTime();
}

function pad(value: number) {
  return String(value).padStart(2, "0");
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
