import { expect, test } from "@playwright/test";
import { classResults, formatElapsed } from "../src/apps/management/results";
import type { CompetitionClass, Competitor, StartGroup } from "../src/shared/types";
import type { DerivedFinish } from "../src/shared/finishAssignments";

const classInfo: CompetitionClass = { id: "class", title: "Open", start_group_id: "group" };
const group: StartGroup = {
  id: "group", title: "Group", first_start_time: "2026-11-16T10:00:00.000Z",
  first_bib: 1, start_interval: "PT30.000S"
};
const runner = (bib: number, status = 0, offset = "PT0S"): Competitor => ({
  id: `c${bib}`, comp_id: "event", comp_class_id: "class", bib, name: `Runner ${bib}`,
  start_time_offset: offset, finish_time: "", status
});
const finish = (competitor: Competitor, finishTime: string): DerivedFinish => ({
  competitor,
  bibEntry: {
    id: `b${competitor.bib}`, comp_id: "event", sequence_no: competitor.bib,
    bib: competitor.bib, status: 0, confirmed_time_entry_id: null
  },
  timeEntry: { id: `t${competitor.bib}`, comp_id: "event", finish_time: finishTime, status: 0 }
});

test("class results order open and finished competitors, ties, statuses and invalid times", () => {
  const people = [runner(1), runner(2, 0, "PT30S"), runner(3), runner(4, 2),
    runner(5, 3), runner(6, 1), runner(7, 0, "PT600S"), runner(8, 0, "bad"),
    runner(9), runner(10)];
  const assignments = [
    finish(people[0], "2026-11-16T10:01:00.000Z"),
    finish(people[1], "2026-11-16T10:01:30.000Z"),
    finish(people[4], "2026-11-16T10:01:40.000Z"),
    finish(people[8], "2026-11-16T10:02:00.000Z"),
    finish(people[9], "2026-11-16T09:59:00.000Z")
  ];
  const now = new Date(2026, 10, 16, 10, 0, 30).getTime();
  const rows = classResults(classInfo, people, [group], assignments, now);
  expect(rows.map((row) => row.competitor.bib)).toEqual([7, 3, 1, 2, 9, 4, 5, 6, 8, 10]);
  expect(rows.filter((row) => row.rank !== undefined).map((row) => row.rank)).toEqual([1, 1, 3]);
  expect(rows.find((row) => row.competitor.bib === 7)?.elapsedMs).toBe(0);
  expect(rows.find((row) => row.competitor.bib === 3)?.elapsedMs).toBe(30000);
  expect(rows.find((row) => row.competitor.bib === 5)?.finishTime).toContain("10:01:40");
  expect(rows.find((row) => row.competitor.bib === 10)?.error).toContain("before start");
  expect(formatElapsed(3723456)).toBe("1:02:03.456");
});

test("management displays live results for one class, preserving status and reacting to updates", async ({ page, request }, testInfo) => {
  const id = "results-test";
  const base = `/rest/competition/${id}`;
  const post = async (url: string, data: object) => {
    const response = await request.post(url, { data });
    expect(response.ok(), await response.text()).toBeTruthy();
  };
  await post("/rest/competition/", { id, title: "Results test", date: "2026-11-16" });
  await post(`${base}/start_group/`, group);
  await post(`${base}/competition_class/`, classInfo);
  await post(`${base}/competition_class/`, { id: "other", title: "Other", start_group_id: "group" });
  for (const person of [runner(1), runner(2, 0, "PT60S"), runner(3, 0, "PT270S"),
    runner(4, 2), runner(5, 3), runner(6, 1),
    { ...runner(7), comp_class_id: "other" }]) {
    await post(`${base}/competitor/`, person);
  }
  for (const [sequence_no, number, seconds] of [[1, 1, "02:00"], [2, 2, "02:30"], [3, 5, "03:00"]] as const) {
    await post(`${base}/finish_bib_entry/`, { id: `b${number}`, sequence_no, bib: number, status: 0 });
    await post(`${base}/finish_time_entry/`, { id: `t${number}`, finish_time: `2026-11-16T10:${seconds}.000Z`, status: 0 });
  }
  await page.goto(`/management/${id}/results`);
  await expect(page.getByLabel("Class", { exact: true })).toBeVisible();
  await page.clock.install({ time: new Date("2026-11-16T10:05:00Z") });
  await page.clock.fastForward(1000);
  await page.getByLabel("Class", { exact: true }).selectOption("class");
  const table = page.locator(".result-table");
  await expect(table.locator("tbody tr")).toHaveCount(6);
  await expect.poll(() => table.locator("tbody tr").evaluateAll(
    (rows) => rows.map((row) => row.getAttribute("data-bib"))
  )).toEqual(["3", "2", "1", "4", "5", "6"]);
  await expect(table.locator('tr[data-bib="3"]')).toContainText("Open");
  await expect(table.locator('tr[data-bib="2"]')).toContainText("0:01:30.000");
  await expect(table.locator('tr[data-bib="5"]')).toContainText("DSQ");
  await expect(table.locator('tr[data-bib="5"]')).toContainText("10:03:00");
  await page.screenshot({ path: testInfo.outputPath("management-results.png"), fullPage: true });
  const before = await table.locator('tr[data-bib="3"] td:last-child').innerText();
  await page.clock.fastForward(5000);
  await expect.poll(() => table.locator('tr[data-bib="3"] td:last-child').innerText())
    .not.toBe(before);
  await page.getByLabel("Class", { exact: true }).selectOption("other");
  await expect(table.locator("tbody tr")).toHaveCount(1);
  await expect(table.locator("tbody tr")).toContainText("Runner 7");
  await page.getByLabel("Class", { exact: true }).selectOption("class");
  const response = await request.put(`${base}/competitor/c5`, { data: { ...runner(5), status: 0 } });
  expect(response.ok(), await response.text()).toBeTruthy();
  await expect(table.locator('tr[data-bib="5"]')).toContainText("0:03:00.000");
  await expect(table.locator('tr[data-bib="5"]')).not.toContainText("DSQ");
});
