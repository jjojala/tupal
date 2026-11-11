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
  /** Name displayed for the start group. */
  title: string;
  /** First scheduled start time. */
  first_start_time: string;
  /** First bib number in this start group, or zero when bibs are not assigned. */
  first_bib: number;
  /** Time between consecutive bibs, as an ISO 8601 duration. */
  start_interval: string;
}

/** Fields accepted when creating or updating a start group. */
export interface StartGroupInput {
  /** Existing identifier, or null to request a generated identifier. */
  id: string | null;
  /** Start-group display name. */
  title: string;
  /** First scheduled start time in local ISO date-time format. */
  first_start_time: string;
  /** First bib number, or zero when bib numbers are not sequenced. */
  first_bib: number;
  /** Interval between consecutive starts as an ISO 8601 duration. */
  start_interval: string;
}

/** Fields accepted when creating or updating a competition class. */
export interface CompetitionClassInput {
  /** Existing identifier, or null to request a generated identifier. */
  id: string | null;
  /** Class display name. */
  title: string;
  /** Start group assigned to this class. */
  start_group_id: string;
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

/** A time recorded by a finish-line timekeeper. */
export interface FinishTimeEntry {
  /** Unique finish-time entry identifier. */
  id: string;
  /** Competition containing the entry. */
  comp_id: string;
  /** Recorded finish time as an ISO date-time string. */
  finish_time: string;
  /** Finish observation status: 0-confirmed, 1-uncertain, 2-incorrect, 3-missing. */
  status: number;
}

/** A bib observation recorded by the numbers official. */
export interface FinishBibEntry {
  /** Unique finish-bib entry identifier. */
  id: string;
  /** Competition containing the entry. */
  comp_id: string;
  /** Position in the bib-entry queue. */
  sequence_no: number;
  /** Read bib number, or null if not identified. */
  bib: number | null;
  /** Finish observation status. */
  status: number;
  /** Linked finish-time entry, or null if not paired. */
  confirmed_time_entry_id: string | null;
}

/** Fields accepted when recording a finish time. */
export interface FinishTimeEntryInput {
  /** Optional client-generated identifier, or null to request a generated identifier. */
  id: string | null;
  /** Recorded finish time as an ISO date-time string. */
  finish_time: string;
  /** Finish observation status. */
  status: number;
}

/** Fields accepted when recording a bib in the finish-order queue. */
export interface FinishBibEntryInput {
  /** Optional client-generated identifier, or null to request a generated identifier. */
  id: string | null;
  /** Position in the bib-entry queue. */
  sequence_no: number;
  /** Read bib number, or null if not identified. */
  bib: number | null;
  /** Finish observation status. */
  status: number;
  /** Linked finish-time entry, or null if not paired. */
  confirmed_time_entry_id: string | null;
}
