<script lang="ts">
  import { deriveFinishAssignments } from "./finishAssignments";
  import type { Competitor, FinishBibEntry, FinishTimeEntry } from "./types";

  export let competitors: Competitor[];
  export let bibEntries: FinishBibEntry[];
  export let timeEntries: FinishTimeEntry[];

  $: assignments = deriveFinishAssignments(competitors, bibEntries, timeEntries);
</script>

<section class="assignments" aria-label="Derived finish assignments">
  <h2>Derived finish assignments</h2>
  {#if assignments.blocker}
    <p>Automatic pairing paused: {assignments.blocker}</p>
  {:else if assignments.waitingFor}
    <p>Waiting for {assignments.waitingFor === "bib" ? "a bib number" : "a finish time"}.</p>
  {:else}
    <p>Automatic pairing is up to date.</p>
  {/if}
  {#if assignments.pairs.length}
    <ol>
      {#each assignments.pairs as pair (pair.bibEntry.id)}
        <li>
          <strong>#{pair.competitor.bib} {pair.competitor.name}</strong>
          <span>{pair.timeEntry.finish_time}</span>
        </li>
      {/each}
    </ol>
  {:else}
    <p>No finish times derived yet.</p>
  {/if}
  <p>Assignments are derived from the current queues and are not saved to competitors.</p>
</section>

<style>
  .assignments {
    margin: 18px 0;
    padding: 18px;
    border: 1px solid #dce5de;
    border-radius: 12px;
    background: white;
  }
  h2 { margin: 0 0 10px; font-size: 1.1rem; }
  p { margin: 8px 0; }
  ol { padding-left: 1.5rem; }
  li span { margin-left: 0.6rem; }
</style>
