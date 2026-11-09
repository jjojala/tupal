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

  await page.getByLabel("Competitor name").fill("Casey Smoke");
  await page.getByLabel("Class").selectOption("system-smoke-class");
  await page.getByLabel("Bib number").fill("501");
  await page.getByLabel("Start offset (seconds)").fill("0");
  await page.getByRole("button", { name: "Add competitor" }).click();

  await expect(page.getByRole("status")).toContainText("Casey Smoke registered.");
  await expect(page.getByText("Casey Smoke", { exact: true })).toBeVisible();

  const competitorUrl =
    `/rest/competition/${encodeURIComponent(competitionId)}/competitor/`;
  const createdResponse = await request.get(competitorUrl);
  expect(createdResponse.ok()).toBeTruthy();
  expect(await createdResponse.json()).toEqual(
    expect.arrayContaining([
      expect.objectContaining({
        name: "Casey Smoke",
        bib: 501,
        comp_class_id: "system-smoke-class"
      })
    ])
  );

  await page.getByRole("button", { name: "Edit Casey Smoke" }).click();
  await page.getByLabel("Competitor name").fill("Casey Updated");
  await page.getByRole("button", { name: "Save changes" }).click();

  await expect(page.getByRole("status")).toContainText("Casey Updated updated.");
  await expect(page.getByText("Casey Updated", { exact: true })).toBeVisible();

  const updatedResponse = await request.get(competitorUrl);
  expect(updatedResponse.ok()).toBeTruthy();
  expect(await updatedResponse.json()).toEqual(
    expect.arrayContaining([
      expect.objectContaining({
        name: "Casey Updated",
        bib: 501,
        comp_class_id: "system-smoke-class"
      })
    ])
  );
});
