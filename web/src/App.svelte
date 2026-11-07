<script lang="ts">
  import Registration from "./apps/registration/Registration.svelte";

  const match = window.location.pathname.match(/^\/registration\/([^/]+)\/?$/);
  const competitionId = match ? decodeURIComponent(match[1]) : null;
</script>

{#if competitionId}
  <Registration {competitionId} />
{:else}
  <main class="route-message">
    <div class="brand-mark" aria-hidden="true">T</div>
    <p class="eyebrow">TUPAL · REGISTRATION</p>
    <h1>Open a registration link</h1>
    <p>Use the competition-specific link provided by the organizer.</p>
  </main>
{/if}
