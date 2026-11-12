import { expect, test } from "@playwright/test";

const competitionId = "system-start-comp";

test("start app lists upcoming competitors and confirms overdue DNS", async ({
  page,
  request
}) => {
  const competitionUrl =
    `/rest/competition/${encodeURIComponent(competitionId)}`;
  const firstStart = Date.now() - 60_000;

  const create = async (route: string, data: object) => {
    const response = await request.post(route, { data });
    expect(response.ok()).toBeTruthy();
  };

  await create("/rest/competition/", {
    id: competitionId,
    title: "Start System Competition",
    date: new Date().toISOString()
  });
  await create(`${competitionUrl}/start_group/`, {
    id: "system-start-group",
    title: "System Start",
    first_start_time: new Date(firstStart).toISOString(),
    first_bib: 701,
    start_interval: "PT30.000S"
  });
  await create(`${competitionUrl}/competition_class/`, {
    id: "system-start-class",
    title: "Open",
    start_group_id: "system-start-group"
  });

  for (const [bib, name, offset] of [
    [701, "Overdue DNS", "PT0.000S"],
    [702, "Overdue starter", "PT30.000S"],
    [703, "Future starter", "PT180.000S"]
  ]) {
    await create(`${competitionUrl}/competitor/`, {
      id: `start-competitor-${bib}`,
      comp_class_id: "system-start-class",
      bib,
      name,
      start_time_offset: offset,
      finish_time: "",
      status: 0
    });
  }

  await page.goto(`/start/${encodeURIComponent(competitionId)}`);
  await expect(
    page.getByRole("heading", { name: "Start System Competition" })
  ).toBeVisible();
  await expect(page.locator(".start-list li")).toHaveCount(1);
  await expect(page.locator('.start-list li[data-bib="703"]')).toContainText(
    "Future starter"
  );
  await expect(page.locator(".dns-list li")).toHaveCount(2);
  await expect(page.locator('.dns-list li[data-bib="701"]')).toContainText(
    "Overdue DNS"
  );

  await page
    .locator('.dns-list li[data-bib="701"]')
    .getByRole("button", { name: "Confirm DNS" })
    .click();
  await expect(page.getByRole("status")).toContainText("Bib 701 confirmed as DNS.");
  await expect(page.locator('.dns-list li[data-bib="701"]')).toHaveCount(0);

  const competitorResponse = await request.get(`${competitionUrl}/competitor/`);
  expect(competitorResponse.ok()).toBeTruthy();
  const competitors = await competitorResponse.json();
  expect(competitors).toEqual(
    expect.arrayContaining([
      expect.objectContaining({ bib: 701, status: 1 }),
      expect.objectContaining({ bib: 702, status: 0 }),
      expect.objectContaining({ bib: 703, status: 0 })
    ])
  );

  await page.goto(`/management/${encodeURIComponent(competitionId)}/results`);
  await expect(page.getByTestId("total-not-started")).toHaveText("1");
  await expect(page.getByTestId("total-started")).toHaveText("1");
  await expect(page.getByTestId("total-dns")).toHaveText("1");
  await expect(page.getByTestId("total-finished")).toHaveText("0");
  await expect(page.getByTestId("total-open")).toHaveText("2");
});
