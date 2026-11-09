import { expect, test } from "@playwright/test";

const competitionId = process.env.TUPAL_COMPETITION_ID ?? "system-smoke-comp";

test("registration UI creates and updates a competitor through the real API", async ({
  page,
  request
}) => {
  await page.goto(`/registration/${encodeURIComponent(competitionId)}`);

  await expect(
    page.getByRole("heading", { name: "System Smoke Competition" })
  ).toBeVisible();
  await expect(page.getByText("No competitors yet")).toBeVisible();

  await page.locator("#competitor-name").fill("Casey Smoke");
  await page.locator("#competitor-class").selectOption("system-smoke-class");
  await expect(page.locator("#competitor-bib")).toHaveValue("500");
  await page.locator("#competitor-offset").fill("0");
  await page.locator("#competitor-submit").click();

  await expect(page.getByRole("status")).toContainText("Casey Smoke registered.");
  await expect(page.getByText("Casey Smoke", { exact: true })).toBeVisible();
  await expect(page.locator("#competitor-bib")).toHaveValue("501");

  const competitorUrl =
    `/rest/competition/${encodeURIComponent(competitionId)}/competitor/`;
  const createdResponse = await request.get(competitorUrl);
  expect(createdResponse.ok()).toBeTruthy();
  const createdCompetitors = await createdResponse.json();
  expect(createdCompetitors).toEqual(
    expect.arrayContaining([
      expect.objectContaining({
        name: "Casey Smoke",
        bib: 500,
        comp_class_id: "system-smoke-class"
      })
    ])
  );

  const createdCompetitorId = createdCompetitors.find(
    (competitor: { id: string; name: string }) => competitor.name === "Casey Smoke"
  ).id;
  await page
    .locator(`[id="edit-competitor-${encodeURIComponent(createdCompetitorId)}"]`)
    .click();
  await expect(page.locator("#competitor-bib")).toHaveValue("500");
  await page.locator("#competitor-name").fill("Casey Updated");
  await page.locator("#competitor-submit").click();

  await expect(page.getByRole("status")).toContainText("Casey Updated updated.");
  await expect(page.getByText("Casey Updated", { exact: true })).toBeVisible();

  const updatedResponse = await request.get(competitorUrl);
  expect(updatedResponse.ok()).toBeTruthy();
  expect(await updatedResponse.json()).toEqual(
    expect.arrayContaining([
      expect.objectContaining({
        name: "Casey Updated",
        bib: 500,
        comp_class_id: "system-smoke-class"
      })
    ])
  );
});
