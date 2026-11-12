<script lang="ts">
  import { onMount } from "svelte";
  import ManagementPageHeading from "./ManagementPageHeading.svelte";
  import Setup from "./Setup.svelte";
  import {
    getClasses,
    getCompetition,
    getCompetitors,
    getFinishBibEntries,
    getFinishTimeEntries,
    getStartGroups,
    subscribeToCompetition
  } from "../../shared/api";
  import { durationToSeconds } from "../../shared/competitors";
  import type {
    Competition,
    CompetitionClass,
    Competitor,
    FinishBibEntry,
    FinishTimeEntry,
    StartGroup
  } from "../../shared/types";

  type ManagementView = "overview" | "setup" | "competitors" | "finish" | "results";

  export let competitionId: string;
  export let view: ManagementView;

  const navigation: {
    id: ManagementView;
    label: string;
    description: string;
  }[] = [
    { id: "overview", label: "Overview", description: "Competition workspace" },
    { id: "setup", label: "Setup", description: "Competition structure" },
    { id: "competitors", label: "Competitors", description: "Entries and details" },
    { id: "finish", label: "Finish", description: "Times and bib records" },
    { id: "results", label: "Results", description: "Standings and review" }
  ];

  let competition: Competition | undefined;
  let classes: CompetitionClass[] = [];
  let competitors: Competitor[] = [];
  let startGroups: StartGroup[] = [];
  let bibEntries: FinishBibEntry[] = [];
  let timeEntries: FinishTimeEntry[] = [];
  let loading = true;
  let pageError = "";
  let connected = false;
  let now = Date.now();

  $: currentNavigation = navigation.find((item) => item.id === view)!;
  $: viewUrl = (target: ManagementView) =>
    target === "overview"
      ? `/management/${encodeURIComponent(competitionId)}`
      : `/management/${encodeURIComponent(competitionId)}/${target}`;
  $: formattedDate = competition ? formatCompetitionDate(competition.date) : "";
  $: competitorTotals = calculateCompetitorTotals(
    competitors,
    classes,
    startGroups,
    now
  );

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

  function formatCompetitionDate(value: string) {
    const date = new Date(value);
    return Number.isNaN(date.getTime())
      ? value
      : new Intl.DateTimeFormat(undefined, {
          year: "numeric",
          month: "long",
          day: "numeric"
        }).format(date);
  }

  function calculateCompetitorTotals(
    allCompetitors: Competitor[],
    allClasses: CompetitionClass[],
    allStartGroups: StartGroup[],
    currentTime: number
  ) {
    let notStarted = 0;
    let started = 0;
    let dns = 0;
    let finished = 0;

    for (const competitor of allCompetitors) {
      if (competitor.status === 1) {
        dns += 1;
        continue;
      }

      const finishTime = Date.parse(competitor.finish_time);
      const hasFinish = Number.isFinite(finishTime) && finishTime > 0;
      if (competitor.status === 3 || (hasFinish && competitor.status !== 2)) {
        finished += 1;
        continue;
      }

      if (competitor.status === 2) {
        started += 1;
        continue;
      }

      const classInfo = allClasses.find(
        (item) => item.id === competitor.comp_class_id
      );
      const startGroup = allStartGroups.find(
        (item) => item.id === classInfo?.start_group_id
      );
      let startTimestamp = Number.POSITIVE_INFINITY;
      if (startGroup) {
        try {
          const firstStart = Date.parse(startGroup.first_start_time);
          const offset = durationToSeconds(competitor.start_time_offset);
          if (Number.isFinite(firstStart)) {
            startTimestamp = firstStart + offset * 1000;
          }
        } catch {
          startTimestamp = Number.POSITIVE_INFINITY;
        }
      }

      if (startTimestamp <= currentTime) started += 1;
      else notStarted += 1;
    }

    return {
      notStarted,
      started,
      dns,
      finished,
      open: allCompetitors.length - dns - finished
    };
  }

  function errorMessage(error: unknown) {
    return error instanceof Error ? error.message : "An unexpected error occurred.";
  }
</script>

<svelte:head>
  <title>
    {competition
      ? `${currentNavigation.label} · ${competition.title} · Tupal`
      : "Management · Tupal"}
  </title>
</svelte:head>

<div class="management-shell">
  <aside class="management-sidebar" aria-label="Competition navigation">
    <a class="management-brand" href="/">
      <span class="brand-mark" aria-hidden="true">T</span>
      <span>Tupal</span>
    </a>
    <p class="sidebar-caption">MANAGEMENT</p>
    {#if competition}
      <div class="sidebar-competition">
        <span class="competition-avatar" aria-hidden="true">
          {competition.title.slice(0, 1).toLocaleUpperCase()}
        </span>
        <span class="sidebar-competition-name">{competition.title}</span>
      </div>
    {/if}
    <nav class="management-nav" aria-label="Competition navigation">
      {#each navigation as item (item.id)}
        <a
          href={viewUrl(item.id)}
          class:active={view === item.id}
          aria-current={view === item.id ? "page" : undefined}
        >
          <span class="nav-indicator" aria-hidden="true"></span>
          <span class="nav-label">{item.label}</span>
        </a>
      {/each}
    </nav>
    <a class="sidebar-back" href="/management/">
      <span aria-hidden="true">←</span>
      All competitions
    </a>
  </aside>

  <div class="management-main">
    <header class="management-topbar">
      {#if competition}
        <div class="topbar-competition">
          <span class="topbar-title">{competition.title}</span>
          <span class="topbar-divider">/</span>
          <span>{currentNavigation.label}</span>
        </div>
      {:else}
        <span>Competition Management</span>
      {/if}
      <span class:online={connected} class="connection-state">
        <span class="connection-dot"></span>
        {connected ? "Live updates on" : "Reconnecting"}
      </span>
    </header>

    <main class="management-content">
      {#if loading}
        <section class="management-state" aria-live="polite">
          <span class="state-spinner" aria-hidden="true"></span>
          Loading competition…
        </section>
      {:else if competition}
        {#if pageError}
          <section class="management-error" role="alert">
            <strong>Couldn’t load all competition data</strong>
            <span>{pageError}</span>
            <button type="button" class="inline-button" onclick={() => void reload()}>
              Try again
            </button>
          </section>
        {/if}

        {#if view === "overview"}
          <section class="overview-intro">
            <p class="management-eyebrow">COMPETITION WORKSPACE</p>
            <h1>{competition.title}</h1>
            <p class="overview-date">{formattedDate}</p>
            <p class="overview-copy">
              Choose a workspace to configure the event, manage competitor
              records, review finish entries, or work with results.
            </p>
          </section>

          <section class="workspace-grid" aria-label="Management workspaces">
            {#each navigation.filter((item) => item.id !== "overview") as item (item.id)}
              <a class="workspace-card" href={viewUrl(item.id)}>
                <span class="workspace-icon" aria-hidden="true">
                  {#if item.id === "setup"}⚙
                  {:else if item.id === "competitors"}♙
                  {:else if item.id === "finish"}◷
                  {:else}▤{/if}
                </span>
                <span class="workspace-card-content">
                  <strong>{item.label}</strong>
                  <span>{item.description}</span>
                </span>
                <span class="workspace-arrow" aria-hidden="true">→</span>
              </a>
            {/each}
          </section>

          <section class="overview-facts" aria-label="Competition at a glance">
            <div>
              <span class="fact-value">{classes.length}</span>
              <span class="fact-label">Classes</span>
            </div>
            <div>
              <span class="fact-value">{startGroups.length}</span>
              <span class="fact-label">Start groups</span>
            </div>
            <div>
              <span class="fact-value">{competitors.length}</span>
              <span class="fact-label">Competitors</span>
            </div>
          </section>
        {:else if view === "setup"}
          <Setup
            {competitionId}
            competitionTitle={competition.title}
            competitionDate={competition.date}
            {classes}
            {competitors}
            {startGroups}
            onChange={reload}
          />
        {:else if view === "competitors"}
          <ManagementPageHeading
            eyebrow="ENTRY MANAGEMENT"
            title="Competitors"
            description="Search, review, and maintain competitor details."
          />
          <section class="management-panel view-placeholder">
            <div class="placeholder-mark" aria-hidden="true">♙</div>
            <h2>Competitor workspace</h2>
            <p>
              The planned workspace is a searchable table for bib, name, class,
              start time, result, and status, with editing and confirmed deletion.
            </p>
            <div class="placeholder-summary">
              <span><strong>{competitors.length}</strong> competitors</span>
              <span><strong>{classes.length}</strong> classes</span>
            </div>
            <span class="planned-tag">Editing not available yet</span>
          </section>
        {:else if view === "finish"}
          <ManagementPageHeading
            eyebrow="FINISH RECORDS"
            title="Finish"
            description="Review time and bib observations, their order, and corrections."
          />
          <div class="finish-summary">
            <section class="management-panel">
              <span class="summary-label">Recorded times</span>
              <strong>{timeEntries.length}</strong>
            </section>
            <section class="management-panel">
              <span class="summary-label">Recorded bib entries</span>
              <strong>{bibEntries.length}</strong>
            </section>
            <section class="management-panel">
              <span class="summary-label">Queue difference</span>
              <strong>{Math.abs(timeEntries.length - bibEntries.length)}</strong>
              <span class="summary-detail">
                {timeEntries.length > bibEntries.length
                  ? "Expecting bib numbers"
                  : bibEntries.length > timeEntries.length
                    ? "Expecting times"
                    : "Queues are in sync"}
              </span>
            </section>
          </div>
          <section class="management-panel view-placeholder">
            <div class="placeholder-mark" aria-hidden="true">◷</div>
            <h2>Finish record review</h2>
            <p>
              Time and bib queues will be shown in order here, with status,
              unmatched entries, and tools to correct records while preserving
              the original observations.
            </p>
            <span class="planned-tag">Detailed review not available yet</span>
          </section>
        {:else}
          <ManagementPageHeading
            eyebrow="COMPETITION OUTCOMES"
            title="Results"
            description="Review standings, competitor status, and what still needs attention."
          />
          <section class="results-layout">
            <div class="results-main-column">
              <section class="management-panel result-section">
                <div class="section-title-row">
                  <div>
                    <h2>Class results</h2>
                    <p>Readiness and best possible position by class.</p>
                  </div>
                  <span class="planned-tag">Coming next</span>
                </div>
                {#if classes.length}
                  <ul class="compact-list">
                    {#each classes as classItem (classItem.id)}
                      <li>
                        <strong>{classItem.title}</strong>
                        <span>Result readiness will appear here</span>
                      </li>
                    {/each}
                  </ul>
                {:else}
                  <p class="empty-note">Set up classes to prepare results.</p>
                {/if}
              </section>
              <section class="management-panel result-section">
                <div class="section-title-row">
                  <div>
                    <h2>Review required</h2>
                    <p>Uncertain, missing, or conflicting data to resolve.</p>
                  </div>
                </div>
                <p class="empty-note">
                  Review flags will be collected here when result generation is implemented.
                </p>
              </section>
            </div>
            <aside class="results-side-column">
              <section class="management-panel">
                <h2>Competitor status</h2>
                <p class="panel-description">
                  Started is inferred from scheduled start time unless DNS is confirmed.
                </p>
                <div class="status-placeholder">
                  <span>Not started</span><strong data-testid="total-not-started">{competitorTotals.notStarted}</strong>
                  <span>Started</span><strong data-testid="total-started">{competitorTotals.started}</strong>
                  <span>DNS</span><strong data-testid="total-dns">{competitorTotals.dns}</strong>
                  <span>Finished / resulted</span><strong data-testid="total-finished">{competitorTotals.finished}</strong>
                  <span>Open</span><strong data-testid="total-open">{competitorTotals.open}</strong>
                </div>
              </section>
              <section class="management-panel publish-panel">
                <h2>Print or export</h2>
                <p class="panel-description">
                  Prepare class results for printing or export once reviewed.
                </p>
                <button class="disabled-action" type="button" disabled>
                  Results not ready
                </button>
              </section>
            </aside>
          </section>
        {/if}
      {:else}
        <section class="management-error" role="alert">
          <strong>Competition not found</strong>
          <span>{pageError || `No competition exists with ID “${competitionId}”.`}</span>
          <a class="inline-button" href="/management/">Back to competitions</a>
        </section>
      {/if}
    </main>
  </div>
</div>

<style>
  .management-shell {
    display: grid;
    grid-template-columns: 252px minmax(0, 1fr);
    min-height: 100vh;
    background: #f5f7f5;
  }

  .management-sidebar {
    position: sticky;
    top: 0;
    display: flex;
    flex-direction: column;
    height: 100vh;
    padding: 24px 16px 18px;
    background: #172b22;
    color: #e8f0eb;
  }

  .management-brand {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 0 10px 24px;
    color: white;
    font-size: 17px;
    font-weight: 750;
    text-decoration: none;
  }

  .brand-mark {
    display: grid;
    width: 30px;
    height: 30px;
    place-items: center;
    border-radius: 9px;
    background: #3e8b63;
    color: white;
    font-size: 16px;
    font-weight: 800;
  }

  .sidebar-caption {
    margin: 0 10px 12px;
    color: #9eb4a6;
    font-size: 10px;
    font-weight: 800;
    letter-spacing: 0.13em;
  }

  .sidebar-competition {
    display: flex;
    align-items: center;
    gap: 10px;
    min-height: 54px;
    margin: 0 4px 18px;
    padding: 8px 10px;
    border: 1px solid rgb(255 255 255 / 12%);
    border-radius: 9px;
    background: rgb(255 255 255 / 5%);
  }

  .competition-avatar {
    display: grid;
    flex: 0 0 32px;
    height: 32px;
    place-items: center;
    border-radius: 8px;
    background: #315a45;
    color: white;
    font-size: 14px;
    font-weight: 750;
  }

  .sidebar-competition-name {
    overflow: hidden;
    font-size: 13px;
    font-weight: 650;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .management-nav {
    display: grid;
    gap: 4px;
  }

  .management-nav a {
    display: flex;
    align-items: center;
    gap: 11px;
    min-height: 42px;
    padding: 0 12px;
    border-radius: 8px;
    color: #c3d2c8;
    font-size: 13px;
    font-weight: 600;
    text-decoration: none;
    transition: background 120ms ease, color 120ms ease;
  }

  .management-nav a:hover {
    background: rgb(255 255 255 / 7%);
    color: white;
  }

  .management-nav a.active {
    background: #2d4d3b;
    color: white;
  }

  .nav-indicator {
    width: 7px;
    height: 7px;
    border: 1px solid #8da597;
    border-radius: 3px;
  }

  .management-nav a.active .nav-indicator {
    border-color: #9bddb5;
    background: #9bddb5;
  }

  .sidebar-back {
    display: flex;
    align-items: center;
    gap: 9px;
    min-height: 42px;
    margin-top: auto;
    padding: 0 10px;
    border-top: 1px solid rgb(255 255 255 / 13%);
    color: #b7c8bd;
    font-size: 12px;
    text-decoration: none;
  }

  .sidebar-back:hover {
    color: white;
  }

  .management-main {
    min-width: 0;
  }

  .management-topbar {
    position: sticky;
    z-index: 2;
    top: 0;
    display: flex;
    align-items: center;
    min-height: 62px;
    gap: 18px;
    padding: 0 34px;
    border-bottom: 1px solid #e1e7e2;
    background: rgb(255 255 255 / 94%);
    backdrop-filter: blur(8px);
    color: #66746b;
    font-size: 13px;
  }

  .topbar-competition {
    display: flex;
    align-items: center;
    gap: 10px;
  }

  .topbar-title {
    color: #25382d;
    font-weight: 700;
  }

  .topbar-divider {
    color: #c0c9c2;
  }

  .connection-state {
    display: inline-flex;
    align-items: center;
    gap: 7px;
    margin-left: auto;
    color: #7a867e;
    font-size: 11px;
  }

  .connection-dot {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: #d29b4d;
  }

  .connection-state.online .connection-dot {
    background: #3c9c68;
  }

  .management-content {
    width: min(1320px, 100%);
    margin: 0 auto;
    padding: 40px 42px 64px;
  }

  .management-eyebrow {
    margin: 0 0 9px;
    color: #367550;
    font-size: 10px;
    font-weight: 800;
    letter-spacing: 0.13em;
  }

  h1,
  h2,
  p {
    margin-top: 0;
  }

  h1 {
    margin-bottom: 9px;
    color: #1d2e24;
    font-size: clamp(29px, 3vw, 38px);
    letter-spacing: -0.04em;
    line-height: 1.12;
  }

  h2 {
    margin-bottom: 5px;
    color: #24352b;
    font-size: 16px;
    letter-spacing: -0.02em;
  }

  .overview-intro {
    max-width: 680px;
    margin: 5px 0 32px;
  }

  .overview-date {
    margin-bottom: 17px;
    color: #607066;
    font-size: 15px;
    font-weight: 600;
  }

  .overview-copy {
    max-width: 600px;
    margin-bottom: 0;
    color: #69766d;
    font-size: 14px;
    line-height: 1.65;
  }

  .workspace-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(280px, 1fr));
    gap: 13px;
    max-width: 920px;
  }

  .workspace-card {
    display: flex;
    align-items: center;
    min-height: 92px;
    gap: 15px;
    padding: 17px 18px;
    border: 1px solid #e0e7e1;
    border-radius: 11px;
    background: white;
    box-shadow: 0 2px 7px rgb(22 43 31 / 3%);
    color: inherit;
    text-decoration: none;
    transition: border-color 120ms ease, box-shadow 120ms ease, transform 120ms ease;
  }

  .workspace-card:hover {
    transform: translateY(-1px);
    border-color: #aac7b4;
    box-shadow: 0 5px 14px rgb(22 43 31 / 7%);
  }

  .workspace-icon {
    display: grid;
    flex: 0 0 42px;
    height: 42px;
    place-items: center;
    border-radius: 10px;
    background: #edf5ef;
    color: #367550;
    font-size: 20px;
  }

  .workspace-card-content {
    display: grid;
    flex: 1;
    gap: 5px;
  }

  .workspace-card-content strong {
    color: #25372c;
    font-size: 14px;
  }

  .workspace-card-content > span {
    color: #77837a;
    font-size: 12px;
  }

  .workspace-arrow {
    color: #8c9a90;
    font-size: 18px;
  }

  .overview-facts {
    display: flex;
    gap: 42px;
    max-width: 920px;
    margin-top: 36px;
    padding: 19px 2px;
    border-top: 1px solid #e0e7e1;
  }

  .overview-facts > div {
    display: grid;
    gap: 3px;
  }

  .fact-value {
    color: #2c4034;
    font-size: 19px;
    font-weight: 750;
    font-variant-numeric: tabular-nums;
  }

  .fact-label {
    color: #758178;
    font-size: 11px;
  }

  .management-panel,
  .management-state,
  .management-error {
    border: 1px solid #e0e7e1;
    border-radius: 11px;
    background: white;
    box-shadow: 0 2px 8px rgb(22 43 31 / 3%);
  }

  .management-panel {
    padding: 22px;
  }

  .section-title-row {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 20px;
    margin-bottom: 18px;
  }

  .section-title-row p,
  .panel-description {
    margin: 0;
    color: #77837a;
    font-size: 12px;
    line-height: 1.5;
  }

  .planned-tag {
    flex: 0 0 auto;
    padding: 5px 8px;
    border-radius: 20px;
    background: #f0f3f0;
    color: #718076;
    font-size: 10px;
    font-weight: 700;
  }

  .compact-list {
    display: grid;
    gap: 0;
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .compact-list li {
    display: flex;
    justify-content: space-between;
    gap: 16px;
    padding: 11px 0;
    border-top: 1px solid #edf0ed;
    font-size: 12px;
  }

  .compact-list li strong {
    color: #35473c;
  }

  .compact-list li span {
    color: #77837a;
    text-align: right;
  }

  .empty-note {
    margin: 0;
    padding: 12px 0 2px;
    color: #7b877e;
    font-size: 12px;
  }

  .view-placeholder {
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    max-width: 780px;
    min-height: 260px;
    padding: 32px;
  }

  .placeholder-mark {
    display: grid;
    width: 44px;
    height: 44px;
    place-items: center;
    margin-bottom: 17px;
    border-radius: 11px;
    background: #edf5ef;
    color: #367550;
    font-size: 22px;
  }

  .view-placeholder p {
    max-width: 620px;
    margin: 7px 0 20px;
    color: #758178;
    font-size: 13px;
    line-height: 1.6;
  }

  .placeholder-summary {
    display: flex;
    gap: 22px;
    margin-bottom: 20px;
    color: #77837a;
    font-size: 12px;
  }

  .placeholder-summary strong {
    color: #35473c;
  }

  .finish-summary {
    display: grid;
    grid-template-columns: repeat(3, minmax(0, 1fr));
    gap: 13px;
    max-width: 860px;
    margin-bottom: 16px;
  }

  .finish-summary .management-panel {
    display: grid;
    gap: 7px;
  }

  .summary-label,
  .summary-detail {
    color: #78847b;
    font-size: 11px;
  }

  .finish-summary strong {
    color: #2c4034;
    font-size: 25px;
    font-variant-numeric: tabular-nums;
  }

  .results-layout {
    display: grid;
    grid-template-columns: minmax(0, 1.45fr) minmax(280px, 0.8fr);
    align-items: start;
    gap: 15px;
  }

  .results-main-column,
  .results-side-column {
    display: grid;
    gap: 14px;
  }

  .result-section .section-title-row {
    margin-bottom: 10px;
  }

  .status-placeholder {
    display: grid;
    grid-template-columns: 1fr auto;
    gap: 12px;
    margin: 16px 0;
    padding: 15px 0;
    border-top: 1px solid #edf0ed;
    border-bottom: 1px solid #edf0ed;
    color: #65736b;
    font-size: 12px;
  }

  .status-placeholder strong {
    color: #3a4b40;
    font-variant-numeric: tabular-nums;
  }

  .publish-panel h2 {
    margin-bottom: 8px;
  }

  .disabled-action {
    width: 100%;
    min-height: 40px;
    margin-top: 18px;
    border: 0;
    border-radius: 7px;
    background: #e9ede9;
    color: #89938b;
    font-size: 12px;
    font-weight: 700;
  }

  .management-state {
    display: flex;
    align-items: center;
    gap: 12px;
    max-width: 440px;
    padding: 20px;
    color: #65736b;
    font-size: 13px;
  }

  .state-spinner {
    width: 17px;
    height: 17px;
    border: 2px solid #dfe7e0;
    border-top-color: #367550;
    border-radius: 50%;
    animation: spin 850ms linear infinite;
  }

  @keyframes spin {
    to { transform: rotate(360deg); }
  }

  .management-error {
    display: grid;
    justify-items: start;
    gap: 9px;
    max-width: 660px;
    margin-bottom: 22px;
    padding: 18px 20px;
    border-color: #ead4d0;
    color: #8b3831;
    font-size: 13px;
  }

  .management-error strong {
    font-size: 14px;
  }

  .inline-button {
    padding: 0;
    border: 0;
    background: transparent;
    color: #286b48;
    font-size: 12px;
    font-weight: 700;
    text-decoration: none;
    cursor: pointer;
  }

  @media (max-width: 900px) {
    .management-shell {
      grid-template-columns: 210px minmax(0, 1fr);
    }

    .management-content {
      padding: 32px 26px 52px;
    }

    .management-topbar {
      padding: 0 26px;
    }

    .results-layout {
      grid-template-columns: 1fr;
    }
  }

  @media (max-width: 680px) {
    .management-shell {
      grid-template-columns: 1fr;
    }

    .management-sidebar {
      position: sticky;
      z-index: 3;
      top: 0;
      height: auto;
      padding: 10px 12px;
    }

    .management-brand,
    .sidebar-caption,
    .sidebar-competition,
    .sidebar-back {
      display: none;
    }

    .management-nav {
      display: flex;
      gap: 4px;
      overflow-x: auto;
    }

    .management-nav a {
      flex: 0 0 auto;
      min-height: 38px;
      padding: 0 11px;
      font-size: 11px;
    }

    .nav-indicator {
      display: none;
    }

    .management-topbar {
      position: static;
      min-height: 48px;
      padding: 0 16px;
      font-size: 11px;
    }

    .management-content {
      padding: 27px 16px 40px;
    }

    .workspace-grid,
    .finish-summary {
      grid-template-columns: 1fr;
    }

    .overview-facts {
      gap: 23px;
    }

    .view-placeholder {
      min-height: 230px;
      padding: 24px;
    }
  }
</style>
