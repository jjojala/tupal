import { expect, test } from "@playwright/test";
import { deriveFinishAssignments } from "../src/shared/finishAssignments";
import type { Competitor, FinishBibEntry, FinishTimeEntry } from "../src/shared/types";

const competitor = (bib: number, status = 0): Competitor => ({
  id: `c${bib}`, comp_id: "c", comp_class_id: "class", bib, name: `Runner ${bib}`,
  start_time_offset: "PT0S", finish_time: "", status
});
const bib = (sequence: number, number: number | null, status = 0): FinishBibEntry => ({
  id: `b${sequence}`, comp_id: "c", sequence_no: sequence,
  bib: number, status, confirmed_time_entry_id: null
});
const time = (second: number, status = 0): FinishTimeEntry => ({
  id: `t${second}`, comp_id: "c",
  finish_time: `2026-11-16T12:00:${String(second).padStart(2, "0")}.000Z`, status
});

test("derives only clear ordered pairs and resumes when blockers are corrected", () => {
  const competitors = [competitor(1), competitor(2, 1), competitor(3, 3)];
  const times = [time(3), time(1), time(2)];
  const bibs = [bib(3, 3), bib(1, 1), bib(2, 2, 1)];
  let result = deriveFinishAssignments(competitors, bibs, times);
  expect(result.pairs.map((pair) => pair.competitor.bib)).toEqual([1]);
  expect(result.blocker).toContain("Bib entry 2");
  result = deriveFinishAssignments(competitors, [bib(1, 1), bib(2, 2), bib(3, 3)], times);
  expect(result.pairs.map((pair) => pair.timeEntry.id)).toEqual(["t1", "t2", "t3"]);
  expect(result.blocker).toBeNull();
  for (const status of [1, 2, 3]) {
    result = deriveFinishAssignments(competitors, [bib(1, 1), bib(2, 2)], [time(1), time(2, status)]);
    expect(result.pairs).toHaveLength(1);
    expect(result.blocker).toContain("Time entry 2");
    result = deriveFinishAssignments(competitors, [bib(1, 1), bib(2, 2, status)], [time(1), time(2)]);
    expect(result.pairs).toHaveLength(1);
    expect(result.blocker).toContain("Bib entry 2");
  }
  result = deriveFinishAssignments(competitors, [bib(1, null, 3)], [time(1)]);
  expect(result.blocker).toContain("Bib entry 1");
  result = deriveFinishAssignments(competitors, [bib(1, 1)], []);
  expect(result.waitingFor).toBe("time");
  result = deriveFinishAssignments(competitors, [], [time(1)]);
  expect(result.waitingFor).toBe("bib");
  result = deriveFinishAssignments(competitors, [bib(1, 99)], [time(1)]);
  expect(result.blocker).toContain("Bib 99");
  result = deriveFinishAssignments(competitors, [bib(1, 1), bib(2, 1)], [time(1), time(2)]);
  expect(result.pairs).toHaveLength(1);
  expect(result.blocker).toContain("Bib 1");
  result = deriveFinishAssignments(competitors, [bib(1, 1), bib(2, 2)], [time(1), { ...time(1), id: "equal" }]);
  expect(result.pairs).toHaveLength(0);
  expect(result.blocker).toContain("same timestamp");
  result = deriveFinishAssignments(competitors, [{ ...bib(1, 1), confirmed_time_entry_id: "other" }], [time(1)]);
  expect(result.pairs).toHaveLength(1);
});

test("finish views show live derived pairs and resume after correction", async ({ page, request }) => {
  const competition = "derived-finish-test";
  const base = `/rest/competition/${competition}`;
  const post = async (url: string, data: object) => {
    const response = await request.post(url, { data });
    expect(response.ok(), await response.text()).toBeTruthy();
  };
  await post("/rest/competition/", { id: competition, title: "Derived finishes", date: "2026-11-16" });
  await post(`${base}/start_group/`, {
    id: "group", title: "Start", first_start_time: "2026-11-16T10:00:00Z",
    first_bib: 1, start_interval: "PT30S"
  });
  await post(`${base}/competition_class/`, { id: "class", title: "Class", start_group_id: "group" });
  for (const number of [1, 2, 3]) {
    await post(`${base}/competitor/`, {
      id: `c${number}`, bib: number, name: `Runner ${number}`, comp_class_id: "class",
      start_time_offset: "PT0S", finish_time: "", status: number === 2 ? 3 : 0
    });
    await post(`${base}/finish_bib_entry/`, {
      id: `b${number}`, sequence_no: number, bib: number, status: number === 2 ? 1 : 0
    });
    await post(`${base}/finish_time_entry/`, time(number));
  }
  const before = await request.get(`${base}/competitor/c2`);
  const originalFinishTime = (await before.json()).finish_time;
  await page.goto(`/times/${competition}`);
  const view = page.getByRole("region", { name: "Derived finish assignments" });
  await expect(view).toContainText("#1 Runner 1");
  await expect(view).toContainText("Bib entry 2 needs review");
  await expect(view).not.toContainText("#3 Runner 3");
  const response = await request.put(`${base}/finish_bib_entry/b2`, {
    data: { id: "b2", status: 0 }
  });
  expect(response.ok(), await response.text()).toBeTruthy();
  await expect(view).toContainText("#2 Runner 2");
  await expect(view).toContainText("#3 Runner 3");
  await expect(view).toContainText("Automatic pairing is up to date");
  for (const route of ["numbers", "finishline", "management"]) {
    await page.goto(`/${route}/${competition}${route === "management" ? "/finish" : ""}`);
    await expect(page.getByRole("region", { name: "Derived finish assignments" }))
      .toContainText("#3 Runner 3");
  }
  const stored = await request.get(`${base}/competitor/c2`);
  expect((await stored.json()).finish_time).toBe(originalFinishTime);
});
