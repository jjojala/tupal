/* Copyright (C) 2026 Jari Ojala (jari.ojala@iki.fi) */
import type { CompetitionClass, Competitor } from "./types";

/** Converts the supported ISO 8601 start-time offset format to seconds. */
export function durationToSeconds(duration: string) {
  const match = duration.match(/^PT(\d+)\.(\d{3})S$/);
  if (!match) throw new Error(`Unsupported start-time offset: ${duration}`);
  return Number(match[1]) + Number(match[2]) / 1000;
}

/** Converts seconds to the ISO 8601 duration format used by the API. */
export function secondsToDuration(seconds: number) {
  const wholeSeconds = Math.floor(seconds);
  const milliseconds = Math.round((seconds - wholeSeconds) * 1000);
  const normalizedSeconds = wholeSeconds + Math.floor(milliseconds / 1000);
  const normalizedMilliseconds = milliseconds % 1000;
  return `PT${normalizedSeconds}.${String(normalizedMilliseconds).padStart(3, "0")}S`;
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
