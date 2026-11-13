<script lang="ts">
  import { onMount } from "svelte";
  import {
    createFinishBibEntry,
    createFinishTimeEntry,
    getCompetition,
    getCompetitors,
    getFinishBibEntries,
    getFinishTimeEntries,
    getStartGroups,
    getClasses,
    subscribeToCompetition,
    updateCompetitor
  } from "../../shared/api";
  import { durationToSeconds } from "../../shared/competitors";
  import type {
    Competition,
    CompetitionClass,
    Competitor,
    CompetitorInput,
    FinishBibEntry,
    FinishBibEntryInput,
    FinishTimeEntry,
    FinishTimeEntryInput,
    StartGroup
  } from "../../shared/types";

  export let competitionId: string;

  const recentEntryLimit = 8;

  let competition: Competition | undefined;
  let competitors: Competitor[] = [];
  let classes: CompetitionClass[] = [];
  let startGroups: StartGroup[] = [];
  let timeEntries: FinishTimeEntry[] = [];
  let bibEntries: FinishBibEntry[] = [];
  let timeStatus = 0;
  let bibStatus = 0;
  let search = "";
  let manualBib = "";
  let dsqSearch = "";
  let loading = true;
  let connected = false;
  let now = Date.now();
  let clock = new Date();
  let pendingTimeSaves = 0;
  let pendingBibSaves = 0;
  let pendingSequenceNos: number[] = [];
  let savingCompetitorIds: string[] = [];
  let pageError = "";
  let notice = "";

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
  $: dsqMatches = dsqSearch.trim()
    ? [...competitors]
        .filter((competitor) =>
          String(competitor.bib).includes(dsqSearch.trim()) ||
          competitor.name.toLocaleLowerCase().includes(dsqSearch.trim().toLocaleLowerCase())
        )
        .sort((left, right) => left.bib - right.bib)
        .slice(0, 20)
    : [];
  $: dsqCompetitors = competitors
    .filter((competitor) => competitor.status === 3)
    .sort((left, right) => left.bib - right.bib);
  $: latestTimes = [...timeEntries]
    .sort((left, right) => right.finish_time.localeCompare(left.finish_time))
    .slice(0, recentEntryLimit);
  $: latestBibs = [...bibEntries]
    .sort((left, right) =>
      right.sequence_no - left.sequence_no || right.id.localeCompare(left.id)
    )
    .slice(0, recentEntryLimit);
  $: queueDifference = bibEntries.length - timeEntries.length;

  onMount(() => {
    void reload();
    const timer = window.setInterval(() => {
      clock = new Date();
      now = clock.getTime();
    }, 100);
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
        competitorData,
        classData,
        startGroupData,
        timeData,
        bibData
      ] = await Promise.all([
        getCompetition(competitionId),
        getCompetitors(competitionId),
        getClasses(competitionId),
        getStartGroups(competitionId),
        getFinishTimeEntries(competitionId),
        getFinishBibEntries(competitionId)
      ]);
      competition = competitionData;
      competitors = competitorData;
      classes = classData;
      startGroups = startGroupData;
      timeEntries = timeData;
      bibEntries = bibData;
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
    const firstStart = Date.parse(startGroup.first_start_time);
    if (!Number.isFinite(firstStart)) return Number.POSITIVE_INFINITY;
    try {
      return firstStart + durationToSeconds(competitor.start_time_offset) * 1000;
    } catch {
      return Number.POSITIVE_INFINITY;
    }
  }

  async function recordTime() {
    pageError = "";
    notice = "";
    pendingTimeSaves += 1;
    const capturedAt = new Date();
    const entry: FinishTimeEntryInput = {
      id: null,
      finish_time: formatLocalTimestamp(capturedAt),
      status: timeStatus
    };
    try {
      const created = await createFinishTimeEntry(competitionId, entry);
      timeEntries = [
        ...timeEntries.filter((item) => item.id !== created.id),
        created
      ];
      notice = `${timeStatusLabel(created.status)} time recorded at ${displayTime(created.finish_time)}.`;
    } catch (error) {
      pageError = errorMessage(error);
    } finally {
      pendingTimeSaves -= 1;
    }
  }

  async function recordBib(bib: number | null, status = bibStatus) {
    if (bib !== null && (!Number.isInteger(bib) || bib < 1 || bib > 65535)) {
      pageError = "Bib number must be a whole number between 1 and 65535.";
      return;
    }

    pageError = "";
    notice = "";
    const sequenceNo = nextSequenceNo();
    pendingSequenceNos = [...pendingSequenceNos, sequenceNo];
    pendingBibSaves += 1;
    const entry: FinishBibEntryInput = {
      id: null,
      sequence_no: sequenceNo,
      bib,
      status,
      confirmed_time_entry_id: null
    };
    try {
      const created = await createFinishBibEntry(competitionId, entry);
      bibEntries = [
        ...bibEntries.filter((item) => item.id !== created.id),
        created
      ];
      if (bib !== null && manualBib === String(bib)) manualBib = "";
      notice = bib === null
        ? `${bibStatusLabel(created.status)} missing-bib entry recorded as number ${created.sequence_no}.`
        : `Bib ${bib} recorded as number ${created.sequence_no} (${bibStatusLabel(created.status).toLowerCase()}).`;
    } catch (error) {
      pageError = errorMessage(error);
    } finally {
      pendingSequenceNos = pendingSequenceNos.filter((item) => item !== sequenceNo);
      pendingBibSaves -= 1;
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
    if (!manualBib.trim()) {
      pageError = "Enter a bib number.";
      return;
    }
    void recordBib(Number(manualBib));
  }

  async function toggleDsq(competitor: Competitor) {
    pageError = "";
    notice = "";
    savingCompetitorIds = [...savingCompetitorIds, competitor.id];
    const nextStatus = competitor.status === 3 ? 0 : 3;
    const update: CompetitorInput = {
      id: competitor.id,
      comp_class_id: competitor.comp_class_id,
      bib: competitor.bib,
      name: competitor.name,
      start_time_offset: competitor.start_time_offset,
      finish_time: competitor.finish_time,
      status: nextStatus
    };
    try {
      const updated = await updateCompetitor(competitionId, update);
      competitors = competitors.map((item) =>
        item.id === updated.id ? updated : item
      );
      notice = nextStatus === 3
        ? `Bib ${competitor.bib} disqualified. The recorded finish time was preserved.`
        : `DSQ removed for bib ${competitor.bib}.`;
    } catch (error) {
      pageError = errorMessage(error);
    } finally {
      savingCompetitorIds = savingCompetitorIds.filter(
        (id) => id !== competitor.id
      );
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

  function timeStatusLabel(status: number) {
    return ["Confirmed", "Uncertain", "Incorrect", "Missing"][status] ?? "Unknown";
  }

  function bibStatusLabel(status: number) {
    return timeStatusLabel(status);
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

  function errorMessage(error: unknown) {
    return error instanceof Error ? error.message : "An unexpected error occurred.";
  }
</script>

<svelte:head>
  <title>{competition ? `Finishline · ${competition.title} · Tupal` : "Finishline · Tupal"}</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
</svelte:head>

<div class="app-shell">
  <header class="topbar">
    <a class="brand" href="/">
      <span class="brand-mark" aria-hidden="true">T</span>
      <span>Tupal</span>
    </a>
    <span class="task-label"><span class="task-dot"></span>Finishline</span>
    <span class:online={connected} class="connection-state">
      <span class="connection-dot"></span>
      {connected ? "Live" : "Offline"}
    </span>
  </header>

  <main class="page finishline-page">
    {#if loading}
      <section class="loading-card" aria-live="polite">Loading competition…</section>
    {:else if competition}
      <div class="page-heading finishline-heading">
        <div>
          <p class="eyebrow">COMBINED FINISH LINE</p>
          <h1>{competition.title}</h1>
          <p class="subheading">Record finish times and bib numbers in one place.</p>
        </div>
        <div class="queue-status" aria-live="polite">
          <strong>{Math.abs(queueDifference)}</strong>
          <span>{queueMessage(queueDifference)}</span>
          <small>{timeEntries.length} times · {bibEntries.length} bib entries</small>
        </div>
      </div>

      {#if pageError}
        <div class="message-card error-message" role="alert">{pageError}</div>
      {/if}
      {#if notice}
        <div class="message-card success-message" role="status">{notice}</div>
      {/if}

      <section class="capture-layout" aria-label="Combined finish recording">
        <section class="panel time-panel" aria-labelledby="time-heading">
          <div class="panel-heading">
            <div>
              <p class="eyebrow">TIMEKEEPING</p>
              <h2 id="time-heading">Record a finish time</h2>
            </div>
          </div>
          <p class="live-clock" aria-label="Current local time">
            {clock.toLocaleTimeString(undefined, {
              hour: "2-digit",
              minute: "2-digit",
              second: "2-digit",
              hour12: false
            })}
            <span>.{String(clock.getMilliseconds()).padStart(3, "0")}</span>
          </p>
          <label for="time-status">Time status</label>
          <select id="time-status" bind:value={timeStatus}>
            <option value={0}>Confirmed</option>
            <option value={1}>Uncertain</option>
            <option value={2}>Incorrect</option>
            <option value={3}>Missing</option>
          </select>
          <p class="field-hint">
            A missing time is recorded with the current clock value as its queue position.
          </p>
          <button
            class="time-record-button"
            type="button"
            aria-busy={pendingTimeSaves > 0}
            onclick={recordTime}
          >
            Record finish time
          </button>
          {#if pendingTimeSaves > 0}
            <p class="saving-note" aria-live="polite">Saving finish time…</p>
          {/if}
        </section>

        <section class="panel bib-panel" aria-labelledby="bib-heading">
          <div class="panel-heading">
            <div>
              <p class="eyebrow">NUMBER READING</p>
              <h2 id="bib-heading">Record a bib number</h2>
            </div>
          </div>

          <div class="bib-status-field">
            <label for="bib-status">Number status</label>
            <select id="bib-status" bind:value={bibStatus}>
              <option value={0}>Confirmed</option>
              <option value={1}>Uncertain</option>
              <option value={2}>Incorrect</option>
              <option value={3}>Missing</option>
            </select>
          </div>

          <form class="manual-entry" onsubmit={submitManualBib}>
            <label for="manual-bib">Bib number</label>
            <div class="manual-entry-controls">
              <input
                id="manual-bib"
                type="text"
                inputmode="numeric"
                pattern="[0-9]*"
                bind:value={manualBib}
                placeholder="Enter bib number"
              />
              <button class="secondary-button" type="submit">
                Record bib
              </button>
            </div>
          </form>

          <div class="suggestions">
            <div class="subsection-heading">
              <h3>Likely finishers</h3>
              <span>{suggestedCompetitors.length}</span>
            </div>
            {#if suggestedCompetitors.length}
              <div class="suggestion-grid">
                {#each suggestedCompetitors as competitor (competitor.id)}
                  <button
                    class="suggestion-button"
                    type="button"
                    onclick={() => void recordBib(competitor.bib)}
                  >
                    <strong>{competitor.bib}</strong>
                    <small>{competitor.name}</small>
                  </button>
                {/each}
              </div>
            {:else}
              <p class="empty-state">No likely finishers. Search the list or enter a bib.</p>
            {/if}
          </div>

          <div class="search-section">
            <label for="competitor-search">Search competitor</label>
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
                        disabled={competitor.bib < 1}
                        onclick={() => void recordBib(competitor.bib)}
                      >
                        <strong>{competitor.bib || "—"}</strong>
                        <span>{competitor.name}</span>
                        {#if enteredBibs.has(competitor.bib)}
                          <small>Entered</small>
                        {:else if competitor.status !== 0}
                          <small>{competitor.status === 3 ? "DSQ" : `Status ${competitor.status}`}</small>
                        {/if}
                      </button>
                    </li>
                  {/each}
                </ul>
              {/if}
            {/if}
          </div>

          <button
            class="missing-bib-button"
            type="button"
            onclick={() => void recordBib(null, 3)}
          >
            Record missing bib
          </button>
          {#if pendingBibSaves > 0}
            <p class="saving-note" aria-live="polite">Saving bib entry…</p>
          {/if}
        </section>
      </section>

      <section class="queue-layout" aria-label="Recent finish queue entries">
        <section class="panel recent-panel" aria-labelledby="recent-times-heading">
          <div class="panel-heading">
            <h2 id="recent-times-heading">Latest times</h2>
            <span class="entry-count">{timeEntries.length} total</span>
          </div>
          {#if latestTimes.length}
            <ol class="entry-list">
              {#each latestTimes as entry (entry.id)}
                <li>
                  <span class="queue-order">{timeEntries.length - timeEntries.indexOf(entry)}</span>
                  <time>{displayTime(entry.finish_time)}</time>
                  <span class:status-warning={entry.status !== 0} class="entry-status">
                    {timeStatusLabel(entry.status)}
                  </span>
                </li>
              {/each}
            </ol>
          {:else}
            <p class="empty-state">No times recorded yet.</p>
          {/if}
        </section>

        <section class="panel recent-panel" aria-labelledby="recent-bibs-heading">
          <div class="panel-heading">
            <h2 id="recent-bibs-heading">Latest bibs</h2>
            <span class="entry-count">{bibEntries.length} total</span>
          </div>
          {#if latestBibs.length}
            <ol class="entry-list">
              {#each latestBibs as entry (entry.id)}
                <li>
                  <span class="queue-order">{entry.sequence_no}</span>
                  <strong class="recorded-bib">{entry.bib ?? "Unknown bib"}</strong>
                  <span class:status-warning={entry.status !== 0} class="entry-status">
                    {bibStatusLabel(entry.status)}
                  </span>
                </li>
              {/each}
            </ol>
          {:else}
            <p class="empty-state">No bibs recorded yet.</p>
          {/if}
        </section>
      </section>

      <details class="panel dsq-panel">
        <summary>
          <span>
            <strong>Disqualifications</strong>
            <small>{dsqCompetitors.length} currently recorded</small>
          </span>
          <span class="disclosure-indicator" aria-hidden="true">+</span>
        </summary>
        <p class="dsq-note">
          DSQ status is applied immediately and preserves the competitor's finish time.
          DSQ reasons are not recorded in this version.
        </p>
        <label for="dsq-search">Find a competitor</label>
        <input
          id="dsq-search"
          type="search"
          bind:value={dsqSearch}
          placeholder="Search bib or name"
          autocomplete="off"
        />
        {#if dsqSearch.trim()}
          {#if dsqMatches.length}
            <ul class="dsq-results">
              {#each dsqMatches as competitor (competitor.id)}
                <li>
                  <div>
                    <strong>#{competitor.bib}</strong>
                    <span>{competitor.name}</span>
                    {#if competitor.finish_time}
                      <small>Finish {displayTime(competitor.finish_time)}</small>
                    {/if}
                  </div>
                  <button
                    class:remove-dsq={competitor.status === 3}
                    type="button"
                    disabled={savingCompetitorIds.includes(competitor.id)}
                    onclick={() => void toggleDsq(competitor)}
                  >
                    {competitor.status === 3 ? "Remove DSQ" : "Disqualify"}
                  </button>
                </li>
              {/each}
            </ul>
          {:else}
            <p class="empty-state">No matching competitors.</p>
          {/if}
        {/if}

        {#if dsqCompetitors.length}
          <h3>DSQ competitors</h3>
          <ul class="dsq-results current-dsq">
            {#each dsqCompetitors as competitor (competitor.id)}
              <li>
                <div>
                  <strong>#{competitor.bib}</strong>
                  <span>{competitor.name}</span>
                </div>
                <button
                  class="remove-dsq"
                  type="button"
                  disabled={savingCompetitorIds.includes(competitor.id)}
                  onclick={() => void toggleDsq(competitor)}
                >
                  Remove DSQ
                </button>
              </li>
            {/each}
          </ul>
        {/if}
      </details>
    {:else}
      <section class="message-card error-message" role="alert">
        {pageError || "This competition could not be loaded."}
      </section>
    {/if}
  </main>
</div>

<style>
  .finishline-page {
    max-width: 1180px;
  }

  .finishline-heading {
    display: flex;
    align-items: flex-end;
    justify-content: space-between;
    gap: 20px;
    margin-bottom: 20px;
  }

  .queue-status {
    display: grid;
    grid-template-columns: auto 1fr;
    align-items: center;
    column-gap: 12px;
    padding: 12px 16px;
    border: 1px solid var(--color-border);
    border-radius: 12px;
    background: white;
  }

  .queue-status strong {
    grid-row: span 2;
    color: var(--color-primary);
    font-size: 32px;
    line-height: 1;
  }

  .queue-status span {
    font-size: 13px;
    font-weight: 700;
  }

  .queue-status small {
    color: var(--color-muted);
  }

  .capture-layout {
    display: grid;
    grid-template-columns: minmax(270px, 0.8fr) minmax(400px, 1.2fr);
    align-items: start;
    gap: 16px;
  }

  .time-panel,
  .bib-panel {
    min-width: 0;
    padding: 20px;
  }

  .time-panel .panel-heading,
  .bib-panel .panel-heading {
    margin-bottom: 16px;
  }

  .live-clock {
    margin: 0 0 16px;
    color: var(--color-ink);
    font-variant-numeric: tabular-nums;
    font-size: clamp(32px, 5vw, 46px);
    font-weight: 750;
    letter-spacing: -0.04em;
    text-align: center;
  }

  .live-clock span {
    color: var(--color-muted);
    font-size: 0.52em;
    letter-spacing: -0.02em;
  }

  .time-panel h2,
  .bib-panel h2,
  .recent-panel h2 {
    margin: 0;
    font-size: 17px;
  }

  label {
    display: block;
    margin-bottom: 7px;
    font-size: 13px;
    font-weight: 650;
  }

  input,
  select {
    width: 100%;
    min-height: 46px;
    padding: 9px 11px;
    border: 1px solid var(--color-border);
    border-radius: 8px;
    background: white;
    color: var(--color-ink);
    font: inherit;
  }

  .field-hint,
  .dsq-note {
    margin: 8px 0 16px;
    color: var(--color-muted);
    font-size: 12px;
    line-height: 1.45;
  }

  .time-record-button {
    width: 100%;
    min-height: 88px;
    border: 0;
    border-radius: 11px;
    background: var(--color-primary);
    color: white;
    font-size: 21px;
    font-weight: 750;
    touch-action: manipulation;
  }

  .time-record-button:hover:not(:disabled) {
    background: var(--color-primary-hover);
  }

  .time-record-button:disabled,
  button:disabled {
    cursor: wait;
    opacity: 0.6;
  }

  .bib-status-field {
    display: grid;
    grid-template-columns: minmax(90px, 0.5fr) minmax(150px, 1fr);
    align-items: center;
    gap: 12px;
    margin-bottom: 14px;
  }

  .bib-status-field label {
    margin: 0;
  }

  .manual-entry {
    display: grid;
    gap: 7px;
  }

  .manual-entry-controls {
    display: grid;
    grid-template-columns: minmax(0, 1fr) auto;
    gap: 8px;
  }

  .secondary-button,
  .missing-bib-button,
  .dsq-results button {
    min-height: 46px;
    padding: 0 14px;
    border: 1px solid var(--color-border);
    border-radius: 8px;
    background: #f7f9f6;
    color: var(--color-ink);
    font: inherit;
    font-weight: 700;
    touch-action: manipulation;
  }

  .secondary-button:hover:not(:disabled),
  .missing-bib-button:hover:not(:disabled) {
    border-color: var(--color-primary);
    background: var(--color-primary-soft);
  }

  .suggestions,
  .search-section {
    margin-top: 20px;
  }

  .subsection-heading,
  .panel-heading {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
  }

  .subsection-heading {
    margin-bottom: 9px;
  }

  .subsection-heading h3,
  .dsq-panel h3 {
    margin: 0;
    font-size: 14px;
  }

  .subsection-heading > span,
  .entry-count {
    color: var(--color-muted);
    font-size: 12px;
  }

  .suggestion-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(105px, 1fr));
    gap: 7px;
  }

  .suggestion-button {
    display: grid;
    justify-items: center;
    gap: 2px;
    min-height: 62px;
    padding: 7px;
    border: 1px solid var(--color-border);
    border-radius: 8px;
    background: white;
    color: var(--color-ink);
    touch-action: manipulation;
  }

  .suggestion-button:hover:not(:disabled) {
    border-color: var(--color-primary);
    background: var(--color-primary-soft);
  }

  .suggestion-button strong {
    font-size: 19px;
    font-variant-numeric: tabular-nums;
  }

  .suggestion-button small {
    overflow: hidden;
    max-width: 100%;
    color: var(--color-muted);
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .competitor-results,
  .entry-list,
  .dsq-results {
    display: grid;
    gap: 5px;
    margin: 8px 0 0;
    padding: 0;
    list-style: none;
  }

  .competitor-results {
    max-height: 180px;
    overflow: auto;
  }

  .competitor-results button {
    display: flex;
    align-items: center;
    gap: 10px;
    width: 100%;
    min-height: 42px;
    padding: 7px 10px;
    border: 0;
    border-radius: 7px;
    background: #f7f9f6;
    color: var(--color-ink);
    text-align: left;
  }

  .competitor-results button span {
    overflow: hidden;
    flex: 1;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .competitor-results small {
    color: var(--color-muted);
    font-size: 11px;
  }

  .missing-bib-button {
    width: 100%;
    margin-top: 14px;
    border-style: dashed;
    color: var(--color-muted);
  }

  .queue-layout {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 16px;
    margin-top: 16px;
  }

  .recent-panel {
    min-width: 0;
    padding: 18px;
  }

  .entry-list li {
    display: flex;
    align-items: center;
    gap: 11px;
    min-height: 42px;
    padding: 6px 9px;
    border-radius: 7px;
    background: #f7f9f6;
  }

  .queue-order {
    min-width: 24px;
    color: var(--color-muted);
    font-size: 12px;
    text-align: right;
  }

  .entry-list time,
  .recorded-bib {
    flex: 1;
    font-size: 16px;
    font-weight: 700;
    font-variant-numeric: tabular-nums;
  }

  .entry-status {
    color: var(--color-muted);
    font-size: 11px;
    font-weight: 650;
  }

  .status-warning {
    color: #855b16;
  }

  .dsq-panel {
    margin-top: 16px;
    padding: 0 18px 18px;
  }

  .dsq-panel summary {
    display: flex;
    align-items: center;
    justify-content: space-between;
    min-height: 62px;
    cursor: pointer;
    list-style: none;
  }

  .dsq-panel summary::-webkit-details-marker {
    display: none;
  }

  .dsq-panel summary > span:first-child {
    display: grid;
    gap: 3px;
  }

  .dsq-panel summary small {
    color: var(--color-muted);
    font-size: 12px;
  }

  .disclosure-indicator {
    color: var(--color-primary);
    font-size: 25px;
    line-height: 1;
  }

  details[open] .disclosure-indicator {
    transform: rotate(45deg);
  }

  .dsq-results li {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    min-height: 48px;
    padding: 7px 10px;
    border-radius: 7px;
    background: #f7f9f6;
  }

  .dsq-results li > div {
    display: grid;
    grid-template-columns: auto minmax(0, 1fr);
    column-gap: 9px;
    align-items: baseline;
    min-width: 0;
  }

  .dsq-results li > div span {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .dsq-results li > div small {
    grid-column: 2;
    color: var(--color-muted);
    font-size: 11px;
  }

  .dsq-results button {
    min-height: 40px;
    background: #fff0ed;
    color: #8b3328;
  }

  .dsq-results button.remove-dsq {
    background: #e8f4eb;
    color: var(--color-primary);
  }

  .current-dsq {
    margin-top: 12px;
  }

  .saving-note,
  .empty-state {
    margin: 8px 0 0;
    color: var(--color-muted);
    font-size: 12px;
    line-height: 1.45;
  }

  .success-message {
    color: var(--color-primary);
  }

  .error-message {
    color: var(--color-danger);
  }

  @media (max-width: 760px) {
    .finishline-heading {
      align-items: stretch;
      flex-direction: column;
      gap: 12px;
    }

    .queue-status {
      align-self: stretch;
    }

    .capture-layout,
    .queue-layout {
      grid-template-columns: 1fr;
      gap: 12px;
    }

    .time-panel,
    .bib-panel,
    .recent-panel {
      padding: 16px;
    }

    .time-record-button {
      min-height: 76px;
    }

    .queue-layout {
      margin-top: 12px;
    }

    .dsq-panel {
      margin-top: 12px;
    }
  }

  @media (max-width: 420px) {
    .manual-entry-controls {
      grid-template-columns: 1fr;
    }

    .secondary-button {
      width: 100%;
    }

    .bib-status-field {
      grid-template-columns: 1fr;
      gap: 6px;
    }

    .dsq-results li {
      align-items: stretch;
      flex-direction: column;
    }

    .dsq-results button {
      width: 100%;
    }
  }
</style>
