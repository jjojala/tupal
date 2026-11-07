export interface Competition {
  id: string;
  title: string;
  date: string;
}

export interface CompetitionClass {
  id: string;
  title: string;
  start_group_id: string;
}

export interface Competitor {
  id: string;
  comp_id: string;
  comp_class_id: string;
  bib: number;
  name: string;
  start_time_offset: string;
  finish_time: string;
  status: number;
}

export interface CompetitorInput {
  id: string | null;
  comp_class_id: string;
  bib: number;
  name: string;
  start_time_offset: string;
  finish_time: string;
  status: number;
}
