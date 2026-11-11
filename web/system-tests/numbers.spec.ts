import { expect, test } from "@playwright/test";

const competitionId =
  process.env.TUPAL_NUMBERS_COMPETITION_ID ?? "system-numbers-comp";

test("numbers app records bibs and filters quick suggestions", async ({
  page,
  request
}) => {
  await page.goto(`/numbers/${encodeURIComponent(competitionId)}`);

  await expect(
    page.getByRole("heading", { name: "Numbers System Competition" })
  ).toBeVisible();
  await expect(page.getByText("2 bib numbers ahead of times")).toBeVisible();
  await expect(page.locator(".suggestion-button")).toHaveCount(1);
  await expect(page.locator(".suggestion-button")).toContainText("502");
  await expect(page.locator(".suggestion-button")).not.toContainText("503");
  await expect(page.locator(".suggestion-button")).not.toContainText("504");
  await expect(page.locator(".suggestion-button")).not.toContainText("505");
  await expect(page.locator(".suggestion-button")).not.toContainText("506");

  await page.locator(".suggestion-button").click();
  await expect(page.getByRole("status")).toContainText(
    "Bib 502 recorded as number 3."
  );
  await expect(page.getByText("3 bib numbers ahead of times")).toBeVisible();

  await page.locator("#competitor-search").fill("Not started");
  const notStartedResult = page.getByRole("button", { name: /503.*Not started yet/ });
  await expect(notStartedResult).toBeVisible();
  await notStartedResult.click();
  await expect(page.getByRole("status")).toContainText(
    "Bib 503 recorded as number 4."
  );

  await page.locator("#manual-bib").fill("999");
  await page.getByRole("button", { name: "Record bib" }).click();
  await expect(page.getByRole("status")).toContainText(
    "Bib 999 recorded as number 5."
  );

  const bibsResponse = await request.get(
    `/rest/competition/${encodeURIComponent(competitionId)}/finish_bib_entry/`
  );
  expect(bibsResponse.ok()).toBeTruthy();
  const bibs = await bibsResponse.json();
  expect(bibs.map((entry: { sequence_no: number; bib: number }) => [
    entry.sequence_no,
    entry.bib
  ])).toEqual([
    [1, 500],
    [2, 501],
    [3, 502],
    [4, 503],
    [5, 999]
  ]);
  await expect(page.locator(".bib-list li")).toHaveCount(5);
});
