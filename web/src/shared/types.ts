/* Copyright (C) 2026 Jari Ojala (jari.ojala@iki.fi) */

/** Competition data returned by the REST API. */
export interface Competition {
  /** Unique competition identifier. */
  id: string;
  /** Competition name displayed to users. */
  title: string;
  /** Competition date in ISO 8601 format. */
  date: string;
}

/** A class configured for a competition. */
export interface CompetitionClass {
  /** Unique class identifier within the competition. */
  id: string;
  /** Class name displayed to users. */
  title: string;
  /** Identifier of the start group assigned to this class. */
  start_group_id: string;
}

/** A start group and its bib-number sequence configuration. */
export interface StartGroup {
  /** Unique start-group identifier within the competition. */
  id: string;
  /** First scheduled start time. */
  first_start_time: string;
  /** First bib number in this start group, or zero when bibs are not assigned. */
  first_bib: number;
  /** Time between consecutive bibs, as an ISO 8601 duration. */
  start_interval: string;
}

/** Competitor data returned by the REST API. */
export interface Competitor {
  /** Unique competitor identifier. */
  id: string;
  /** Identifier of the competition this competitor belongs to. */
  comp_id: string;
  /** Identifier of the competitor's class. */
  comp_class_id: string;
  /** Assigned bib number, or zero when no number is assigned. */
  bib: number;
  /** Competitor's name. */
  name: string;
  /** Start-time offset from the start group, as an ISO 8601 duration. */
  start_time_offset: string;
  /** Finish time as an ISO 8601 datetime, or an empty string if unset. */
  finish_time: string;
  /** Competition status code. */
  status: number;
}

/** Fields accepted when creating or updating a competitor. */
export interface CompetitorInput {
  /** Existing competitor identifier, or null to request a generated identifier. */
  id: string | null;
  /** Identifier of the competitor's class. */
  comp_class_id: string;
  /** Bib number, or zero when no number is assigned. */
  bib: number;
  /** Competitor's name. */
  name: string;
  /** Start-time offset from the start group, as an ISO 8601 duration. */
  start_time_offset: string;
  /** Finish time as an ISO 8601 datetime, or an empty string if unset. */
  finish_time: string;
  /** Competition status code. */
  status: number;
}
