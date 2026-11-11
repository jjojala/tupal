<script lang="ts">
  export let eyebrow: string;
  export let title: string;
  export let description: string;
</script>

<section class="management-page-heading">
  <p class="management-eyebrow">{eyebrow}</p>
  <h1>{title}</h1>
  <p>{description}</p>
</section>

<style>
  .management-page-heading {
    margin-bottom: 27px;
  }

  .management-eyebrow {
    margin: 0 0 9px;
    color: #367550;
    font-size: 10px;
    font-weight: 800;
    letter-spacing: 0.13em;
  }

  h1 {
    margin: 0 0 9px;
    color: #1d2e24;
    font-size: clamp(29px, 3vw, 38px);
    letter-spacing: -0.04em;
    line-height: 1.12;
  }

  .management-page-heading > p:last-child {
    margin: 0;
    color: #69766d;
    font-size: 14px;
  }
</style>
