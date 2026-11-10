import { expect, test } from "@playwright/test";

const competitionId = process.env.TUPAL_COMPETITION_ID ?? "system-smoke-comp";

test("times app records finish times and shows the queue difference", async ({
  page,
  request
}) => {
  await page.goto(`/times/${encodeURIComponent(competitionId)}`);

  await expect(
    page.getByRole("heading", { name: "System Smoke Competition" })
  ).toBeVisible();
  await expect(page.getByText("2 bib numbers ahead of times")).toBeVisible();
  await expect(page.getByText("No finish times recorded yet.")).toBeVisible();

  const recordButton = page.getByRole("button", {
    name: "Record finish time"
  });
  for (let count = 1; count <= 3; count += 1) {
    await recordButton.click();
    await expect(page.getByRole("status")).toContainText("recorded.");
    await expect(
      page.getByText(`${count} total`, { exact: true })
    ).toBeVisible();
  }

  await expect(page.getByText("1 time ahead of bib numbers")).toBeVisible();
  const timesResponse = await request.get(
    `/rest/competition/${encodeURIComponent(competitionId)}/finish_time_entry/`
  );
  expect(timesResponse.ok()).toBeTruthy();
  const times = await timesResponse.json();
  expect(times).toHaveLength(3);
  expect(times).toEqual(
    expect.arrayContaining([
      expect.objectContaining({
        finish_time: expect.stringMatching(/^\d{4}-\d{2}-\d{2}T/),
        status: 0
      })
    ])
  );
  await expect(page.locator(".time-list li")).toHaveCount(3);
});
