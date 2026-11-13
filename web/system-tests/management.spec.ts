import { expect, test } from "@playwright/test";

const competitionId = process.env.TUPAL_COMPETITION_ID ?? "system-smoke-comp";

test("management initial view creates a competition and lists its workspace link", async ({
  page,
  request
}) => {
  await page.goto("/management/");
  await page.getByRole("button", { name: "New competition" }).click();
  await page.getByLabel("Competition name").fill("New Management Test Event");
  await page.getByLabel("Competition date").fill("2026-12-05");
  await page.getByRole("button", { name: "Create competition" }).click();

  await expect(page.getByRole("status")).toContainText(
    "New Management Test Event created."
  );
  const competitionResponse = await request.get("/rest/competition/");
  expect(competitionResponse.ok()).toBeTruthy();
  const competitions = await competitionResponse.json();
  const created = competitions.find(
    (competition: { title: string }) =>
      competition.title === "New Management Test Event"
  );
  expect(created).toMatchObject({
    date: expect.stringMatching(/^2026-12-05T/),
    id: expect.any(String)
  });
  const createdResponse = await request.get(
    `/rest/competition/${encodeURIComponent(created.id)}`
  );
  expect(createdResponse.ok()).toBeTruthy();

  const competitionCard = page.getByRole("link", {
    name: /New Management Test Event/
  });
  await expect(competitionCard).toHaveAttribute(
    "href",
    `/management/${created.id}`
  );
  await competitionCard.click();
  await expect(page).toHaveURL(new RegExp(`/management/${created.id}$`));
  await expect(
    page.getByRole("heading", { name: "New Management Test Event" })
  ).toBeVisible();
});

test("management routes open the competition workspace directly", async ({
  page
}) => {
  await page.goto("/management/");

  await expect(page.getByRole("heading", { name: "Your competitions" })).toBeVisible();
  const competitionCard = page
    .getByRole("link", { name: /System Smoke Competition/ })
    .first();
  await expect(competitionCard).toHaveAttribute(
    "href",
    `/management/${competitionId}`
  );
  await page.getByRole("searchbox", { name: "Search competitions" }).fill("no matching competition");
  await expect(
    page.getByRole("heading", { name: "No matching competitions" })
  ).toBeVisible();
  await page.getByRole("searchbox", { name: "Search competitions" }).clear();
  await competitionCard.click();
  await expect(page).toHaveURL(new RegExp(`/management/${competitionId}$`));

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
