<script lang="ts">
  import Registration from "./apps/registration/Registration.svelte";
  import Times from "./apps/times/Times.svelte";
  import Numbers from "./apps/numbers/Numbers.svelte";
  import Start from "./apps/start/Start.svelte";
  import Finishline from "./apps/finishline/Finishline.svelte";
  import Management from "./apps/management/Management.svelte";

  type ManagementView =
    | "overview"
    | "setup"
    | "competitors"
    | "finish"
    | "results";

  function isManagementView(value: string | undefined): value is ManagementView {
    return (
      value === "setup" ||
      value === "competitors" ||
      value === "finish" ||
      value === "results"
    );
  }

  const registrationMatch = window.location.pathname.match(
    /^\/registration\/([^/]+)\/?$/
  );
  const timesMatch = window.location.pathname.match(/^\/times\/([^/]+)\/?$/);
  const numbersMatch = window.location.pathname.match(/^\/numbers\/([^/]+)\/?$/);
  const startMatch = window.location.pathname.match(/^\/start\/([^/]+)\/?$/);
  const finishlineMatch = window.location.pathname.match(/^\/finishline\/([^/]+)\/?$/);
  const managementMatch = window.location.pathname.match(
    /^\/management\/([^/]+)(?:\/(setup|competitors|finish|results))?\/?$/
  );
  const registrationCompetitionId = registrationMatch
    ? decodeURIComponent(registrationMatch[1])
    : null;
  const timesCompetitionId = timesMatch
    ? decodeURIComponent(timesMatch[1])
    : null;
  const numbersCompetitionId = numbersMatch
    ? decodeURIComponent(numbersMatch[1])
    : null;
  const startCompetitionId = startMatch
    ? decodeURIComponent(startMatch[1])
    : null;
  const finishlineCompetitionId = finishlineMatch
    ? decodeURIComponent(finishlineMatch[1])
    : null;
  const managementCompetitionId = managementMatch
    ? decodeURIComponent(managementMatch[1])
    : null;
  const managementView = isManagementView(managementMatch?.[2])
    ? managementMatch[2]
    : "overview";
</script>

{#if registrationCompetitionId}
  <Registration competitionId={registrationCompetitionId} />
{:else if timesCompetitionId}
  <Times competitionId={timesCompetitionId} />
{:else if numbersCompetitionId}
  <Numbers competitionId={numbersCompetitionId} />
{:else if startCompetitionId}
  <Start competitionId={startCompetitionId} />
{:else if finishlineCompetitionId}
  <Finishline competitionId={finishlineCompetitionId} />
{:else if managementCompetitionId}
  <Management
    competitionId={managementCompetitionId}
    view={managementView}
  />
{:else}
  <main class="route-message">
    <div class="brand-mark" aria-hidden="true">T</div>
    <p class="eyebrow">TUPAL</p>
    <h1>Open a competition app link</h1>
    <p>Use the competition-specific link provided by the organizer.</p>
  </main>
{/if}
