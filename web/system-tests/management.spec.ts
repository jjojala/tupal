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

test("management setup creates, updates, and safely deletes start groups and classes", async ({
  page,
  request
}) => {
  await page.goto(`/management/${encodeURIComponent(competitionId)}/setup`);

  await page.getByRole("button", { name: "Add start group" }).click();
  await page.getByLabel("Start-group name").fill("Evening start");
  await page.getByLabel("First start").fill("2026-11-11T18:00");
  await page.getByLabel(/First bib/).fill("700");
  await page.getByLabel("Start interval").fill("45");
  await page.getByRole("button", { name: "Create start group" }).click();

  const startGroupsUrl =
    `/rest/competition/${encodeURIComponent(competitionId)}/start_group/`;
  let groupsResponse = await request.get(startGroupsUrl);
  expect(groupsResponse.ok()).toBeTruthy();
  let groups = await groupsResponse.json();
  let group = groups.find(
    (item: { title: string }) => item.title === "Evening start"
  );
  expect(group).toMatchObject({
    first_bib: 700,
    start_interval: "PT45.000S"
  });

  let groupRow = page.getByRole("row").filter({ hasText: "Evening start" });
  await groupRow.getByRole("button", { name: "Edit" }).click();
  await page.getByLabel("Start-group name").fill("Evening start updated");
  await page.getByLabel("First start").fill("2026-11-11T18:30");
  await page.getByLabel(/First bib/).fill("710");
  await page.getByLabel("Start interval").fill("30");
  await page.getByRole("button", { name: "Save changes" }).click();
  await expect(page.getByRole("status")).toContainText("Start group updated.");

  groupsResponse = await request.get(startGroupsUrl);
  groups = await groupsResponse.json();
  group = groups.find(
    (item: { title: string }) => item.title === "Evening start updated"
  );
  expect(group).toMatchObject({
    first_bib: 710,
    start_interval: "PT30.000S"
  });

  await page.getByRole("button", { name: "Add class" }).click();
  await page.getByLabel("Class name").fill("Evening Open");
  await page.getByLabel("Start group").selectOption(group.id);
  await page.getByRole("button", { name: "Create class" }).click();

  const classesUrl =
    `/rest/competition/${encodeURIComponent(competitionId)}/competition_class/`;
  const classesResponse = await request.get(classesUrl);
  expect(classesResponse.ok()).toBeTruthy();
  const classes = await classesResponse.json();
  const classItem = classes.find(
    (item: { title: string }) => item.title === "Evening Open"
  );
  expect(classItem?.start_group_id).toBe(group.id);

  const assignedGroupRow = page
    .getByRole("row")
    .filter({ hasText: "Evening start updated" });
  await expect(
    assignedGroupRow.getByRole("button", { name: "Delete" })
  ).toBeDisabled();

  const classRow = page.getByRole("row").filter({ hasText: "Evening Open" });
  await classRow.getByRole("button", { name: "Edit" }).click();
  await page.getByLabel("Class name").fill("Evening Open Updated");
  await page.getByRole("button", { name: "Save changes" }).click();
  await expect(page.getByRole("status")).toContainText("Class updated.");

  page.once("dialog", (dialog) => dialog.accept());
  await page
    .getByRole("row")
    .filter({ hasText: "Evening Open Updated" })
    .getByRole("button", { name: "Delete" })
    .click();
  await expect(page.getByRole("status")).toContainText("deleted.");

  page.once("dialog", (dialog) => dialog.accept());
  await page
    .getByRole("row")
    .filter({ hasText: "Evening start updated" })
    .getByRole("button", { name: "Delete" })
    .click();
  await expect(page.getByRole("status")).toContainText("deleted.");

  groupsResponse = await request.get(startGroupsUrl);
  expect(await groupsResponse.json()).not.toEqual(
    expect.arrayContaining([expect.objectContaining({ id: group.id })])
  );
});
