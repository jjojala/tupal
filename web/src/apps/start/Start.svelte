<script lang="ts">
  import { onMount } from "svelte";
  import {
    getClasses,
    getCompetition,
    getCompetitors,
    getStartGroups,
    subscribeToCompetition,
    updateCompetitor
  } from "../../shared/api";
  import {
    durationToSeconds,
    wallClockToTimestamp
  } from "../../shared/competitors";
  import type {
    Competition,
    CompetitionClass,
    Competitor,
    StartGroup
  } from "../../shared/types";

  type StartRow = {
    competitor: Competitor;
    classTitle: string;
    timestamp: number;
  };

  export let competitionId: string;

  let competition: Competition | undefined;
  let classes: CompetitionClass[] = [];
  let competitors: Competitor[] = [];
  let startGroups: StartGroup[] = [];
  let loading = true;
  let connected = false;
  let pageError = "";
  let notice = "";
  let savingIds: string[] = [];
  let now = Date.now();

  $: classById = new Map(classes.map((item) => [item.id, item]));
  $: startGroupById = new Map(startGroups.map((item) => [item.id, item]));
  $: rows = competitors.flatMap((competitor): StartRow[] => {
    if (competitor.bib < 1 || competitor.status !== 0 || hasFinish(competitor)) {
      return [];
    }

    const classInfo = classById.get(competitor.comp_class_id);
    const startGroup = classInfo
      ? startGroupById.get(classInfo.start_group_id)
      : undefined;
    if (!classInfo || !startGroup) return [];

    try {
      const firstStart = wallClockToTimestamp(startGroup.first_start_time);
      const offset = durationToSeconds(competitor.start_time_offset);
      if (!Number.isFinite(firstStart)) return [];
      return [{
        competitor,
        classTitle: classInfo.title,
        timestamp: firstStart + offset * 1000
      }];
    } catch {
      return [];
    }
  }).sort((a, b) => a.timestamp - b.timestamp || a.competitor.bib - b.competitor.bib);
  $: startList = rows.filter((row) => row.timestamp > now);
  $: dnsSuggestions = rows.filter((row) => row.timestamp <= now);

  onMount(() => {
    void reload();
    const unsubscribe = subscribeToCompetition(
      competitionId,
      () => void reload(),
      (state) => (connected = state)
    );
    const timer = window.setInterval(() => (now = Date.now()), 1000);
    return () => {
      unsubscribe();
      window.clearInterval(timer);
    };
  });

  async function reload() {
    pageError = "";
    try {
      const [competitionData, classData, competitorData, startGroupData] =
        await Promise.all([
          getCompetition(competitionId),
          getClasses(competitionId),
          getCompetitors(competitionId),
          getStartGroups(competitionId)
        ]);
      competition = competitionData;
      classes = classData;
      competitors = competitorData;
      startGroups = startGroupData;
    } catch (error) {
      pageError = errorMessage(error);
    } finally {
      loading = false;
    }
  }

  function hasFinish(competitor: Competitor) {
    const finishTime = Date.parse(competitor.finish_time);
    return Number.isFinite(finishTime) && finishTime > 0;
  }

  async function confirmDns(competitor: Competitor) {
    pageError = "";
    notice = "";
    savingIds = [...savingIds, competitor.id];
    try {
      await updateCompetitor(competitionId, {
        id: competitor.id,
        comp_class_id: competitor.comp_class_id,
        bib: competitor.bib,
        name: competitor.name,
        start_time_offset: competitor.start_time_offset,
        finish_time: competitor.finish_time,
        status: 1
      });
      await reload();
      notice = `Bib ${competitor.bib} confirmed as DNS.`;
    } catch (error) {
      pageError = errorMessage(error);
    } finally {
      savingIds = savingIds.filter((id) => id !== competitor.id);
    }
  }

  function formatStartTime(timestamp: number) {
    return new Intl.DateTimeFormat(undefined, {
      hour: "2-digit",
      minute: "2-digit",
      second: "2-digit"
    }).format(timestamp);
  }

  function errorMessage(error: unknown) {
    return error instanceof Error ? error.message : "An unexpected error occurred.";
  }
</script>

<svelte:head>
  <title>{competition ? `Start · ${competition.title} · Tupal` : "Start · Tupal"}</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
</svelte:head>

<main class="start-shell">
  <header class="start-header">
    <div>
      <p class="eyebrow">TUPAL · START</p>
      <h1>{competition?.title ?? "Start list"}</h1>
      {#if competition}
        <p class="competition-date">{competition.date}</p>
      {/if}
    </div>
    <span class:connected class="connection-status">
      {connected ? "Connected" : "Reconnecting"}
    </span>
  </header>

  {#if pageError}
    <p class="error-message" role="alert">{pageError}</p>
  {/if}
  {#if notice}
    <p class="notice-message" role="status">{notice}</p>
  {/if}

  {#if loading}
    <p class="empty-message">Loading start list…</p>
  {:else if competition}
    <section class="start-section" aria-labelledby="start-list-heading">
      <div class="section-heading">
        <div>
          <h2 id="start-list-heading">Upcoming starters</h2>
          <p>Competitors are listed in scheduled start-time order.</p>
        </div>
        <span class="count">{startList.length}</span>
      </div>

      {#if startList.length}
        <ul class="start-list">
          {#each startList as row (row.competitor.id)}
            <li data-bib={row.competitor.bib}>
              <time>{formatStartTime(row.timestamp)}</time>
              <div class="competitor-details">
                <strong class="bib">#{row.competitor.bib}</strong>
                <span>{row.competitor.name}</span>
                <small>{row.classTitle}</small>
              </div>
            </li>
          {/each}
        </ul>
      {:else}
        <p class="empty-message">No upcoming starters.</p>
      {/if}
    </section>

    <section class="dns-section" aria-labelledby="dns-heading">
      <div class="section-heading">
        <div>
          <h2 id="dns-heading">Check for DNS</h2>
          <p>Overdue competitors are treated as started unless DNS is confirmed.</p>
        </div>
        <span class="count">{dnsSuggestions.length}</span>
      </div>

      {#if dnsSuggestions.length}
        <ul class="dns-list">
          {#each dnsSuggestions as row (row.competitor.id)}
            <li data-bib={row.competitor.bib}>
              <div class="competitor-details">
                <strong class="bib">#{row.competitor.bib}</strong>
                <span>{row.competitor.name}</span>
              </div>
              <button
                type="button"
                disabled={savingIds.includes(row.competitor.id)}
                on:click={() => confirmDns(row.competitor)}
              >
                Confirm DNS
              </button>
            </li>
          {/each}
        </ul>
      {:else}
        <p class="empty-message">No overdue starters to check.</p>
      {/if}
    </section>
  {:else}
    <p class="error-message" role="alert">Competition not found.</p>
  {/if}
</main>

<style>
  :global(body) {
    margin: 0;
    background: #f3f6f4;
    color: #17241d;
    font-family: Inter, ui-sans-serif, system-ui, sans-serif;
  }

  .start-shell {
    width: min(100%, 720px);
    min-height: 100vh;
    margin: 0 auto;
    padding: 20px 16px 40px;
    box-sizing: border-box;
  }

  .start-header,
  .section-heading,
  .start-list li,
  .dns-list li {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 14px;
  }

  .start-header {
    margin-bottom: 24px;
    align-items: flex-start;
  }

  .eyebrow {
    margin: 0 0 6px;
    color: #48755b;
    font-size: 0.72rem;
    font-weight: 800;
    letter-spacing: 0.12em;
  }

  h1,
  h2,
  p {
    margin-top: 0;
  }

  h1 {
    margin-bottom: 4px;
    font-size: clamp(1.6rem, 7vw, 2.2rem);
    line-height: 1.15;
  }

  .competition-date,
  .section-heading p {
    margin: 0;
    color: #5a6c61;
    font-size: 0.9rem;
  }

  .connection-status {
    flex-shrink: 0;
    margin-top: 5px;
    padding: 6px 9px;
    border-radius: 999px;
    background: #fff1d8;
    color: #795318;
    font-size: 0.75rem;
    font-weight: 700;
  }

  .connection-status.connected {
    background: #dff2e5;
    color: #21603a;
  }

  .start-section,
  .dns-section {
    margin-top: 18px;
    padding: 18px 14px;
    border: 1px solid #dce5de;
    border-radius: 16px;
    background: #fff;
    box-shadow: 0 5px 18px #223a2a0a;
  }

  .section-heading {
    margin-bottom: 14px;
  }

  .section-heading h2 {
    margin-bottom: 4px;
    font-size: 1.08rem;
  }

  .count {
    display: grid;
    min-width: 32px;
    height: 32px;
    place-items: center;
    border-radius: 50%;
    background: #e8f2eb;
    color: #27623e;
    font-weight: 800;
  }

  .start-list,
  .dns-list {
    display: grid;
    gap: 9px;
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .start-list li,
  .dns-list li {
    min-width: 0;
    padding: 12px 0;
    border-top: 1px solid #edf1ee;
  }

  time {
    flex: 0 0 76px;
    color: #405f4a;
    font-variant-numeric: tabular-nums;
    font-weight: 750;
  }

  .competitor-details {
    display: flex;
    flex: 1;
    min-width: 0;
    flex-direction: column;
    gap: 2px;
  }

  .bib {
    color: #183e27;
    font-size: 1.04rem;
  }

  .competitor-details span,
  .competitor-details small {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .competitor-details small {
    color: #68796e;
  }

  button {
    flex: 0 0 auto;
    min-height: 42px;
    padding: 0 13px;
    border: 0;
    border-radius: 10px;
    background: #23683c;
    color: #fff;
    font: inherit;
    font-size: 0.87rem;
    font-weight: 750;
  }

  button:disabled {
    opacity: 0.55;
  }

  .dns-section {
    border-color: #eadfc7;
  }

  .empty-message,
  .error-message,
  .notice-message {
    margin: 8px 0 0;
    color: #64736a;
  }

  .error-message,
  .notice-message {
    padding: 12px;
    border-radius: 10px;
  }

  .error-message {
    background: #ffebe8;
    color: #872b20;
  }

  .notice-message {
    background: #e1f3e7;
    color: #205a34;
  }

  @media (max-width: 440px) {
    .start-list li {
      flex-wrap: wrap;
    }

    .dns-list li {
      align-items: flex-start;
      flex-direction: column;
    }

    .dns-list li button {
      align-self: stretch;
      width: 100%;
    }
  }
</style>
