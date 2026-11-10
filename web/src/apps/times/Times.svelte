<script lang="ts">
  import { onMount } from "svelte";
  import {
    createFinishTimeEntry,
    getFinishBibEntries,
    getCompetition,
    getFinishTimeEntries,
    subscribeToCompetition
  } from "../../shared/api";
  import type {
    Competition,
    FinishBibEntry,
    FinishTimeEntry,
    FinishTimeEntryInput
  } from "../../shared/types";

  export let competitionId: string;

  let competition: Competition | undefined;
  let timeEntries: FinishTimeEntry[] = [];
  let bibEntries: FinishBibEntry[] = [];
  let loading = true;
  let pendingSaves = 0;
  let pageError = "";
  let notice = "";
  let connected = false;
  let clock = new Date();

  $: timesAhead = timeEntries.length - bibEntries.length;
  $: latestEntries = [...timeEntries]
    .sort((left, right) => right.finish_time.localeCompare(left.finish_time))
    .slice(0, 8);

  onMount(() => {
    void reload();
    const clockTimer = window.setInterval(() => (clock = new Date()), 100);
    const unsubscribe = subscribeToCompetition(
      competitionId,
      () => void reload(),
      (state) => (connected = state)
    );
    return () => {
      window.clearInterval(clockTimer);
      unsubscribe();
    };
  });

  async function reload() {
    pageError = "";
    try {
      const [competitionData, times, bibs] = await Promise.all([
        getCompetition(competitionId),
        getFinishTimeEntries(competitionId),
        getFinishBibEntries(competitionId)
      ]);
      competition = competitionData;
      timeEntries = times;
      bibEntries = bibs;
    } catch (error) {
      pageError = errorMessage(error);
    } finally {
      loading = false;
    }
  }

  async function recordTime() {
    pendingSaves += 1;
    pageError = "";
    notice = "";
    const capturedAt = new Date();
    const entry: FinishTimeEntryInput = {
      id: null,
      finish_time: formatLocalTimestamp(capturedAt),
      status: 0
    };

    try {
      const created = await createFinishTimeEntry(competitionId, entry);
      timeEntries = [
        ...timeEntries.filter((item) => item.id !== created.id),
        created
      ];
      notice = `Time ${displayTime(created.finish_time)} recorded.`;
    } catch (error) {
      pageError = errorMessage(error);
    } finally {
      pendingSaves -= 1;
    }
  }

  function formatLocalTimestamp(value: Date) {
    const pad = (part: number, size = 2) => String(part).padStart(size, "0");
    return `${value.getFullYear()}-${pad(value.getMonth() + 1)}-${pad(value.getDate())}` +
      `T${pad(value.getHours())}:${pad(value.getMinutes())}:${pad(value.getSeconds())}` +
      `.${pad(value.getMilliseconds(), 3)}Z`;
  }

  function displayTime(value: string) {
    const match = value.match(/T(\d{2}:\d{2}:\d{2}(?:\.\d{1,3})?)/);
    return match?.[1] ?? value;
  }

  function errorMessage(error: unknown) {
    return error instanceof Error ? error.message : "An unexpected error occurred.";
  }

  function aheadMessage(count: number) {
    if (count > 0) {
      return `${count} ${count === 1 ? "time" : "times"} ahead of bib numbers`;
    }
    if (count < 0) {
      const bibsAhead = Math.abs(count);
      return `${bibsAhead} ${bibsAhead === 1 ? "bib number" : "bib numbers"} ahead of times`;
    }
    return "Times and bib numbers are in sync";
  }
</script>

<svelte:head>
  <title>{competition ? `Times · ${competition.title}` : "Times · Tupal"}</title>
</svelte:head>

<div class="app-shell">
  <header class="topbar">
    <a class="brand" href="/">
      <span class="brand-mark" aria-hidden="true">T</span>
      <span>Tupal</span>
    </a>
    <span class="task-label"><span class="task-dot"></span>Finish times</span>
    <span class:online={connected} class="connection-state">
      <span class="connection-dot"></span>
      {connected ? "Live" : "Offline"}
    </span>
  </header>

  <main class="page times-page">
    {#if loading}
      <section class="loading-card" aria-live="polite">Loading competition…</section>
    {:else if competition}
      <div class="page-heading times-heading">
        <div>
          <p class="eyebrow">FINISH LINE</p>
          <h1>{competition.title}</h1>
          <p class="subheading">Tap once for each finisher crossing the line.</p>
        </div>
      </div>

      {#if pageError}
        <div class="message-card error-message" role="alert">{pageError}</div>
      {/if}

      <section class="times-layout" aria-label="Finish time recording">
        <div class="panel capture-panel">
          <p class="eyebrow">CURRENT TIME</p>
          <p class="live-clock" aria-label="Current local time">
            {clock.toLocaleTimeString(undefined, {
              hour: "2-digit",
              minute: "2-digit",
              second: "2-digit",
              hour12: false
            })}
            <span>.{String(clock.getMilliseconds()).padStart(3, "0")}</span>
          </p>
          <button
            class="record-button"
            type="button"
            aria-busy={pendingSaves > 0}
            onclick={recordTime}
          >
            Record finish time
          </button>
          <p class="capture-hint">The time is saved immediately to this competition.</p>
          {#if pendingSaves > 0}
            <p class="capture-hint" aria-live="polite">
              Saving {pendingSaves} {pendingSaves === 1 ? "time" : "times"}…
            </p>
          {/if}
          {#if notice}
            <p class="success-message" role="status">{notice}</p>
          {/if}
        </div>

        <div class="panel queue-panel">
          <div class="queue-count" aria-live="polite">
            <span class="queue-count-number">{Math.abs(timesAhead)}</span>
            <span>{aheadMessage(timesAhead)}</span>
          </div>
          <div class="queue-totals">
            <div>
              <span class="queue-total-number">{timeEntries.length}</span>
              <span>times</span>
            </div>
            <div>
              <span class="queue-total-number">{bibEntries.length}</span>
              <span>bib numbers</span>
            </div>
          </div>
          <p class="queue-explanation">
            Queue difference is calculated from recorded finish times and bib entries.
          </p>
        </div>
      </section>

      <section class="panel recent-panel" aria-labelledby="recent-heading">
        <div class="panel-heading">
          <h2 id="recent-heading">Latest recorded times</h2>
          <span class="entry-count">{timeEntries.length} total</span>
        </div>
        {#if latestEntries.length === 0}
          <p class="empty-state">No finish times recorded yet.</p>
        {:else}
          <ol class="time-list">
            {#each latestEntries as entry (entry.id)}
              <li>
                <span class="time-list-index">{timeEntries.length - timeEntries.indexOf(entry)}</span>
                <time datetime={entry.finish_time}>{displayTime(entry.finish_time)}</time>
              </li>
            {/each}
          </ol>
        {/if}
      </section>
    {:else}
      <section class="message-card error-message" role="alert">
        {pageError || "This competition could not be loaded."}
      </section>
    {/if}
  </main>
</div>

<style>
  .times-page {
    max-width: 900px;
  }

  .times-heading {
    margin-bottom: 22px;
  }

  .times-layout {
    display: grid;
    grid-template-columns: minmax(0, 1.15fr) minmax(260px, 0.85fr);
    gap: 20px;
  }

  .capture-panel,
  .queue-panel {
    min-height: 270px;
  }

  .capture-panel {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    text-align: center;
  }

  .capture-panel .eyebrow {
    margin-bottom: 10px;
  }

  .live-clock {
    margin: 0 0 24px;
    font-variant-numeric: tabular-nums;
    font-size: clamp(36px, 8vw, 58px);
    font-weight: 750;
    letter-spacing: -0.05em;
  }

  .live-clock span {
    color: var(--color-muted);
    font-size: 0.55em;
    letter-spacing: -0.02em;
  }

  .record-button {
    width: 100%;
    min-height: 84px;
    border: 0;
    border-radius: 12px;
    background: var(--color-primary);
    color: white;
    font-size: 21px;
    font-weight: 750;
    touch-action: manipulation;
  }

  .record-button:hover:not(:disabled) {
    background: var(--color-primary-hover);
  }

  .record-button:disabled {
    cursor: wait;
    opacity: 0.72;
  }

  .capture-hint,
  .queue-explanation {
    margin: 12px 0 0;
    color: var(--color-muted);
    font-size: 13px;
    text-align: center;
  }

  .success-message {
    margin: 12px 0 0;
    color: var(--color-primary);
    font-size: 14px;
    font-weight: 650;
  }

  .queue-panel {
    display: flex;
    flex-direction: column;
    justify-content: center;
    text-align: center;
  }

  .queue-count {
    display: grid;
    justify-items: center;
    gap: 4px;
    color: var(--color-primary);
    font-size: 16px;
    font-weight: 700;
  }

  .queue-count-number {
    font-variant-numeric: tabular-nums;
    font-size: 54px;
    line-height: 1;
  }

  .queue-totals {
    display: flex;
    justify-content: center;
    gap: 48px;
    margin-top: 24px;
    padding-top: 20px;
    border-top: 1px solid var(--color-border);
  }

  .queue-totals > div {
    display: grid;
    gap: 3px;
    color: var(--color-muted);
    font-size: 13px;
  }

  .queue-total-number {
    color: var(--color-ink);
    font-size: 20px;
    font-weight: 700;
    font-variant-numeric: tabular-nums;
  }

  .recent-panel {
    margin-top: 20px;
  }

  .recent-panel .panel-heading {
    margin-bottom: 12px;
  }

  .entry-count {
    color: var(--color-muted);
    font-size: 13px;
  }

  .time-list {
    display: grid;
    gap: 8px;
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .time-list li {
    display: flex;
    align-items: center;
    gap: 14px;
    padding: 12px 14px;
    border-radius: 9px;
    background: #f7f9f6;
  }

  .time-list-index {
    width: 28px;
    color: var(--color-muted);
    font-size: 13px;
    text-align: right;
  }

  .time-list time {
    font-size: 18px;
    font-weight: 650;
    font-variant-numeric: tabular-nums;
  }

  .empty-state {
    margin: 0;
    padding: 18px 0 4px;
    color: var(--color-muted);
    font-size: 14px;
  }

  .error-message {
    margin-bottom: 18px;
    padding: 14px 18px;
    color: var(--color-danger);
  }

  @media (max-width: 680px) {
    .times-page {
      padding: 28px 16px 40px;
    }

    .times-layout {
      grid-template-columns: 1fr;
      gap: 14px;
    }

    .capture-panel {
      min-height: 250px;
      padding: 20px;
    }

    .queue-panel {
      min-height: 220px;
    }

    .recent-panel {
      margin-top: 14px;
      padding: 20px;
    }

    .record-button {
      min-height: 96px;
    }
  }
</style>
