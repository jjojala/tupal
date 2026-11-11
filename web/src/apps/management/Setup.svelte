<script lang="ts">
  import {
    createCompetitionClass,
    createStartGroup,
    deleteCompetitionClass,
    deleteStartGroup,
    updateCompetitionClass,
    updateStartGroup
  } from "../../shared/api";
  import { durationToSeconds } from "../../shared/competitors";
  import type {
    CompetitionClass,
    CompetitionClassInput,
    Competitor,
    StartGroup,
    StartGroupInput
  } from "../../shared/types";
  import ManagementPageHeading from "./ManagementPageHeading.svelte";

  export let competitionId: string;
  export let competitionTitle: string;
  export let competitionDate: string;
  export let classes: CompetitionClass[] = [];
  export let competitors: Competitor[] = [];
  export let startGroups: StartGroup[] = [];
  export let onChange: () => Promise<void>;

  let startGroupFormOpen = false;
  let classFormOpen = false;
  let editingStartGroupId: string | undefined;
  let editingClassId: string | undefined;
  let startGroupTitle = "";
  let firstStartTime = "";
  let firstBib = "0";
  let startIntervalSeconds = "0";
  let classTitle = "";
  let classStartGroupId = "";
  let saving = false;
  let error = "";
  let notice = "";

  $: classCountByStartGroup = new Map(
    startGroups.map((group) => [
      group.id,
      classes.filter((item) => item.start_group_id === group.id).length
    ])
  );
  $: competitorCountByClass = new Map(
    classes.map((item) => [
      item.id,
      competitors.filter((competitor) => competitor.comp_class_id === item.id).length
    ])
  );

  function localDateTimeInput(value: string) {
    return value.slice(0, 16);
  }

  function newLocalDateTimeInput() {
    const now = new Date();
    const pad = (value: number) => String(value).padStart(2, "0");
    return `${now.getFullYear()}-${pad(now.getMonth() + 1)}-${pad(now.getDate())}` +
      `T${pad(now.getHours())}:${pad(now.getMinutes())}`;
  }

  function formatDate(value: string) {
    const match = value.match(/^(\d{4})-(\d{2})-(\d{2})/);
    if (!match) return value;
    return new Intl.DateTimeFormat(undefined, {
      year: "numeric",
      month: "short",
      day: "numeric"
    }).format(new Date(Number(match[1]), Number(match[2]) - 1, Number(match[3])));
  }

  function formatStartTime(value: string) {
    const match = value.match(/T(\d{2}:\d{2})/);
    return match?.[1] ?? value;
  }

  function formatInterval(value: string) {
    try {
      const seconds = durationToSeconds(value);
      if (seconds === 0) return "Mass start";
      if (seconds % 60 === 0) return `${seconds / 60} min`;
      return `${seconds} sec`;
    } catch {
      return value;
    }
  }

  function resetStartGroupForm() {
    startGroupFormOpen = false;
    editingStartGroupId = undefined;
    startGroupTitle = "";
    firstStartTime = "";
    firstBib = "0";
    startIntervalSeconds = "0";
  }

  function openNewStartGroupForm() {
    error = "";
    notice = "";
    editingStartGroupId = undefined;
    startGroupTitle = "";
    firstStartTime = newLocalDateTimeInput();
    firstBib = "0";
    startIntervalSeconds = "0";
    startGroupFormOpen = true;
  }

  function editStartGroup(group: StartGroup) {
    error = "";
    notice = "";
    editingStartGroupId = group.id;
    startGroupTitle = group.title;
    firstStartTime = localDateTimeInput(group.first_start_time);
    firstBib = String(group.first_bib);
    try {
      startIntervalSeconds = String(durationToSeconds(group.start_interval));
    } catch {
      startIntervalSeconds = "0";
    }
    startGroupFormOpen = true;
  }

  async function saveStartGroup(event: SubmitEvent) {
    event.preventDefault();
    error = "";
    notice = "";

    const title = startGroupTitle.trim();
    const parsedFirstBib = Number(firstBib);
    const parsedInterval = Number(startIntervalSeconds);
    if (!title) {
      error = "Enter a start-group name.";
      return;
    }
    if (!firstStartTime || !/^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}$/.test(firstStartTime)) {
      error = "Enter a valid first start time.";
      return;
    }
    if (
      !Number.isInteger(parsedFirstBib) ||
      parsedFirstBib < 0 ||
      parsedFirstBib > 65535
    ) {
      error = "First bib must be a whole number between 0 and 65535.";
      return;
    }
    if (
      !Number.isFinite(parsedInterval) ||
      parsedInterval < 0 ||
      !Number.isSafeInteger(Math.round(parsedInterval * 1000))
    ) {
      error = "Start interval must be zero or a positive number of seconds.";
      return;
    }

    const totalMilliseconds = Math.round(parsedInterval * 1000);
    const intervalSeconds = Math.floor(totalMilliseconds / 1000);
    const intervalMilliseconds = totalMilliseconds % 1000;
    const payload: StartGroupInput = {
      id: editingStartGroupId ?? null,
      title,
      first_start_time: `${firstStartTime}:00`,
      first_bib: parsedFirstBib,
      start_interval:
        `PT${intervalSeconds}.${String(intervalMilliseconds).padStart(3, "0")}S`
    };
    saving = true;
    try {
      const isEditing = Boolean(editingStartGroupId);
      await (isEditing
        ? updateStartGroup(competitionId, payload)
        : createStartGroup(competitionId, payload));
      resetStartGroupForm();
      await onChange();
      notice = isEditing ? "Start group updated." : "Start group created.";
    } catch (saveError) {
      error = errorMessage(saveError);
    } finally {
      saving = false;
    }
  }

  async function removeStartGroup(group: StartGroup) {
    const assignedClasses = classCountByStartGroup.get(group.id) ?? 0;
    if (assignedClasses > 0) return;
    if (!window.confirm(`Delete start group “${group.title}”? This cannot be undone.`)) {
      return;
    }
    error = "";
    notice = "";
    saving = true;
    try {
      await deleteStartGroup(competitionId, group.id);
      await onChange();
      notice = `Start group “${group.title}” deleted.`;
    } catch (removeError) {
      error = errorMessage(removeError);
    } finally {
      saving = false;
    }
  }

  function resetClassForm() {
    classFormOpen = false;
    editingClassId = undefined;
    classTitle = "";
    classStartGroupId = "";
  }

  function openNewClassForm() {
    error = "";
    notice = "";
    editingClassId = undefined;
    classTitle = "";
    classStartGroupId = startGroups[0]?.id ?? "";
    classFormOpen = true;
  }

  function editClass(item: CompetitionClass) {
    error = "";
    notice = "";
    editingClassId = item.id;
    classTitle = item.title;
    classStartGroupId = item.start_group_id;
    classFormOpen = true;
  }

  async function saveClass(event: SubmitEvent) {
    event.preventDefault();
    error = "";
    notice = "";
    const title = classTitle.trim();
    if (!title) {
      error = "Enter a class name.";
      return;
    }
    if (!startGroups.some((group) => group.id === classStartGroupId)) {
      error = "Choose a start group for this class.";
      return;
    }

    const payload: CompetitionClassInput = {
      id: editingClassId ?? null,
      title,
      start_group_id: classStartGroupId
    };
    saving = true;
    try {
      const isEditing = Boolean(editingClassId);
      await (isEditing
        ? updateCompetitionClass(competitionId, payload)
        : createCompetitionClass(competitionId, payload));
      resetClassForm();
      await onChange();
      notice = isEditing ? "Class updated." : "Class created.";
    } catch (saveError) {
      error = errorMessage(saveError);
    } finally {
      saving = false;
    }
  }

  async function removeClass(item: CompetitionClass) {
    const assignedCompetitors = competitorCountByClass.get(item.id) ?? 0;
    if (assignedCompetitors > 0) return;
    if (!window.confirm(`Delete class “${item.title}”? This cannot be undone.`)) {
      return;
    }
    error = "";
    notice = "";
    saving = true;
    try {
      await deleteCompetitionClass(competitionId, item.id);
      await onChange();
      notice = `Class “${item.title}” deleted.`;
    } catch (removeError) {
      error = errorMessage(removeError);
    } finally {
      saving = false;
    }
  }

  function errorMessage(value: unknown) {
    return value instanceof Error ? value.message : "An unexpected error occurred.";
  }
</script>

<div class="setup-content">
  <ManagementPageHeading
    eyebrow="CONFIGURATION"
    title="Competition setup"
    description="Manage the start schedule and the classes that competitors enter."
  />

  {#if error}
    <div class="setup-alert error" role="alert">{error}</div>
  {/if}
  {#if notice}
    <div class="setup-alert success" role="status">{notice}</div>
  {/if}

  <section class="setup-panel competition-panel">
    <div class="section-heading">
      <div>
        <p class="section-eyebrow">EVENT</p>
        <h2>Competition details</h2>
      </div>
      <span class="readonly-badge">Identity</span>
    </div>
    <div class="competition-details">
      <div>
        <span class="detail-label">Competition name</span>
        <strong>{competitionTitle}</strong>
      </div>
      <div>
        <span class="detail-label">Date</span>
        <strong>{formatDate(competitionDate)}</strong>
      </div>
      <div>
        <span class="detail-label">ID</span>
        <code>{competitionId}</code>
      </div>
    </div>
    <p class="section-footnote">
      Competition identity editing is not part of this setup step.
    </p>
  </section>

  <div class="structure-grid">
    <section class="setup-panel structure-panel">
      <div class="section-heading">
        <div>
          <p class="section-eyebrow">SCHEDULE</p>
          <h2>Start groups</h2>
          <p class="section-description">
            Define a shared start time, first bib, and interval for each group.
          </p>
        </div>
        <span class="count-badge">{startGroups.length}</span>
      </div>

      {#if startGroupFormOpen}
        <form class="editor-form" onsubmit={saveStartGroup}>
          <div class="form-title-row">
            <h3>{editingStartGroupId ? "Edit start group" : "New start group"}</h3>
            {#if editingStartGroupId}
              <span class="readonly-badge">ID: {editingStartGroupId}</span>
            {/if}
          </div>
          <div class="form-grid">
            <label class="wide-field">
              Start-group name
              <input
                type="text"
                maxlength="100"
                bind:value={startGroupTitle}
                placeholder="e.g. Main start"
                required
              />
            </label>
            <label>
              First start
              <input type="datetime-local" bind:value={firstStartTime} required />
            </label>
            <label>
              First bib <span class="label-hint">(0 = not assigned)</span>
              <input
                type="number"
                min="0"
                max="65535"
                step="1"
                bind:value={firstBib}
                required
              />
            </label>
            <label>
              Start interval
              <div class="unit-input">
                <input
                  type="number"
                  min="0"
                  step="0.001"
                  bind:value={startIntervalSeconds}
                  required
                />
                <span>seconds</span>
              </div>
              <span class="label-hint">Use 0 for a mass start.</span>
            </label>
          </div>
          <div class="form-actions">
            <button
              class="button button-secondary"
              type="button"
              onclick={resetStartGroupForm}
              disabled={saving}
            >Cancel</button>
            <button class="button button-primary" type="submit" disabled={saving}>
              {saving ? "Saving…" : editingStartGroupId ? "Save changes" : "Create start group"}
            </button>
          </div>
        </form>
      {:else}
        <button
          class="add-button"
          type="button"
          onclick={openNewStartGroupForm}
          disabled={saving}
        >+ Add start group</button>
      {/if}

      {#if startGroups.length}
        <div class="table-scroll">
          <table>
            <thead>
              <tr>
                <th scope="col">Start group</th>
                <th scope="col">First start</th>
                <th scope="col">Bib sequence</th>
                <th scope="col">Classes</th>
                <th scope="col"><span class="visually-hidden">Actions</span></th>
              </tr>
            </thead>
            <tbody>
              {#each startGroups as group (group.id)}
                <tr>
                  <td>
                    <strong>{group.title}</strong>
                    <span class="row-subtitle">{group.id}</span>
                  </td>
                  <td>{formatStartTime(group.first_start_time)}</td>
                  <td>
                    {#if group.first_bib > 0}
                      #{group.first_bib} · {formatInterval(group.start_interval)}
                    {:else}
                      Not assigned
                    {/if}
                  </td>
                  <td>{classCountByStartGroup.get(group.id) ?? 0}</td>
                  <td class="action-cell">
                    <button
                      class="row-action"
                      type="button"
                      onclick={() => editStartGroup(group)}
                      disabled={saving}
                    >Edit</button>
                    <button
                      class="row-action danger-action"
                      type="button"
                      onclick={() => void removeStartGroup(group)}
                      disabled={saving || (classCountByStartGroup.get(group.id) ?? 0) > 0}
                      title={(classCountByStartGroup.get(group.id) ?? 0) > 0
                        ? "Reassign or delete its classes first"
                        : "Delete start group"}
                    >Delete</button>
                  </td>
                </tr>
              {/each}
            </tbody>
          </table>
        </div>
      {:else}
        <div class="empty-structure">
          <strong>No start groups configured</strong>
          <span>Add a start group before creating classes.</span>
        </div>
      {/if}
    </section>

    <section class="setup-panel structure-panel">
      <div class="section-heading">
        <div>
          <p class="section-eyebrow">ENTRY CATEGORIES</p>
          <h2>Classes</h2>
          <p class="section-description">
            Assign each class to the start group its competitors will use.
          </p>
        </div>
        <span class="count-badge">{classes.length}</span>
      </div>

      {#if classFormOpen}
        <form class="editor-form" onsubmit={saveClass}>
          <div class="form-title-row">
            <h3>{editingClassId ? "Edit class" : "New class"}</h3>
            {#if editingClassId}
              <span class="readonly-badge">ID: {editingClassId}</span>
            {/if}
          </div>
          <div class="form-grid class-form-grid">
            <label>
              Class name
              <input
                type="text"
                maxlength="100"
                bind:value={classTitle}
                placeholder="e.g. Women 18–39"
                required
              />
            </label>
            <label>
              Start group
              <select bind:value={classStartGroupId} required>
                <option value="" disabled>Choose a start group</option>
                {#each startGroups as group (group.id)}
                  <option value={group.id}>{group.title}</option>
                {/each}
              </select>
            </label>
          </div>
          <div class="form-actions">
            <button
              class="button button-secondary"
              type="button"
              onclick={resetClassForm}
              disabled={saving}
            >Cancel</button>
            <button class="button button-primary" type="submit" disabled={saving}>
              {saving ? "Saving…" : editingClassId ? "Save changes" : "Create class"}
            </button>
          </div>
        </form>
      {:else}
        <button
          class="add-button"
          type="button"
          onclick={openNewClassForm}
          disabled={saving || startGroups.length === 0}
          title={startGroups.length === 0 ? "Create a start group first" : undefined}
        >+ Add class</button>
      {/if}

      {#if classes.length}
        <div class="table-scroll">
          <table>
            <thead>
              <tr>
                <th scope="col">Class</th>
                <th scope="col">Start group</th>
                <th scope="col">Competitors</th>
                <th scope="col"><span class="visually-hidden">Actions</span></th>
              </tr>
            </thead>
            <tbody>
              {#each classes as item (item.id)}
                <tr>
                  <td>
                    <strong>{item.title}</strong>
                    <span class="row-subtitle">{item.id}</span>
                  </td>
                  <td>
                    {startGroups.find((group) => group.id === item.start_group_id)?.title ??
                      item.start_group_id}
                  </td>
                  <td>{competitorCountByClass.get(item.id) ?? 0}</td>
                  <td class="action-cell">
                    <button
                      class="row-action"
                      type="button"
                      onclick={() => editClass(item)}
                      disabled={saving}
                    >Edit</button>
                    <button
                      class="row-action danger-action"
                      type="button"
                      onclick={() => void removeClass(item)}
                      disabled={saving || (competitorCountByClass.get(item.id) ?? 0) > 0}
                      title={(competitorCountByClass.get(item.id) ?? 0) > 0
                        ? "Reassign or delete its competitors first"
                        : "Delete class"}
                    >Delete</button>
                  </td>
                </tr>
              {/each}
            </tbody>
          </table>
        </div>
      {:else}
        <div class="empty-structure">
          <strong>No classes configured</strong>
          <span>
            {startGroups.length
              ? "Add classes to organize competitor entries."
              : "Create a start group before adding classes."}
          </span>
        </div>
      {/if}
    </section>
  </div>
</div>

<style>
  .setup-content {
    width: 100%;
  }

  .setup-alert {
    display: flex;
    align-items: center;
    min-height: 42px;
    margin: -10px 0 16px;
    padding: 10px 13px;
    border: 1px solid;
    border-radius: 8px;
    font-size: 12px;
  }

  .setup-alert.error {
    border-color: #ebd0cc;
    background: #fff8f6;
    color: #983c33;
  }

  .setup-alert.success {
    border-color: #cfe4d5;
    background: #f3faf5;
    color: #286b48;
  }

  .setup-panel {
    min-width: 0;
    padding: 21px;
    border: 1px solid #e0e7e1;
    border-radius: 11px;
    background: white;
    box-shadow: 0 2px 8px rgb(22 43 31 / 3%);
  }

  .competition-panel {
    margin-bottom: 15px;
  }

  .section-heading {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 18px;
    margin-bottom: 17px;
  }

  .section-eyebrow {
    margin: 0 0 6px;
    color: #64806d;
    font-size: 9px;
    font-weight: 800;
    letter-spacing: 0.13em;
  }

  h2,
  h3,
  p {
    margin-top: 0;
  }

  h2 {
    margin-bottom: 4px;
    color: #24352b;
    font-size: 15px;
    letter-spacing: -0.02em;
  }

  h3 {
    margin: 0;
    color: #35473c;
    font-size: 13px;
  }

  .section-description {
    max-width: 440px;
    margin: 5px 0 0;
    color: #77837a;
    font-size: 11px;
    line-height: 1.45;
  }

  .readonly-badge,
  .count-badge {
    flex: 0 0 auto;
    padding: 5px 8px;
    border-radius: 20px;
    background: #f0f3f0;
    color: #718076;
    font-size: 10px;
    font-weight: 700;
  }

  .count-badge {
    min-width: 27px;
    text-align: center;
    font-variant-numeric: tabular-nums;
  }

  .competition-details {
    display: grid;
    grid-template-columns: 1.3fr 1fr 1fr;
    gap: 18px;
    padding: 15px 0 13px;
    border-top: 1px solid #edf0ed;
  }

  .competition-details > div {
    display: grid;
    align-content: start;
    gap: 5px;
    min-width: 0;
  }

  .detail-label {
    color: #78847b;
    font-size: 10px;
  }

  .competition-details strong,
  .competition-details code {
    overflow: hidden;
    color: #34473b;
    font-size: 12px;
    font-weight: 650;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .section-footnote {
    margin: 0;
    color: #919b93;
    font-size: 10px;
  }

  .structure-grid {
    display: grid;
    grid-template-columns: minmax(0, 1fr) minmax(0, 1fr);
    align-items: start;
    gap: 15px;
  }

  .structure-panel {
    min-height: 320px;
  }

  .add-button {
    min-height: 35px;
    margin-bottom: 13px;
    padding: 0 11px;
    border: 1px solid #cfe0d3;
    border-radius: 7px;
    background: #f8fbf8;
    color: #286b48;
    font-size: 11px;
    font-weight: 700;
  }

  .add-button:hover:not(:disabled) {
    background: #edf5ef;
  }

  .add-button:disabled {
    cursor: not-allowed;
    opacity: 0.55;
  }

  .editor-form {
    margin: 0 0 15px;
    padding: 15px;
    border: 1px solid #dfe9e1;
    border-radius: 9px;
    background: #f9fbf9;
  }

  .form-title-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 12px;
    margin-bottom: 13px;
  }

  .form-grid {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 11px;
  }

  .form-grid label {
    display: grid;
    align-content: start;
    gap: 6px;
    color: #435349;
    font-size: 10px;
    font-weight: 700;
  }

  .form-grid .wide-field {
    grid-column: 1 / -1;
  }

  .form-grid input,
  .form-grid select {
    width: 100%;
    min-width: 0;
    min-height: 37px;
    padding: 7px 9px;
    border: 1px solid #d5ded6;
    border-radius: 6px;
    background: white;
    color: #26382d;
    font-size: 11px;
    font-weight: 450;
  }

  .label-hint {
    color: #89948c;
    font-size: 9px;
    font-weight: 450;
  }

  .unit-input {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .unit-input input {
    flex: 1;
  }

  .unit-input > span {
    color: #768178;
    font-size: 10px;
    font-weight: 450;
  }

  .form-actions {
    display: flex;
    justify-content: flex-end;
    gap: 8px;
    margin-top: 15px;
  }

  .button {
    min-height: 35px;
    padding: 0 11px;
    border: 1px solid transparent;
    border-radius: 6px;
    font-size: 10px;
    font-weight: 700;
  }

  .button:disabled {
    cursor: wait;
    opacity: 0.6;
  }

  .button-primary {
    background: #286b48;
    color: white;
  }

  .button-primary:hover:not(:disabled) {
    background: #1e5739;
  }

  .button-secondary {
    border-color: #dce4dc;
    background: white;
    color: #59675d;
  }

  .table-scroll {
    overflow-x: auto;
  }

  table {
    width: 100%;
    border-collapse: collapse;
    text-align: left;
  }

  th {
    padding: 9px 7px;
    border-top: 1px solid #edf0ed;
    color: #849087;
    font-size: 9px;
    font-weight: 700;
    white-space: nowrap;
  }

  td {
    padding: 10px 7px;
    border-top: 1px solid #edf0ed;
    color: #59675d;
    font-size: 10px;
    white-space: nowrap;
  }

  td strong {
    display: block;
    color: #34473b;
    font-size: 11px;
  }

  .row-subtitle {
    display: block;
    margin-top: 3px;
    color: #919b93;
    font-size: 9px;
  }

  .action-cell {
    text-align: right;
  }

  .row-action {
    margin-left: 6px;
    padding: 5px 6px;
    border: 0;
    border-radius: 5px;
    background: transparent;
    color: #286b48;
    font-size: 9px;
    font-weight: 700;
  }

  .row-action:hover:not(:disabled) {
    background: #edf5ef;
  }

  .row-action:disabled {
    cursor: not-allowed;
    opacity: 0.38;
  }

  .danger-action {
    color: #a3473e;
  }

  .danger-action:hover:not(:disabled) {
    background: #fff1ef;
  }

  .empty-structure {
    display: grid;
    justify-items: center;
    gap: 6px;
    padding: 35px 10px;
    border-top: 1px solid #edf0ed;
    color: #7d8980;
    font-size: 11px;
    text-align: center;
  }

  .empty-structure strong {
    color: #516056;
    font-size: 12px;
  }

  .visually-hidden {
    position: absolute;
    width: 1px;
    height: 1px;
    overflow: hidden;
    clip: rect(0, 0, 0, 0);
    white-space: nowrap;
    clip-path: inset(50%);
  }

  @media (max-width: 1050px) {
    .structure-grid {
      grid-template-columns: 1fr;
    }
  }

  @media (max-width: 680px) {
    .competition-details {
      grid-template-columns: 1fr;
      gap: 12px;
    }

    .setup-panel {
      padding: 17px;
    }

    .form-grid {
      grid-template-columns: 1fr;
    }

    .form-grid .wide-field {
      grid-column: auto;
    }

    .form-title-row {
      align-items: flex-start;
      flex-direction: column;
    }
  }
</style>
