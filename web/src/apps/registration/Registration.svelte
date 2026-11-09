<script lang="ts">
  import { onMount } from "svelte";
  import {
    createCompetitor,
    deleteCompetitor,
    getClasses,
    getCompetition,
    getCompetitors,
    getStartGroups,
    subscribeToCompetition,
    updateCompetitor
  } from "../../shared/api";
  import {
    durationToSeconds,
    hasDuplicateBib,
    nextAvailableBib,
    secondsToDuration
  } from "../../shared/competitors";
  import type {
    Competition,
    CompetitionClass,
    Competitor,
    CompetitorInput,
    StartGroup
  } from "../../shared/types";

  export let competitionId: string;

  let competition: Competition | undefined;
  let classes: CompetitionClass[] = [];
  let startGroups: StartGroup[] = [];
  let competitors: Competitor[] = [];
  let selectedClassId = "";
  let name = "";
  let bib = "";
  let bibWasEdited = false;
  let startOffsetSeconds = "0";
  let editingId: string | undefined;
  let search = "";
  let loading = true;
  let saving = false;
  let pageError = "";
  let formError = "";
  let notice = "";
  let connected = false;

  $: classById = new Map(classes.map((item) => [item.id, item]));
  $: visibleCompetitors = competitors
    .filter((competitor) => {
      const query = search.trim().toLocaleLowerCase();
      return (
        !query ||
        competitor.name.toLocaleLowerCase().includes(query) ||
        String(competitor.bib).includes(query)
      );
    })
    .sort((left, right) => left.bib - right.bib || left.name.localeCompare(right.name));

  onMount(() => {
    void reload();
    return subscribeToCompetition(
      competitionId,
      () => void reload(),
      (state) => (connected = state)
    );
  });

  async function reload() {
    loading = true;
    pageError = "";
    try {
      const [competitionData, classData, startGroupData, competitorData] = await Promise.all([
        getCompetition(competitionId),
        getClasses(competitionId),
        getStartGroups(competitionId),
        getCompetitors(competitionId)
      ]);
      competition = competitionData;
      classes = classData;
      startGroups = startGroupData;
      competitors = competitorData;
      if (!selectedClassId && classData.length > 0) {
        selectedClassId = classData[0].id;
      }
      if (!editingId && !bibWasEdited) suggestBib();
    } catch (error) {
      pageError = errorMessage(error);
    } finally {
      loading = false;
    }
  }

  function errorMessage(error: unknown) {
    return error instanceof Error ? error.message : "An unexpected error occurred.";
  }

  function resetForm() {
    editingId = undefined;
    name = "";
    bib = "";
    bibWasEdited = false;
    startOffsetSeconds = "0";
    formError = "";
  }

  function suggestBib() {
    const suggestedBib = nextAvailableBib(competitors, classes, startGroups, selectedClassId);
    bib = suggestedBib === undefined ? "" : String(suggestedBib);
  }

  function selectClass(event: Event) {
    if (!(event.currentTarget instanceof HTMLSelectElement)) return;
    selectedClassId = event.currentTarget.value;
    if (!editingId) {
      bibWasEdited = false;
      suggestBib();
    }
  }

  function editCompetitor(competitor: Competitor) {
    editingId = competitor.id;
    selectedClassId = competitor.comp_class_id;
    name = competitor.name;
    bib = String(competitor.bib);
    bibWasEdited = true;
    startOffsetSeconds = String(durationToSeconds(competitor.start_time_offset));
    formError = "";
    notice = "";
    document.getElementById("competitor-name")?.focus();
  }

  async function saveCompetitor(event: SubmitEvent) {
    event.preventDefault();
    formError = "";
    notice = "";

    const parsedBib = Number(bib);
    const parsedOffset = Number(startOffsetSeconds);
    if (!selectedClassId || !name.trim()) {
      formError = "Enter a name and choose a class.";
      return;
    }
    if (!Number.isInteger(parsedBib) || parsedBib < 1 || parsedBib > 65535) {
      formError = "Bib number must be a whole number between 1 and 65535.";
      return;
    }
    if (!Number.isFinite(parsedOffset) || parsedOffset < 0) {
      formError = "Start offset must be zero or more seconds.";
      return;
    }
    if (hasDuplicateBib(competitors, classes, parsedBib, selectedClassId, editingId)) {
      formError = "That bib number is already assigned in this start group.";
      return;
    }

    const isEditing = Boolean(editingId);
    const previous = editingId
      ? competitors.find((competitor) => competitor.id === editingId)
      : undefined;
    const payload: CompetitorInput = {
      id: editingId ?? null,
      comp_class_id: selectedClassId,
      bib: parsedBib,
      name: name.trim(),
      start_time_offset: secondsToDuration(parsedOffset),
      finish_time: previous?.finish_time ?? "",
      status: previous?.status ?? 0
    };

    saving = true;
    try {
      if (isEditing) {
        await updateCompetitor(competitionId, payload);
        notice = `${payload.name} updated.`;
      } else {
        await createCompetitor(competitionId, payload);
        notice = `${payload.name} registered.`;
      }
      resetForm();
      await reload();
      if (!isEditing) {
        window.setTimeout(
          () => document.getElementById("competitor-name")?.focus(),
          0
        );
      }
    } catch (error) {
      formError = errorMessage(error);
    } finally {
      saving = false;
    }
  }

  async function removeCompetitor(competitor: Competitor) {
    const confirmed = window.confirm(
      `Remove ${competitor.name} (bib ${competitor.bib}) from this competition? This cannot be undone.`
    );
    if (!confirmed) return;

    notice = "";
    pageError = "";
    try {
      await deleteCompetitor(competitionId, competitor.id);
      if (editingId === competitor.id) resetForm();
      notice = `${competitor.name} removed.`;
      await reload();
    } catch (error) {
      pageError = errorMessage(error);
    }
  }
</script>

<svelte:head>
  <title>
    {competition ? `${competition.title} · Registration` : "Registration"} · Tupal
  </title>
</svelte:head>

<div class="app-shell">
  <header class="topbar">
    <a
      id="registration-home"
      class="brand"
      href={`/registration/${encodeURIComponent(competitionId)}`}
      aria-label="Tupal registration home"
    >
      <span class="brand-mark" aria-hidden="true">T</span>
      <span>Tupal</span>
    </a>
    <div class="task-label">
      <span class="task-dot" aria-hidden="true"></span>
      Registration
    </div>
    <div class:online={connected} class="connection-state" aria-live="polite">
      <span class="connection-dot" aria-hidden="true"></span>
      {connected ? "Live updates on" : "Reconnecting…"}
    </div>
  </header>

  <main class="page">
    {#if loading && !competition}
      <section class="loading-card" aria-live="polite">Loading competition…</section>
    {:else if pageError && !competition}
      <section class="message-card error-banner" role="alert">
        <strong>Could not open this competition</strong>
        <p>{pageError}</p>
        <button id="retry-load" class="button button-secondary" onclick={() => void reload()}>Try again</button>
      </section>
    {:else if competition}
      <div class="page-heading">
        <div>
          <p class="eyebrow">COMPETITOR CHECK-IN</p>
          <h1>{competition.title}</h1>
          <p class="subheading">
            {new Intl.DateTimeFormat(undefined, { dateStyle: "medium", timeZone: "UTC" }).format(
              new Date(competition.date)
            )} <span aria-hidden="true">·</span>
            <span>{competitors.length} registered</span>
          </p>
        </div>
      </div>

      {#if pageError}
        <div class="error-banner" role="alert">{pageError}</div>
      {/if}
      {#if notice}
        <div class="notice-banner" role="status">{notice}</div>
      {/if}

      <div class="registration-layout">
        <section class="panel form-panel" aria-labelledby="form-title">
          <div class="panel-heading">
            <div>
              <p class="eyebrow">{editingId ? "UPDATE ENTRY" : "NEW ENTRY"}</p>
              <h2 id="form-title">{editingId ? "Edit competitor" : "Register competitor"}</h2>
            </div>
            {#if editingId}
              <button id="cancel-edit" class="text-button" type="button" onclick={resetForm}>Cancel</button>
            {/if}
          </div>

          {#if classes.length === 0}
            <div class="empty-state">
              <strong>No classes are set up yet</strong>
              <p>Ask the competition manager to add classes before registration.</p>
            </div>
          {:else}
            <form id="competitor-form" onsubmit={saveCompetitor}>
              <label for="competitor-name">Competitor name</label>
              <input
                id="competitor-name"
                bind:value={name}
                autocomplete="name"
                placeholder="Family name, given name"
                required
                maxlength="120"
              />

              <label for="competitor-class">Class</label>
              <select id="competitor-class" value={selectedClassId} onchange={selectClass} required>
                <option value="" disabled>Choose a class</option>
                {#each classes as competitionClass (competitionClass.id)}
                  <option value={competitionClass.id}>{competitionClass.title}</option>
                {/each}
              </select>

              <div class="field-row">
                <div>
                  <label for="competitor-bib">Bib number</label>
                  <input
                    id="competitor-bib"
                    bind:value={bib}
                    oninput={() => (bibWasEdited = true)}
                    type="number"
                    inputmode="numeric"
                    min="1"
                    max="65535"
                    step="1"
                    placeholder="e.g. 101"
                    required
                  />
                </div>
                <div>
                  <label for="competitor-offset">Start offset (seconds)</label>
                  <input
                    id="competitor-offset"
                    bind:value={startOffsetSeconds}
                    type="number"
                    inputmode="decimal"
                    min="0"
                    step="0.001"
                    required
                  />
                </div>
              </div>

              {#if formError}
                <p class="field-error" role="alert">{formError}</p>
              {/if}

              <button
                id="competitor-submit"
                class="button button-primary submit-button"
                type="submit"
                disabled={saving}
              >
                {saving ? "Saving…" : editingId ? "Save changes" : "Add competitor"}
              </button>
            </form>
          {/if}
        </section>

        <section class="panel roster-panel" aria-labelledby="roster-title">
          <div class="panel-heading roster-heading">
            <div>
              <p class="eyebrow">COMPETITION ROSTER</p>
              <h2 id="roster-title">Registered competitors</h2>
            </div>
            <span class="count-pill">{competitors.length}</span>
          </div>

          <label class="search-label" for="competitor-search">Find by name or bib</label>
          <input
            id="competitor-search"
            class="search-input"
            type="search"
            bind:value={search}
            placeholder="Search roster"
          />

          {#if competitors.length === 0}
            <div class="empty-state roster-empty">
              <strong>No competitors yet</strong>
              <p>Add the first entry using the form.</p>
            </div>
          {:else if visibleCompetitors.length === 0}
            <div class="empty-state roster-empty">No matches found.</div>
          {:else}
            <ul class="roster-list">
              {#each visibleCompetitors as competitor (competitor.id)}
                <li class="roster-row">
                  <span class="bib-badge">{competitor.bib}</span>
                  <div class="competitor-details">
                    <strong>{competitor.name}</strong>
                    <span>{classById.get(competitor.comp_class_id)?.title ?? "Unknown class"}</span>
                  </div>
                  <div class="row-actions">
                    <button
                      id={`edit-competitor-${encodeURIComponent(competitor.id)}`}
                      class="icon-button"
                      type="button"
                      aria-label={`Edit ${competitor.name}`}
                      onclick={() => editCompetitor(competitor)}
                    >Edit</button>
                    <button
                      id={`remove-competitor-${encodeURIComponent(competitor.id)}`}
                      class="icon-button danger-action"
                      type="button"
                      aria-label={`Remove ${competitor.name}`}
                      onclick={() => void removeCompetitor(competitor)}
                    >Remove</button>
                  </div>
                </li>
              {/each}
            </ul>
          {/if}
        </section>
      </div>
    {:else}
      <section class="message-card error-banner" role="alert">
        <strong>Competition not found</strong>
        <p>Check that this registration link is correct.</p>
      </section>
    {/if}
  </main>
</div>
