import { defineConfig, devices } from "@playwright/test";

export default defineConfig({
  testDir: "./system-tests",
  testMatch: "*.spec.ts",
  fullyParallel: false,
  reporter: "list",
  use: {
    baseURL: process.env.TUPAL_BASE_URL,
    timezoneId: "UTC",
    ...devices["Desktop Chrome"]
  }
});
