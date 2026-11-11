import { execFileSync, spawn } from "node:child_process";
import { createServer } from "node:net";
import path from "node:path";
import { once } from "node:events";
import { fileURLToPath } from "node:url";

const webDirectory = fileURLToPath(new URL("../", import.meta.url));
const daemonPath = process.env.TUPAL_SERVER_EXECUTABLE;
const webRoot = process.env.TUPAL_WEB_ROOT;

if (!daemonPath || !webRoot) {
  throw new Error(
    "Set TUPAL_SERVER_EXECUTABLE and TUPAL_WEB_ROOT to run system tests."
  );
}

async function findAvailablePort() {
  const server = createServer();
  server.listen(0, "127.0.0.1");
  await once(server, "listening");
  const { port } = server.address();
  await new Promise((resolve, reject) =>
    server.close((error) => (error ? reject(error) : resolve()))
  );
  return port;
}

async function requestJson(baseUrl, route, body) {
  const response = await fetch(new URL(route, baseUrl), {
    method: body === undefined ? "GET" : "POST",
    headers: body === undefined ? {} : { "Content-Type": "application/json" },
    body: body === undefined ? undefined : JSON.stringify(body)
  });
  const responseBody = await response.text();
  if (!response.ok) {
    throw new Error(
      `${response.status} ${response.statusText} for ${route}: ${responseBody}`
    );
  }
  return responseBody ? JSON.parse(responseBody) : undefined;
}

const port = await findAvailablePort();
const baseUrl = `http://127.0.0.1:${port}`;
const competitionId = "system-smoke-comp";
const numbersCompetitionId = "system-numbers-comp";
let daemonOutput = "";
const daemon = spawn(
  daemonPath,
  [
    "--db",
    "sqlite3://:memory:",
    "--web-root",
    webRoot,
    "--bind",
    baseUrl
  ],
  { cwd: path.dirname(daemonPath), stdio: ["ignore", "pipe", "pipe"] }
);
let daemonStartError;
daemon.on("error", (error) => (daemonStartError = error));
daemon.stdout.setEncoding("utf8").on("data", (chunk) => (daemonOutput += chunk));
daemon.stderr.setEncoding("utf8").on("data", (chunk) => (daemonOutput += chunk));

async function stopDaemon() {
  if (daemonStartError) return;
  if (daemon.exitCode !== null || daemon.signalCode !== null) return;
  daemon.kill();
  await new Promise((resolve) => {
    const timer = setTimeout(() => {
      daemon.kill("SIGKILL");
      resolve();
    }, 3000);
    daemon.once("exit", () => {
      clearTimeout(timer);
      resolve();
    });
  });
}

async function waitForDaemon() {
  const deadline = Date.now() + 20000;
  let lastError;
  while (Date.now() < deadline) {
    if (daemonStartError) {
      throw new Error(`Could not start tupald: ${daemonStartError.message}`);
    }
    if (daemon.exitCode !== null) {
      throw new Error(`tupald exited with ${daemon.exitCode}:\n${daemonOutput}`);
    }
    try {
      await requestJson(baseUrl, "/rest/competition/");
      return;
    } catch (error) {
      lastError = error;
      await new Promise((resolve) => setTimeout(resolve, 200));
    }
  }
  throw new Error(`Timed out waiting for tupald: ${lastError}\n${daemonOutput}`);
}

let testExitCode = 1;
try {
  await waitForDaemon();
  await requestJson(baseUrl, "/rest/competition/", {
    id: competitionId,
    title: "System Smoke Competition",
    date: "2026-11-09T00:00:00.000Z"
  });
  await requestJson(baseUrl, `/rest/competition/${competitionId}/start_group/`, {
    id: "system-smoke-start",
    title: "Smoke Test Start",
    first_start_time: "2026-11-09T10:00:00.000Z",
    first_bib: 500,
    start_interval: "PT30S"
  });
  await requestJson(
    baseUrl,
    `/rest/competition/${competitionId}/competition_class/`,
    {
      id: "system-smoke-class",
      title: "Open",
      start_group_id: "system-smoke-start"
    }
  );

  const tupalCliPath = path.resolve(webDirectory, "../script/tupal");
  for (const [sequenceNo, bib] of [
    [1, 500],
    [2, 501]
  ]) {
    execFileSync(
      tupalCliPath,
      [
        "create",
        "finish-bib-entry",
        "--competition",
        competitionId,
        "--sequence-no",
        String(sequenceNo),
        "--bib",
        String(bib)
      ],
      {
        cwd: webDirectory,
        env: { ...process.env, TUPAL_URL: baseUrl },
        stdio: "pipe"
      }
    );
  }

  const numbersStartTime = new Date(Date.now() - 60 * 60 * 1000).toISOString();
  await requestJson(baseUrl, "/rest/competition/", {
    id: numbersCompetitionId,
    title: "Numbers System Competition",
    date: numbersStartTime
  });
  await requestJson(
    baseUrl,
    `/rest/competition/${numbersCompetitionId}/start_group/`,
    {
      id: "system-numbers-start",
      title: "Numbers Test Start",
      first_start_time: numbersStartTime,
      first_bib: 500,
      start_interval: "PT30S"
    }
  );
  await requestJson(
    baseUrl,
    `/rest/competition/${numbersCompetitionId}/competition_class/`,
    {
      id: "system-numbers-class",
      title: "Numbers Open",
      start_group_id: "system-numbers-start"
    }
  );
  for (const [bib, name, offset, status] of [
    [500, "Already entered A", "PT0.000S", 0],
    [501, "Already entered B", "PT30.000S", 0],
    [502, "Likely finisher", "PT60.000S", 0],
    [503, "Not started yet", "PT86400.000S", 0],
    [504, "Did not start", "PT90.000S", 1],
    [505, "Did not finish", "PT120.000S", 2],
    [506, "Disqualified", "PT150.000S", 3]
  ]) {
    await requestJson(
      baseUrl,
      `/rest/competition/${numbersCompetitionId}/competitor/`,
      {
        id: `numbers-competitor-${bib}`,
        comp_class_id: "system-numbers-class",
        bib,
        name,
        start_time_offset: offset,
        finish_time: "",
        status
      }
    );
  }
  for (const [sequenceNo, bib] of [
    [1, 500],
    [2, 501]
  ]) {
    execFileSync(
      tupalCliPath,
      [
        "create",
        "finish-bib-entry",
        "--competition",
        numbersCompetitionId,
        "--sequence-no",
        String(sequenceNo),
        "--bib",
        String(bib)
      ],
      {
        cwd: webDirectory,
        env: { ...process.env, TUPAL_URL: baseUrl },
        stdio: "pipe"
      }
    );
  }

  const initialCompetition = await requestJson(
    baseUrl,
    `/rest/competition/${competitionId}`
  );
  if (initialCompetition.title !== "System Smoke Competition") {
    throw new Error("REST smoke check did not retrieve the seeded competition.");
  }

  const playwright = spawn(
    process.execPath,
    [
      path.join(webDirectory, "node_modules", "@playwright", "test", "cli.js"),
      "test",
      "--config",
      path.join(webDirectory, "playwright.config.ts")
    ],
    {
      cwd: webDirectory,
      stdio: "inherit",
      env: {
        ...process.env,
        TUPAL_BASE_URL: baseUrl,
        TUPAL_COMPETITION_ID: competitionId,
        TUPAL_NUMBERS_COMPETITION_ID: numbersCompetitionId
      }
    }
  );
  const [code] = await once(playwright, "exit");
  testExitCode = code ?? 1;
} catch (error) {
  console.error(error);
} finally {
  await stopDaemon();
}

if (testExitCode !== 0 && daemonOutput) {
  console.error(`tupald output:\n${daemonOutput}`);
}
process.exitCode = testExitCode;
