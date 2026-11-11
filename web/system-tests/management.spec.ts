import { expect, test } from "@playwright/test";

const competitionId = process.env.TUPAL_COMPETITION_ID ?? "system-smoke-comp";

test("management routes open the competition workspace directly", async ({
  page
}) => {
  await page.goto(`/management/${encodeURIComponent(competitionId)}`);

  await expect(page.getByRole("heading", { name: "System Smoke Competition" })).toBeVisible();
  await expect(page.getByRole("navigation", { name: "Competition navigation" })).toBeVisible();
  await expect(page.getByRole("link", { name: "Overview" })).toHaveAttribute(
    "aria-current",
    "page"
  );
  await expect(page.getByRole("link", { name: "Setup", exact: true })).toBeVisible();
  await expect(
    page.getByRole("link", { name: "Competitors", exact: true })
  ).toBeVisible();
  await expect(page.getByRole("link", { name: "Finish", exact: true })).toBeVisible();
  await expect(page.getByRole("link", { name: "Results", exact: true })).toBeVisible();

  await page.goto(`/management/${encodeURIComponent(competitionId)}/results`);
  await expect(
    page.getByRole("heading", { name: "Results", exact: true })
  ).toBeVisible();
  await expect(page.getByRole("link", { name: "Results" })).toHaveAttribute(
    "aria-current",
    "page"
  );
  await expect(page.getByRole("heading", { name: "Competitor status" })).toBeVisible();

  await page.getByRole("link", { name: "Setup", exact: true }).click();
  await expect(page).toHaveURL(
    new RegExp(`/management/${competitionId}/setup$`)
  );
  await expect(
    page.getByRole("heading", { name: "Competition setup" })
  ).toBeVisible();
});
