/* Copyright (C) 2026 Jari Ojala  */
import type {
  Competition,
  CompetitionClass,
  CompetitionClassInput,
  Competitor,
  CompetitorInput,
  FinishBibEntry,
  FinishBibEntryInput,
  FinishTimeEntry,
  FinishTimeEntryInput,
  StartGroup,
  StartGroupInput
} from "./types";

const competitionUrl = (competitionId: string) =>
  `/rest/competition/${encodeURIComponent(competitionId)}`;

/** Sends a JSON REST request and returns its decoded response. */
async function request<T>(url: string, init?: RequestInit): Promise<T> {
  const headers = new Headers(init?.headers);
  headers.set("Accept", "application/json");
  if (init?.body) {
    headers.set("Content-Type", "application/json");
  }

  const response = await fetch(url, { ...init, headers });
  const responseText = await response.text();
  let payload: unknown;
  try {
    payload = responseText ? JSON.parse(responseText) : undefined;
  } catch {
    if (!response.ok) {
      throw new Error(responseText || `Request failed (${response.status})`);
    }
    throw new Error("The server returned an invalid response.");
  }

  if (!response.ok) {
    let message = `Request failed (${response.status} ${response.statusText})`;
    if (typeof payload === "string") {
      message = payload;
    } else if (
      typeof payload === "object" &&
      payload !== null &&
      "message" in payload &&
      typeof payload.message === "string"
    ) {
      message = payload.message;
    }
    throw new Error(message);
  }

  return payload as T;
}

/** Fetches the competition identified by `competitionId`. */
export function getCompetition(competitionId: string) {
  return request<Competition>(competitionUrl(competitionId));
}

/** Fetches all competitions available to the management app. */
export function getCompetitions() {
  return request<Competition[]>("/rest/competition/");
}

/** Creates a competition with an API-generated identifier. */
export function createCompetition(title: string, date: string) {
  return request<Competition>("/rest/competition/", {
    method: "POST",
    body: JSON.stringify({ id: null, title, date })
  });
}

/** Fetches the classes configured for a competition. */
export function getClasses(competitionId: string) {
  return request<CompetitionClass[]>(
    `${competitionUrl(competitionId)}/competition_class/`
  );
}

/** Fetches the start groups configured for a competition. */
export function getStartGroups(competitionId: string) {
  return request<StartGroup[]>(
    `${competitionUrl(competitionId)}/start_group/`
  );
}

/** Creates a start group in a competition. */
export function createStartGroup(
  competitionId: string,
  startGroup: StartGroupInput
) {
  return request<StartGroup>(
    `${competitionUrl(competitionId)}/start_group/`,
    {
      method: "POST",
      body: JSON.stringify(startGroup)
    }
  );
}

/** Updates a start group in a competition. */
export function updateStartGroup(
  competitionId: string,
  startGroup: StartGroupInput
) {
  if (!startGroup.id) throw new Error("Cannot update a start group without an id");
  return request<StartGroup>(
    `${competitionUrl(competitionId)}/start_group/${encodeURIComponent(startGroup.id)}`,
    {
      method: "PUT",
      body: JSON.stringify(startGroup)
    }
  );
}

/** Deletes a start group from a competition. */
export function deleteStartGroup(competitionId: string, startGroupId: string) {
  return request<unknown>(
    `${competitionUrl(competitionId)}/start_group/${encodeURIComponent(startGroupId)}`,
    { method: "DELETE" }
  );
}

/** Creates a class in a competition. */
export function createCompetitionClass(
  competitionId: string,
  competitionClass: CompetitionClassInput
) {
  return request<CompetitionClass>(
    `${competitionUrl(competitionId)}/competition_class/`,
    {
      method: "POST",
      body: JSON.stringify(competitionClass)
    }
  );
}

/** Updates a class in a competition. */
export function updateCompetitionClass(
  competitionId: string,
  competitionClass: CompetitionClassInput
) {
  if (!competitionClass.id) {
    throw new Error("Cannot update a class without an id");
  }
  return request<CompetitionClass>(
    `${competitionUrl(competitionId)}/competition_class/${encodeURIComponent(competitionClass.id)}`,
    {
      method: "PUT",
      body: JSON.stringify(competitionClass)
    }
  );
}

/** Deletes a class from a competition. */
export function deleteCompetitionClass(
  competitionId: string,
  classId: string
) {
  return request<unknown>(
    `${competitionUrl(competitionId)}/competition_class/${encodeURIComponent(classId)}`,
    { method: "DELETE" }
  );
}

/** Fetches the competitors registered for a competition. */
export function getCompetitors(competitionId: string) {
  return request<Competitor[]>(
    `${competitionUrl(competitionId)}/competitor/`
  );
}

/** Creates a competitor in a competition. */
export function createCompetitor(
  competitionId: string,
  competitor: CompetitorInput
) {
  return request<Competitor>(`${competitionUrl(competitionId)}/competitor/`, {
    method: "POST",
    body: JSON.stringify(competitor)
  });
}

/** Updates an existing competitor in a competition. */
export function updateCompetitor(
  competitionId: string,
  competitor: CompetitorInput
) {
  if (!competitor.id) {
    throw new Error("Cannot update a competitor without an id");
  }

  return request<Competitor>(
    `${competitionUrl(competitionId)}/competitor/${encodeURIComponent(competitor.id)}`,
    {
      method: "PUT",
      body: JSON.stringify(competitor)
    }
  );
}

/** Deletes a competitor from a competition. */
export function deleteCompetitor(competitionId: string, competitorId: string) {
  return request<unknown>(
    `${competitionUrl(competitionId)}/competitor/${encodeURIComponent(competitorId)}`,
    { method: "DELETE" }
  );
}

/** Fetches the finish-time queue for a competition. */
export function getFinishTimeEntries(competitionId: string) {
  return request<FinishTimeEntry[]>(
    `${competitionUrl(competitionId)}/finish_time_entry/`
  );
}

/** Fetches the finish-bib queue for a competition. */
export function getFinishBibEntries(competitionId: string) {
  return request<FinishBibEntry[]>(
    `${competitionUrl(competitionId)}/finish_bib_entry/`
  );
}

/** Records a bib in the competition's finish-order queue. */
export function createFinishBibEntry(
  competitionId: string,
  entry: FinishBibEntryInput
) {
  return request<FinishBibEntry>(
    `${competitionUrl(competitionId)}/finish_bib_entry/`,
    {
      method: "POST",
      body: JSON.stringify(entry)
    }
  );
}

/** Records a finish time in the competition's time queue. */
export function createFinishTimeEntry(
  competitionId: string,
  entry: FinishTimeEntryInput
) {
  return request<FinishTimeEntry>(
    `${competitionUrl(competitionId)}/finish_time_entry/`,
    {
      method: "POST",
      body: JSON.stringify(entry)
    }
  );
}

/**
 * Subscribes to competition updates and connection-state changes.
 * @returns A function that closes the subscription and stops reconnect attempts.
 */
export function subscribeToCompetition(
  competitionId: string,
  onUpdate: () => void,
  onConnectionChange: (connected: boolean) => void
) {
  let socket: WebSocket | undefined;
  let retryTimer: number | undefined;
  let closed = false;

  const connect = () => {
    if (closed) return;

    const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
    socket = new WebSocket(
      `${protocol}//${window.location.host}/ws/${encodeURIComponent(competitionId)}`
    );
    socket.onopen = () => onConnectionChange(true);
    socket.onmessage = () => onUpdate();
    socket.onerror = () => onConnectionChange(false);
    socket.onclose = () => {
      onConnectionChange(false);
      if (!closed) retryTimer = window.setTimeout(connect, 2000);
    };
  };

  connect();

  return () => {
    closed = true;
    if (retryTimer !== undefined) window.clearTimeout(retryTimer);
    socket?.close();
  };
}
