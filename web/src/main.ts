import { mount } from "svelte";
import App from "./App.svelte";
import "./shared/styles.css";

const target = document.getElementById("app");

if (!target) {
  throw new Error("Missing application mount element");
}

mount(App, { target });
