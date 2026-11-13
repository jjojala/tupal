<script lang="ts">
  import { onMount } from "svelte";
  import {
    createFinishBibEntry,
    getClasses,
    getCompetition,
    getCompetitors,
    getFinishBibEntries,
    getFinishTimeEntries,
    getStartGroups,
    subscribeToCompetition
  } from "../../shared/api";
  import {
    durationToSeconds,
    wallClockToTimestamp
  } from "../../shared/competitors";
  import type {
    Competition,
    CompetitionClass,
    Competitor,
    FinishBibEntry,
    FinishBibEntryInput,
    FinishTimeEntry,
    StartGroup
  } from "../../shared/types";

  export let competitionId: string;

  let competition: Competition | undefined;
  let classes: CompetitionClass[] = [];
  let competitors: Competitor[] = [];
  let startGroups: StartGroup[] = [];
  let bibEntries: FinishBibEntry[] = [];
  let timeEntries: FinishTimeEntry[] = [];
  let search = "";
  let manualBib = "";
  let loading = true;
  let pendingSaves = 0;
  let pendingSequenceNos: number[] = [];
  let pageError = "";
  let notice = "";
  let connected = false;
  let now = Date.now();

  const recentEntryLimit = 12;

  $: enteredBibs = new Set(
    bibEntries.flatMap((entry) => entry.bib === null ? [] : [entry.bib])
  );
  $: suggestedCompetitors = [...competitors]
    .filter((competitor) =>
      competitor.bib > 0 &&
      competitor.status === 0 &&
      !enteredBibs.has(competitor.bib) &&
      startTimestamp(competitor) <= now
    )
    .sort((left, right) =>
      startTimestamp(left) - startTimestamp(right) || left.bib - right.bib
    )
    .slice(0, 8);
  $: matchingCompetitors = search.trim()
    ? [...competitors]
        .filter((competitor) =>
          String(competitor.bib).includes(search.trim()) ||
          competitor.name.toLocaleLowerCase().includes(search.trim().toLocaleLowerCase())
        )
        .sort((left, right) => left.bib - right.bib)
        .slice(0, 30)
    : [];
  $: recentEntries = [...bibEntries]
    .sort((left, right) =>
      right.sequence_no - left.sequence_no || right.id.localeCompare(left.id)
    )
    .slice(0, recentEntryLimit);
  $: queueDifference = bibEntries.length - timeEntries.length;

  onMount(() => {
    void reload();
    const timer = window.setInterval(() => (now = Date.now()), 1000);
    const unsubscribe = subscribeToCompetition(
      competitionId,
      () => void reload(),
      (state) => (connected = state)
    );
    return () => {
      window.clearInterval(timer);
      unsubscribe();
    };
  });

  async function reload() {
    pageError = "";
    try {
      const [
        competitionData,
        classData,
        competitorData,
        startGroupData,
        bibData,
        timeData
      ] = await Promise.all([
        getCompetition(competitionId),
        getClasses(competitionId),
        getCompetitors(competitionId),
        getStartGroups(competitionId),
        getFinishBibEntries(competitionId),
        getFinishTimeEntries(competitionId)
      ]);
      competition = competitionData;
      classes = classData;
      competitors = competitorData;
      startGroups = startGroupData;
      bibEntries = bibData;
      timeEntries = timeData;
    } catch (error) {
      pageError = errorMessage(error);
    } finally {
      loading = false;
    }
  }

  function startTimestamp(competitor: Competitor) {
    const classInfo = classes.find(
      (item) => item.id === competitor.comp_class_id
    );
    const startGroup = startGroups.find(
      (item) => item.id === classInfo?.start_group_id
    );
    if (!startGroup) return Number.POSITIVE_INFINITY;
    const firstStart = wallClockToTimestamp(startGroup.first_start_time);
    if (!Number.isFinite(firstStart)) return Number.POSITIVE_INFINITY;
    try {
      return firstStart + durationToSeconds(competitor.start_time_offset) * 1000;
    } catch {
      return Number.POSITIVE_INFINITY;
    }
  }

  async function recordBib(bib: number) {
    if (!Number.isInteger(bib) || bib < 1 || bib > 65535) {
      pageError = "Bib number must be a whole number between 1 and 65535.";
      return;
    }

    pageError = "";
    notice = "";
    const sequenceNo = nextSequenceNo();
    pendingSequenceNos = [...pendingSequenceNos, sequenceNo];
    pendingSaves += 1;
    const entry: FinishBibEntryInput = {
      id: null,
      sequence_no: sequenceNo,
      bib,
      status: 0,
      confirmed_time_entry_id: null
    };
    try {
      const created = await createFinishBibEntry(competitionId, entry);
      bibEntries = [
        ...bibEntries.filter((item) => item.id !== created.id),
        created
      ];
      if (manualBib === String(bib)) manualBib = "";
      notice = `Bib ${bib} recorded as number ${created.sequence_no}.`;
    } catch (error) {
      pageError = errorMessage(error);
    } finally {
      pendingSequenceNos = pendingSequenceNos.filter(
        (pendingSequenceNo) => pendingSequenceNo !== sequenceNo
      );
      pendingSaves -= 1;
    }
  }

  function nextSequenceNo() {
    const usedSequenceNos = new Set([
      ...bibEntries.map((entry) => entry.sequence_no),
      ...pendingSequenceNos
    ]);
    let sequenceNo = 1;
    while (usedSequenceNos.has(sequenceNo)) sequenceNo += 1;
    return sequenceNo;
  }

  function submitManualBib(event: SubmitEvent) {
    event.preventDefault();
    recordManualBib();
  }

  function recordManualBib() {
    const bib = Number(manualBib);
    if (!manualBib.trim()) {
      pageError = "Enter a bib number.";
      return;
    }
    void recordBib(bib);
  }

  function errorMessage(error: unknown) {
    return error instanceof Error ? error.message : "An unexpected error occurred.";
  }

  function queueMessage(count: number) {
    if (count > 0) {
      return `${count} ${count === 1 ? "bib number" : "bib numbers"} ahead of times`;
    }
    if (count < 0) {
      const timesAhead = Math.abs(count);
      return `${timesAhead} ${timesAhead === 1 ? "time" : "times"} ahead of bib numbers`;
    }
    return "Times and bib numbers are in sync";
  }
</script>

<svelte:head>
  <title>{competition ? `Numbers · ${competition.title}` : "Numbers · Tupal"}</title>
</svelte:head>

<div class="app-shell">
  <header class="topbar">
    <a class="brand" href="/">
      <span class="brand-mark" aria-hidden="true">T</span>
      <span>Tupal</span>
    </a>
    <span class="task-label"><span class="task-dot"></span>Finish numbers</span>
    <span class:online={connected} class="connection-state">
      <span class="connection-dot"></span>
      {connected ? "Live" : "Offline"}
    </span>
  </header>

  <main class="page numbers-page">
    {#if loading}
      <section class="loading-card" aria-live="polite">Loading competition…</section>
    {:else if competition}
      <div class="page-heading numbers-heading">
        <div>
          <p class="eyebrow">FINISH LINE</p>
          <h1>{competition.title}</h1>
          <p class="subheading">Record bib numbers in the order competitors finish.</p>
        </div>
      </div>

      {#if pageError}
        <div class="message-card error-message" role="alert">{pageError}</div>
      {/if}

      <section class="numbers-layout" aria-label="Finish bib recording">
        <div class="panel entry-panel">
          <p class="eyebrow">MANUAL ENTRY</p>
          <form onsubmit={submitManualBib}>
            <label for="manual-bib">Bib number</label>
            <div class="manual-entry">
              <input
                id="manual-bib"
                type="text"
                inputmode="numeric"
                pattern="[0-9]*"
                bind:value={manualBib}
                placeholder="Enter bib number"
              />
              <button
                class="record-button"
                type="button"
                onclick={recordManualBib}
              >
                Record bib
              </button>
            </div>
          </form>

          <div class="search-section">
            <label for="competitor-search">Search the full start list</label>
            <input
              id="competitor-search"
              type="search"
              bind:value={search}
              placeholder="Search bib or name"
              autocomplete="off"
            />
            {#if search.trim()}
              {#if matchingCompetitors.length === 0}
                <p class="empty-state">No matching competitors.</p>
              {:else}
                <ul class="competitor-results">
                  {#each matchingCompetitors as competitor (competitor.id)}
                    <li>
                      <button
                        type="button"
                        class="competitor-result"
                        onclick={() => void recordBib(competitor.bib)}
                        disabled={competitor.bib < 1}
                      >
                        <span class="competitor-bib">{competitor.bib || "—"}</span>
                        <span class="competitor-name">{competitor.name}</span>
                        {#if enteredBibs.has(competitor.bib)}
                          <span class="result-note">Entered</span>
                        {:else if competitor.status !== 0}
                          <span class="result-note">Status {competitor.status}</span>
                        {/if}
                      </button>
                    </li>
                  {/each}
                </ul>
              {/if}
            {/if}
          </div>
        </div>

        <div class="panel suggestion-panel">
          <div class="panel-heading">
            <div>
              <p class="eyebrow">QUICK SELECT</p>
              <h2>Likely finishers</h2>
            </div>
          </div>
          <p class="suggestion-note">
            Started by scheduled time; already entered and DNS/DNF/DSQ competitors are hidden.
          </p>
          {#if suggestedCompetitors.length === 0}
            <p class="empty-state">No eligible suggestions right now. Search the start list or enter a bib.</p>
          {:else}
            <div class="suggestion-grid">
              {#each suggestedCompetitors as competitor (competitor.id)}
                <button
                  type="button"
                  class="suggestion-button"
                  onclick={() => void recordBib(competitor.bib)}
                >
                  <span>{competitor.bib}</span>
                  <small>{competitor.name}</small>
                </button>
              {/each}
            </div>
          {/if}
        </div>
      </section>

      {#if pendingSaves > 0}
        <p class="saving-note" aria-live="polite">
          Saving {pendingSaves} {pendingSaves === 1 ? "entry" : "entries"}…
        </p>
      {/if}
      {#if notice}
        <p class="success-message" role="status">{notice}</p>
      {/if}

      <section class="panel queue-panel">
        <div class="queue-summary" aria-live="polite">
          <div class="queue-count">
            <span class="queue-count-number">{Math.abs(queueDifference)}</span>
            <span>{queueMessage(queueDifference)}</span>
          </div>
          <div class="queue-totals">
            <div>
              <span class="queue-total-number">{bibEntries.length}</span>
              <span>bib numbers</span>
            </div>
            <div>
              <span class="queue-total-number">{timeEntries.length}</span>
              <span>times</span>
            </div>
          </div>
        </div>
        <section class="recent-section" aria-labelledby="recent-heading">
          <div class="panel-heading">
            <h2 id="recent-heading">Latest bib numbers</h2>
            <span class="entry-count">{bibEntries.length} total</span>
          </div>
          {#if recentEntries.length === 0}
            <p class="empty-state">No bib numbers recorded yet.</p>
          {:else}
            <ol class="bib-list">
              {#each recentEntries as entry (entry.id)}
                <li>
                  <span class="sequence-number">{entry.sequence_no}</span>
                  <span class="recorded-bib">{entry.bib ?? "Unknown bib"}</span>
                  <span class="entry-status">
                    {entry.status === 0 ? "Confirmed" : `Status ${entry.status}`}
                  </span>
                </li>
              {/each}
            </ol>
          {/if}
        </section>
      </section>
    {:else}
      <section class="message-card error-message" role="alert">
        {pageError || "This competition could not be loaded."}
      </section>
    {/if}
  </main>
</div>

<style>
  .numbers-page {
    max-width: 1000px;
  }

  .numbers-heading {
    margin-bottom: 22px;
  }

  .numbers-layout {
    display: grid;
    grid-template-columns: minmax(0, 1fr) minmax(280px, 0.9fr);
    align-items: start;
    gap: 18px;
  }

  .entry-panel,
  .suggestion-panel {
    min-height: 340px;
  }

  .entry-panel form,
  .search-section {
    display: grid;
    gap: 8px;
  }

  .manual-entry {
    display: grid;
    grid-template-columns: minmax(0, 1fr) auto;
    gap: 9px;
  }

  input {
    width: 100%;
    min-height: 48px;
    padding: 10px 12px;
    border: 1px solid var(--color-border);
    border-radius: 8px;
    color: var(--color-ink);
    background: white;
  }

  label {
    font-size: 13px;
    font-weight: 650;
  }

  .record-button {
    min-height: 48px;
    padding: 0 18px;
    border: 0;
    border-radius: 8px;
    background: var(--color-primary);
    color: white;
    font-weight: 700;
    touch-action: manipulation;
  }

  .record-button:hover {
    background: var(--color-primary-hover);
  }

  .search-section {
    margin-top: 24px;
  }

  .competitor-results {
    display: grid;
    gap: 5px;
    max-height: 210px;
    overflow: auto;
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .competitor-result {
    display: flex;
    align-items: center;
    gap: 10px;
    width: 100%;
    min-height: 44px;
    padding: 7px 10px;
    border: 0;
    border-radius: 7px;
    background: #f7f9f6;
    color: var(--color-ink);
    text-align: left;
  }

  .competitor-result:hover:not(:disabled) {
    background: var(--color-primary-soft);
  }

  .competitor-result:disabled {
    cursor: not-allowed;
    opacity: 0.55;
  }

  .competitor-bib {
    min-width: 45px;
    font-weight: 750;
    font-variant-numeric: tabular-nums;
  }

  .competitor-name {
    overflow: hidden;
    flex: 1;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .result-note {
    color: var(--color-muted);
    font-size: 11px;
  }

  .suggestion-panel .panel-heading {
    margin-bottom: 8px;
  }

  .suggestion-note {
    margin: 0 0 16px;
    color: var(--color-muted);
    font-size: 12px;
    line-height: 1.45;
  }

  .suggestion-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 9px;
  }

  .suggestion-button {
    display: grid;
    justify-items: center;
    gap: 2px;
    min-height: 76px;
    padding: 8px;
    border: 1px solid var(--color-border);
    border-radius: 9px;
    background: white;
    color: var(--color-ink);
    touch-action: manipulation;
  }

  .suggestion-button:hover {
    border-color: var(--color-primary);
    background: var(--color-primary-soft);
  }

  .suggestion-button > span {
    font-size: 22px;
    font-weight: 750;
    font-variant-numeric: tabular-nums;
  }

  .suggestion-button small {
    overflow: hidden;
    max-width: 100%;
    color: var(--color-muted);
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .queue-panel {
    margin-top: 18px;
  }

  .queue-summary {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    padding-bottom: 18px;
    border-bottom: 1px solid var(--color-border);
  }

  .queue-count {
    display: grid;
    gap: 4px;
    color: var(--color-primary);
    font-size: 14px;
    font-weight: 700;
  }

  .queue-count-number {
    font-size: 40px;
    line-height: 1;
  }

  .queue-totals {
    display: flex;
    gap: 28px;
  }

  .queue-totals > div {
    display: grid;
    gap: 2px;
    color: var(--color-muted);
    font-size: 12px;
  }

  .queue-total-number {
    color: var(--color-ink);
    font-size: 18px;
    font-weight: 700;
    font-variant-numeric: tabular-nums;
  }

  .recent-section {
    padding-top: 18px;
  }

  .recent-section .panel-heading {
    margin-bottom: 10px;
  }

  .entry-count,
  .entry-status {
    color: var(--color-muted);
    font-size: 12px;
  }

  .bib-list {
    display: grid;
    gap: 6px;
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .bib-list li {
    display: flex;
    align-items: center;
    gap: 12px;
    min-height: 44px;
    padding: 8px 12px;
    border-radius: 7px;
    background: #f7f9f6;
  }

  .sequence-number {
    min-width: 28px;
    color: var(--color-muted);
    text-align: right;
  }

  .recorded-bib {
    flex: 1;
    font-size: 17px;
    font-weight: 700;
    font-variant-numeric: tabular-nums;
  }

  .saving-note,
  .success-message {
    margin: 12px 0 0;
    color: var(--color-muted);
    font-size: 13px;
  }

  .success-message {
    color: var(--color-primary);
    font-weight: 650;
  }

  .error-message {
    margin-bottom: 16px;
    padding: 14px 18px;
    color: var(--color-danger);
  }

  .empty-state {
    margin: 8px 0 0;
    color: var(--color-muted);
    font-size: 13px;
    line-height: 1.45;
  }

  @media (max-width: 720px) {
    .numbers-page {
      padding: 28px 16px 40px;
    }

    .numbers-layout {
      grid-template-columns: 1fr;
      gap: 12px;
    }

    .entry-panel,
    .suggestion-panel {
      min-height: auto;
      padding: 20px;
    }

    .suggestion-panel {
      order: -1;
    }

    .queue-panel {
      margin-top: 12px;
      padding: 20px;
    }

    .queue-summary {
      align-items: flex-start;
      flex-direction: column;
    }
  }
</style>
