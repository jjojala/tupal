<script lang="ts">
  import { onMount } from "svelte";
  import { createCompetition, getCompetitions } from "../../shared/api";
  import type { Competition } from "../../shared/types";

  let competitions: Competition[] = [];
  let search = "";
  let title = "";
  let date = localDateValue(new Date());
  let formOpen = false;
  let saving = false;
  let loading = true;
  let pageError = "";
  let formError = "";
  let notice = "";

  $: filteredCompetitions = [...competitions]
    .filter((competition) => {
      const query = search.trim().toLocaleLowerCase();
      return !query ||
        competition.title.toLocaleLowerCase().includes(query) ||
        competition.id.toLocaleLowerCase().includes(query);
    })
    .sort((left, right) =>
      right.date.localeCompare(left.date) ||
      left.title.localeCompare(right.title)
    );

  onMount(() => {
    void reload();
  });

  async function reload() {
    pageError = "";
    loading = true;
    try {
      competitions = await getCompetitions();
    } catch (error) {
      pageError = errorMessage(error);
    } finally {
      loading = false;
    }
  }

  async function submitCompetition(event: SubmitEvent) {
    event.preventDefault();
    formError = "";
    notice = "";

    const trimmedTitle = title.trim();
    if (!trimmedTitle) {
      formError = "Enter a competition name.";
      return;
    }
    if (!date) {
      formError = "Choose a competition date.";
      return;
    }

    saving = true;
    try {
      const created = await createCompetition(trimmedTitle, date);
      competitions = [...competitions, created];
      title = "";
      date = localDateValue(new Date());
      formOpen = false;
      search = "";
      notice = `${created.title} created.`;
    } catch (error) {
      formError = errorMessage(error);
    } finally {
      saving = false;
    }
  }

  function localDateValue(value: Date) {
    const pad = (part: number) => String(part).padStart(2, "0");
    return `${value.getFullYear()}-${pad(value.getMonth() + 1)}-${pad(value.getDate())}`;
  }

  function formatDate(value: string) {
    const date = new Date(value);
    return Number.isNaN(date.getTime())
      ? value
      : new Intl.DateTimeFormat(undefined, {
          year: "numeric",
          month: "long",
          day: "numeric"
        }).format(date);
  }

  function errorMessage(error: unknown) {
    return error instanceof Error ? error.message : "An unexpected error occurred.";
  }
</script>

<svelte:head>
  <title>Competitions · Tupal Management</title>
</svelte:head>

<div class="management-home">
  <header class="home-topbar">
    <a class="brand" href="/">
      <span class="brand-mark" aria-hidden="true">T</span>
      <span>Tupal</span>
    </a>
    <span class="product-label">COMPETITION MANAGEMENT</span>
  </header>

  <main class="home-content">
    <section class="page-intro">
      <p class="eyebrow">MANAGEMENT</p>
      <h1>Your competitions</h1>
      <p>Choose an event to open its management workspace.</p>
    </section>

    <section class="competition-panel" aria-labelledby="competition-list-heading">
      <div class="list-heading">
        <div>
          <h2 id="competition-list-heading">All competitions</h2>
          {#if !loading && !pageError}
            <p>{competitions.length} {competitions.length === 1 ? "competition" : "competitions"}</p>
          {/if}
        </div>
        {#if competitions.length > 0}
          <label class="search-field">
            <span class="visually-hidden">Search competitions</span>
            <input
              type="search"
              bind:value={search}
              placeholder="Search by name or ID"
              autocomplete="off"
            />
          </label>
        {/if}
        <button
          class="create-toggle"
          type="button"
          aria-expanded={formOpen}
          onclick={() => {
            formOpen = !formOpen;
            formError = "";
          }}
        >
          {formOpen ? "Cancel" : "New competition"}
        </button>
      </div>

      {#if formOpen}
        <form class="create-form" onsubmit={submitCompetition}>
          <div class="create-form-heading">
            <h3>Create a competition</h3>
            <p>A unique ID will be generated automatically.</p>
          </div>
          {#if formError}
            <p class="form-error" role="alert">{formError}</p>
          {/if}
          <div class="form-fields">
            <label>
              Competition name
              <input
                type="text"
                bind:value={title}
                maxlength="200"
                required
                autocomplete="off"
                placeholder="e.g. Summer Orienteering"
              />
            </label>
            <label>
              Competition date
              <input type="date" bind:value={date} required />
            </label>
            <button class="submit-button" type="submit" disabled={saving}>
              {saving ? "Creating…" : "Create competition"}
            </button>
          </div>
        </form>
      {/if}

      {#if notice}
        <p class="success-message" role="status">{notice}</p>
      {/if}

      {#if loading}
        <div class="home-state" aria-live="polite">
          <span class="state-spinner" aria-hidden="true"></span>
          Loading competitions…
        </div>
      {:else if pageError}
        <div class="error-state" role="alert">
          <strong>Couldn’t load competitions</strong>
          <span>{pageError}</span>
          <button type="button" class="retry-button" onclick={() => void reload()}>
            Try again
          </button>
        </div>
      {:else if competitions.length === 0}
        <div class="empty-state">
          <span class="empty-icon" aria-hidden="true">▤</span>
          <h3>No competitions yet</h3>
          <p>Create a competition to start setting up your event.</p>
          <button
            class="empty-create-button"
            type="button"
            onclick={() => (formOpen = true)}
          >
            Create competition
          </button>
        </div>
      {:else if filteredCompetitions.length === 0}
        <div class="empty-state">
          <h3>No matching competitions</h3>
          <p>Try another competition name or ID.</p>
        </div>
      {:else}
        <ul class="competition-list">
          {#each filteredCompetitions as competition (competition.id)}
            <li>
              <a
                class="competition-card"
                href={`/management/${encodeURIComponent(competition.id)}`}
              >
                <span class="competition-avatar" aria-hidden="true">
                  {competition.title.slice(0, 1).toLocaleUpperCase()}
                </span>
                <span class="competition-info">
                  <strong>{competition.title}</strong>
                  <span>{formatDate(competition.date)}</span>
                  <small>ID: {competition.id}</small>
                </span>
                <span class="open-label">
                  Open workspace <span aria-hidden="true">→</span>
                </span>
              </a>
            </li>
          {/each}
        </ul>
      {/if}
    </section>
  </main>
</div>

<style>
  .management-home {
    min-height: 100vh;
    background: #f5f7f5;
    color: #1c2921;
  }

  .home-topbar {
    display: flex;
    align-items: center;
    gap: 18px;
    height: 66px;
    padding: 0 max(24px, calc((100vw - 1180px) / 2));
    border-bottom: 1px solid #e1e7e2;
    background: #fff;
  }

  .brand {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    color: #1c2921;
    font-size: 17px;
    font-weight: 750;
    text-decoration: none;
  }

  .brand-mark,
  .competition-avatar {
    display: grid;
    flex: 0 0 auto;
    place-items: center;
    border-radius: 9px;
    background: #216b4b;
    color: #fff;
    font-weight: 800;
  }

  .brand-mark {
    width: 31px;
    height: 31px;
  }

  .product-label {
    padding-left: 18px;
    border-left: 1px solid #e1e7e2;
    color: #718077;
    font-size: 11px;
    font-weight: 750;
    letter-spacing: 0.1em;
  }

  .home-content {
    width: min(940px, 100%);
    margin: 0 auto;
    padding: 54px 24px 72px;
  }

  .page-intro {
    margin-bottom: 30px;
  }

  .eyebrow {
    margin: 0 0 8px;
    color: #327151;
    font-size: 10px;
    font-weight: 800;
    letter-spacing: 0.13em;
  }

  h1,
  h2,
  h3,
  p {
    margin-top: 0;
  }

  h1 {
    margin-bottom: 9px;
    font-size: clamp(29px, 4vw, 38px);
    letter-spacing: -0.04em;
  }

  .page-intro > p:last-child {
    margin-bottom: 0;
    color: #69786f;
    font-size: 15px;
  }

  .competition-panel {
    overflow: hidden;
    border: 1px solid #dfe6e0;
    border-radius: 14px;
    background: #fff;
    box-shadow: 0 2px 12px rgb(22 43 31 / 5%);
  }

  .list-heading {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    padding: 20px 22px;
    border-bottom: 1px solid #edf0ed;
  }

  .create-toggle,
  .submit-button,
  .empty-create-button {
    min-height: 42px;
    padding: 0 15px;
    border: 0;
    border-radius: 8px;
    background: #216b4b;
    color: #fff;
    font: inherit;
    font-size: 12px;
    font-weight: 750;
    white-space: nowrap;
  }

  .create-toggle:hover,
  .submit-button:hover:not(:disabled),
  .empty-create-button:hover {
    background: #19553b;
  }

  .create-form {
    display: grid;
    gap: 15px;
    padding: 20px 22px;
    border-bottom: 1px solid #edf0ed;
    background: #f9fbf9;
  }

  .create-form-heading h3 {
    margin-bottom: 4px;
    color: #25362b;
    font-size: 14px;
  }

  .create-form-heading p {
    margin: 0;
    color: #718077;
    font-size: 12px;
  }

  .form-fields {
    display: grid;
    grid-template-columns: minmax(200px, 1fr) minmax(160px, 0.55fr) auto;
    align-items: end;
    gap: 12px;
  }

  .form-fields label {
    display: grid;
    gap: 6px;
    color: #35443b;
    font-size: 12px;
    font-weight: 650;
  }

  .form-fields input {
    width: 100%;
    min-height: 42px;
    padding: 8px 10px;
    border: 1px solid #d5ded6;
    border-radius: 7px;
    background: #fff;
    color: #1c2921;
    font: inherit;
    font-size: 13px;
  }

  .submit-button:disabled {
    cursor: wait;
    opacity: 0.65;
  }

  .form-error,
  .success-message {
    margin: 0;
    font-size: 12px;
  }

  .form-error {
    color: #a43c36;
  }

  .success-message {
    padding: 12px 22px;
    border-bottom: 1px solid #edf0ed;
    background: #f0f8f2;
    color: #216b4b;
    font-weight: 700;
  }

  .list-heading h2 {
    margin-bottom: 4px;
    font-size: 16px;
  }

  .list-heading p {
    margin: 0;
    color: #77837a;
    font-size: 12px;
  }

  .search-field {
    width: min(320px, 48%);
    margin: 0;
  }

  .search-field input {
    min-height: 42px;
    padding: 9px 12px;
    border: 1px solid #d5ded6;
    border-radius: 8px;
    color: #1c2921;
    font: inherit;
    font-size: 13px;
  }

  .competition-list {
    display: grid;
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .competition-list li + li {
    border-top: 1px solid #edf0ed;
  }

  .competition-card {
    display: flex;
    align-items: center;
    gap: 15px;
    min-height: 91px;
    padding: 16px 22px;
    color: inherit;
    text-decoration: none;
    transition: background 120ms ease;
  }

  .competition-card:hover,
  .competition-card:focus-visible {
    background: #f7faf7;
  }

  .competition-avatar {
    width: 42px;
    height: 42px;
    border-radius: 12px;
    background: #e7f2eb;
    color: #276344;
    font-size: 17px;
  }

  .competition-info {
    display: grid;
    flex: 1;
    min-width: 0;
    gap: 3px;
  }

  .competition-info strong {
    overflow: hidden;
    font-size: 14px;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .competition-info span,
  .competition-info small {
    color: #718077;
    font-size: 12px;
  }

  .competition-info small {
    font-size: 11px;
  }

  .open-label {
    display: flex;
    align-items: center;
    gap: 9px;
    color: #286b48;
    font-size: 12px;
    font-weight: 750;
    white-space: nowrap;
  }

  .open-label span {
    font-size: 16px;
  }

  .home-state,
  .empty-state,
  .error-state {
    display: grid;
    justify-items: center;
    gap: 8px;
    padding: 48px 24px;
    color: #718077;
    text-align: center;
  }

  .home-state {
    display: flex;
    justify-content: center;
  }

  .empty-state h3,
  .error-state strong {
    margin: 4px 0 0;
    color: #25362b;
    font-size: 15px;
  }

  .empty-state p,
  .error-state span {
    margin: 0;
    font-size: 13px;
  }

  .empty-icon {
    display: grid;
    width: 42px;
    height: 42px;
    place-items: center;
    border-radius: 12px;
    background: #edf5ef;
    color: #367550;
    font-size: 20px;
  }

  .empty-create-button {
    margin-top: 6px;
  }

  .state-spinner {
    width: 16px;
    height: 16px;
    border: 2px solid #d8e5db;
    border-top-color: #327151;
    border-radius: 50%;
    animation: spin 0.8s linear infinite;
  }

  .retry-button {
    min-height: 40px;
    margin-top: 6px;
    padding: 0 14px;
    border: 1px solid #cddbd0;
    border-radius: 8px;
    background: #fff;
    color: #286b48;
    font: inherit;
    font-size: 13px;
    font-weight: 700;
  }

  .visually-hidden {
    position: absolute;
    overflow: hidden;
    width: 1px;
    height: 1px;
    padding: 0;
    clip: rect(0, 0, 0, 0);
    white-space: nowrap;
    clip-path: inset(50%);
  }

  @keyframes spin {
    to { transform: rotate(360deg); }
  }

  @media (max-width: 600px) {
    .home-topbar {
      height: 58px;
      padding: 0 16px;
    }

    .product-label {
      padding-left: 13px;
      font-size: 9px;
    }

    .home-content {
      padding: 36px 14px 48px;
    }

    .page-intro {
      margin-bottom: 20px;
    }

    .list-heading {
      align-items: stretch;
      flex-wrap: wrap;
      gap: 13px;
      padding: 16px;
    }

    .search-field {
      width: 100%;
      order: 2;
    }

    .create-toggle {
      margin-left: auto;
    }

    .form-fields {
      grid-template-columns: 1fr;
    }

    .create-form {
      padding: 16px;
    }

    .success-message {
      padding: 12px 16px;
    }

    .competition-card {
      gap: 10px;
      min-height: 84px;
      padding: 13px;
    }

    .competition-avatar {
      width: 36px;
      height: 36px;
      border-radius: 10px;
    }

    .open-label {
      gap: 4px;
      font-size: 0;
    }

    .open-label span {
      font-size: 19px;
    }
  }
</style>
