<script lang="ts">
  import Registration from "./apps/registration/Registration.svelte";
  import Times from "./apps/times/Times.svelte";
  import Numbers from "./apps/numbers/Numbers.svelte";

  const registrationMatch = window.location.pathname.match(
    /^\/registration\/([^/]+)\/?$/
  );
  const timesMatch = window.location.pathname.match(/^\/times\/([^/]+)\/?$/);
  const numbersMatch = window.location.pathname.match(/^\/numbers\/([^/]+)\/?$/);
  const registrationCompetitionId = registrationMatch
    ? decodeURIComponent(registrationMatch[1])
    : null;
  const timesCompetitionId = timesMatch
    ? decodeURIComponent(timesMatch[1])
    : null;
  const numbersCompetitionId = numbersMatch
    ? decodeURIComponent(numbersMatch[1])
    : null;
</script>

{#if registrationCompetitionId}
  <Registration competitionId={registrationCompetitionId} />
{:else if timesCompetitionId}
  <Times competitionId={timesCompetitionId} />
{:else if numbersCompetitionId}
  <Numbers competitionId={numbersCompetitionId} />
{:else}
  <main class="route-message">
    <div class="brand-mark" aria-hidden="true">T</div>
    <p class="eyebrow">TUPAL</p>
    <h1>Open a competition app link</h1>
    <p>Use the competition-specific link provided by the organizer.</p>
  </main>
{/if}
