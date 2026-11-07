import { svelte } from "@sveltejs/vite-plugin-svelte";
import { defineConfig } from "vite";

export default defineConfig({
  base: "/",
  plugins: [svelte()],
  server: {
    proxy: {
      "/rest": "http://127.0.0.1:8085",
      "/ws": {
        target: "ws://127.0.0.1:8085",
        ws: true
      }
    }
  },
  build: {
    outDir: process.env.TUPAL_WEB_OUTPUT_DIR ?? "dist",
    emptyOutDir: true
  }
});
