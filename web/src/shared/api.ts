import type {
  Competition,
  CompetitionClass,
  Competitor,
  CompetitorInput
} from "./types";

const competitionUrl = (competitionId: string) =>
  `/rest/competition/${encodeURIComponent(competitionId)}`;

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

export function getCompetition(competitionId: string) {
  return request<Competition>(competitionUrl(competitionId));
}

export function getClasses(competitionId: string) {
  return request<CompetitionClass[]>(
    `${competitionUrl(competitionId)}/competition_class/`
  );
}

export function getCompetitors(competitionId: string) {
  return request<Competitor[]>(
    `${competitionUrl(competitionId)}/competitor/`
  );
}

export function createCompetitor(
  competitionId: string,
  competitor: CompetitorInput
) {
  return request<Competitor>(`${competitionUrl(competitionId)}/competitor/`, {
    method: "POST",
    body: JSON.stringify(competitor)
  });
}

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

export function deleteCompetitor(competitionId: string, competitorId: string) {
  return request<unknown>(
    `${competitionUrl(competitionId)}/competitor/${encodeURIComponent(competitorId)}`,
    { method: "DELETE" }
  );
}

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
