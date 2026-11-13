import { expect, test } from "@playwright/test";

const competitionId =
  process.env.TUPAL_FINISHLINE_COMPETITION_ID ?? "system-finishline-comp";
const competitionUrl =
  `/rest/competition/${encodeURIComponent(competitionId)}`;

test("finishline records both queues with statuses and toggles DSQ without losing finish time", async ({
  page,
  request
}) => {
  await page.goto(`/finishline/${encodeURIComponent(competitionId)}`);
  await expect(
    page.getByRole("heading", { name: "Finishline System Competition" })
  ).toBeVisible();
  await page.setViewportSize({ width: 390, height: 844 });
  await expect
    .poll(() =>
      page.evaluate(() => document.documentElement.scrollWidth <= window.innerWidth)
    )
    .toBe(true);
  await page.setViewportSize({ width: 1024, height: 768 });
  const captureColumnCount = await page
    .locator(".capture-layout")
    .evaluate((element) =>
      getComputedStyle(element).gridTemplateColumns.split(" ").length
    );
  expect(captureColumnCount).toBe(2);
  await expect(
    page.getByRole("button", { name: /802.*Likely Finishline Competitor/ })
  ).toBeVisible();

  await page.locator("#time-status").selectOption("1");
  await page.getByRole("button", { name: "Record finish time" }).click();
  await expect(page.getByRole("status")).toContainText("Uncertain time recorded");

  await page.locator("#time-status").selectOption("3");
  await page.getByRole("button", { name: "Record finish time" }).click();
  await expect(page.getByRole("status")).toContainText("Missing time recorded");

  await page.locator("#bib-status").selectOption("2");
  await page.locator("#manual-bib").fill("999");
  await page.getByRole("button", { name: "Record bib" }).click();
  await expect(page.getByRole("status")).toContainText("Bib 999 recorded");

  await page.locator("#bib-status").selectOption("0");
  await page
    .getByRole("button", { name: /802.*Likely Finishline Competitor/ })
    .click();
  await expect(page.getByRole("status")).toContainText("Bib 802 recorded");

  await page.getByRole("button", { name: "Record missing bib" }).click();
  await expect(page.getByRole("status")).toContainText("missing-bib entry recorded");

  const timesResponse = await request.get(`${competitionUrl}/finish_time_entry/`);
  const bibsResponse = await request.get(`${competitionUrl}/finish_bib_entry/`);
  expect(timesResponse.ok()).toBeTruthy();
  expect(bibsResponse.ok()).toBeTruthy();
  const times = await timesResponse.json();
  const bibs = await bibsResponse.json();
  expect(times.map((entry: { status: number }) => entry.status)).toEqual([1, 3]);
  expect(bibs.map((entry: { bib: number | null; status: number }) => [
    entry.bib,
    entry.status
  ])).toEqual([
    [999, 2],
    [802, 0],
    [null, 3]
  ]);
  await expect(page.getByText("1 bib number ahead of times")).toBeVisible();

  await page.locator(".dsq-panel summary").click();
  await page.locator("#dsq-search").fill("DSQ Candidate");
  await page
    .getByRole("button", { name: "Disqualify", exact: true })
    .click();
  await expect(page.getByRole("status")).toContainText(
    "Bib 801 disqualified. The recorded finish time was preserved."
  );

  const competitorUrl = `${competitionUrl}/competitor/finishline-competitor-801`;
  let competitorResponse = await request.get(competitorUrl);
  expect(competitorResponse.ok()).toBeTruthy();
  let competitor = await competitorResponse.json();
  expect(competitor.status).toBe(3);
  expect(competitor.finish_time).toBe("2026-11-13T10:00:00.000Z");
  await expect(
    page.getByRole("button", { name: "Remove DSQ", exact: true }).first()
  ).toBeVisible();

  await page
    .getByRole("button", { name: "Remove DSQ", exact: true })
    .first()
    .click();
  await expect(page.getByRole("status")).toContainText("DSQ removed for bib 801.");
  competitorResponse = await request.get(competitorUrl);
  competitor = await competitorResponse.json();
  expect(competitor.status).toBe(0);
  expect(competitor.finish_time).toBe("2026-11-13T10:00:00.000Z");
});
