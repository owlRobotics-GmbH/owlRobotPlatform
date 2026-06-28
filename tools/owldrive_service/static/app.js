const state = {
  devices: [],
  selected: null,
  ws: null,
  samples: [],
  jobs: new Map(),
  images: [],
  configSchema: [],
  configValues: {},
  configDraft: {},
  presets: [],
  motorPresets: [],
  motionPresets: [],
  pcbPresets: [],
  flashDevices: new Set(),
  canServices: [],
  canUsers: [],
  canReceiveLists: [],
  systemPermissions: null,
  targetTimer: null,
  targetHeld: false,
  targetSending: false,
  targetPending: false,
};

const $ = (id) => document.getElementById(id);

const PLOT_SERIES = [
  { key: "velocity", label: "velocity", unit: "rad/s", color: "#4cc9f0" },
  { key: "angle", label: "angle", unit: "rad", color: "#90be6d" },
  { key: "current", label: "current", unit: "A", color: "#f9c74f" },
  { key: "target", label: "target", unit: "controller target", color: "#f94144" },
];

function selectedNode() {
  if (!state.selected) throw new Error("No device selected");
  return state.selected.node_id;
}

async function api(path, options = {}) {
  const res = await fetch(path, options);
  if (!res.ok) throw new Error(await parseApiError(res));
  return res.json();
}

async function parseApiError(res) {
  const text = await res.text();
  if (!text) return `${res.status} ${res.statusText}`;
  try {
    const payload = JSON.parse(text);
    if (typeof payload.detail === "string") return payload.detail;
    if (Array.isArray(payload.detail)) {
      return payload.detail.map((item) => item.msg || item.detail || JSON.stringify(item)).join("; ");
    }
  } catch (_err) {
    return text;
  }
  return text;
}

function renderDevices() {
  const root = $("devices");
  root.innerHTML = "";
  if (!state.devices.length) {
    root.innerHTML = '<p class="muted">No devices found.</p>';
    return;
  }
  for (const device of state.devices) {
    const el = document.createElement("div");
    el.className = "device" + (state.selected?.node_id === device.node_id ? " active" : "");
    el.innerHTML = `<strong>Node ${device.node_id}</strong><div class="muted">FW ${device.firmware_version}, ${device.answer_ms} ms</div>`;
    el.onclick = () => {
      const changed = state.selected?.node_id !== device.node_id;
      state.selected = device;
      $("selected-label").textContent = `Node ${device.node_id}`;
      if (changed) resetDeviceViews();
      renderDevices();
    };
    root.appendChild(el);
  }
}

async function scan() {
  const info = await api("/api/interfaces");
  $("bus-status").textContent = `Active: ${info.active} | available: ${info.interfaces.join(", ") || "none"}`;
  const data = await api("/api/devices");
  state.devices = data.devices;
  if (!state.selected && state.devices.length) state.selected = state.devices[0];
  renderDevices();
  renderFlashDeviceList();
}

function renderFlashDeviceList() {
  const root = $("flash-device-list");
  root.innerHTML = "";
  if (!state.devices.length) {
    root.innerHTML = '<p class="muted">No devices found.</p>';
    return;
  }
  for (const device of state.devices) {
    const label = document.createElement("label");
    const checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.checked = state.flashDevices.has(device.node_id);
    checkbox.onchange = () => {
      if (checkbox.checked) state.flashDevices.add(device.node_id);
      else state.flashDevices.delete(device.node_id);
    };
    label.append(checkbox, document.createTextNode(`Node ${device.node_id} (FW ${device.firmware_version})`));
    root.appendChild(label);
  }
}

function resetDeviceViews() {
  stopLive();
  state.samples = [];
  state.configValues = {};
  state.configDraft = {};
  $("metrics").innerHTML = "";
  $("config-editor").innerHTML = "";
  $("config-summary").textContent = "No config loaded";
  $("config-status").textContent = "";
  setTargetControlEnabled(false);
  drawPlot();
  renderPlotLegend();
}

async function loadImages() {
  const data = await api("/api/firmware/images");
  state.images = data.images || [];
  const select = $("firmware-image");
  select.innerHTML = "";
  for (const image of state.images) {
    const option = document.createElement("option");
    option.value = image.id;
    option.textContent = `${image.source}: ${image.name} (${Math.round(image.size / 1024)} kB)`;
    select.appendChild(option);
  }
  if (!state.images.length) {
    const option = document.createElement("option");
    option.textContent = data.warning || "No images found";
    select.appendChild(option);
  }
}

async function loadConfigSchema() {
  const data = await api("/api/config/schema");
  state.configSchema = data.fields || [];
}

async function loadPresets() {
  const data = await api("/api/config/presets");
  state.presets = data.presets || [];
  const select = $("profile-preset");
  if (select) fillProfilePresetSelect(select);
}

async function loadMotorPresets() {
  const data = await api("/api/config/motor-presets");
  state.motorPresets = data.presets || [];
  const select = $("motor-preset");
  if (select) fillMotorPresetSelect(select);
}

async function loadMotionPresets() {
  const data = await api("/api/config/motion-presets");
  state.motionPresets = data.presets || [];
  const select = $("motion-preset");
  if (select) fillMotionPresetSelect(select);
}

async function loadPcbPresets() {
  const data = await api("/api/config/pcb-presets");
  state.pcbPresets = data.presets || [];
  const select = $("pcb-preset");
  if (select) fillPcbPresetSelect(select);
}

async function loadCanUsers() {
  $("can-users-status").textContent = "Loading...";
  const [permissions, data] = await Promise.all([
    api("/api/system/permissions"),
    api("/api/system/services"),
  ]);
  state.systemPermissions = permissions;
  state.canServices = data.services || [];
  state.canUsers = state.canServices.flatMap((service) => service.can_users || []);
  state.canReceiveLists = data.receive_lists || [];
  renderCanUsers();
  renderCanReceiveLists();
  const canServices = state.canServices.filter((service) => service.can_active).length;
  updateCanUsersStatus(data.error || "");
}

async function loadConfig() {
  const node = selectedNode();
  $("config-status").textContent = `Loading node ${node}...`;
  if (!state.configSchema.length) await loadConfigSchema();
  const data = await api(`/api/devices/${node}/config`);
  state.configValues = data.values || {};
  state.configDraft = { ...state.configValues };
  renderConfigSummary();
  renderConfigEditor();
  selectPresetCombosByCurrentNames();
  $("config-status").textContent = `Node ${node} loaded`;
}

function renderConfigSummary() {
  const root = $("config-summary");
  const profile = state.configDraft.name || "";
  const motor = state.configDraft["motor.name"] || "";
  const pcb = state.configDraft["pcb.name"] || "";
  root.innerHTML = `
    <div><strong>Selected profile:</strong> ${escapeHtml(profile)}</div>
    <div><strong>Motor:</strong> ${escapeHtml(motor)}</div>
    <div><strong>PCB:</strong> ${escapeHtml(pcb)}</div>
  `;
}

function escapeHtml(value) {
  return String(value ?? "").replace(/[&<>"']/g, (ch) => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    '"': "&quot;",
    "'": "&#39;",
  }[ch]));
}

function groupedFields() {
  const groups = {};
  for (const field of state.configSchema) {
    if (field.visible === false) continue;
    if (!groups[field.group]) groups[field.group] = [];
    groups[field.group].push(field);
  }
  return groups;
}

function renderConfigEditor() {
  const root = $("config-editor");
  root.innerHTML = "";
  const groups = groupedFields();
  for (const [group, fields] of Object.entries(groups)) {
    const section = document.createElement("div");
    section.className = "config-group";
    section.innerHTML = `<h3>${group}</h3>`;
    const tools = renderGroupTools(group);
    if (tools) section.appendChild(tools);
    const grid = document.createElement("div");
    grid.className = "field-grid";
    for (const field of fields) {
      const wrap = document.createElement("label");
      wrap.className = "config-field";
      wrap.dataset.path = field.path;
      const value = state.configDraft[field.path];
      const title = `${field.label}${field.reboot ? " (reboot)" : ""}`;
      let input;
      if (field.type === "bool") {
        input = document.createElement("select");
        input.innerHTML = '<option value="false">off</option><option value="true">on</option>';
        input.value = value ? "true" : "false";
      } else if (field.options) {
        input = document.createElement("select");
        field.options.forEach((name, idx) => {
          const option = document.createElement("option");
          option.value = String(idx);
          option.textContent = name;
          input.appendChild(option);
        });
        input.value = String(value ?? 0);
      } else {
        input = document.createElement("input");
        input.type = field.type === "string" ? "text" : "number";
        if (field.type === "float") input.step = field.precision === undefined || field.precision === null ? "any" : String(Math.pow(10, -field.precision));
        else if (field.type !== "string") input.step = "1";
        if (field.min !== null && field.min !== undefined) input.min = field.min;
        if (field.max !== null && field.max !== undefined) input.max = field.max;
        input.value = formatConfigValue(value, field);
      }
      input.oninput = () => {
        const next = coerceConfigValue(input.value, field);
        state.configDraft[field.path] = next;
        wrap.classList.toggle("changed", !sameConfigFieldValue(state.configValues[field.path], next, field));
      };
      wrap.classList.toggle("changed", !sameConfigFieldValue(state.configValues[field.path], value, field));
      wrap.append(document.createTextNode(title), input);
      if (field.reboot) {
        const hint = document.createElement("span");
        hint.className = "muted";
        hint.textContent = "takes effect after reboot";
        wrap.appendChild(hint);
      }
      grid.appendChild(wrap);
    }
    section.appendChild(grid);
    root.appendChild(section);
  }
}

function renderGroupTools(group) {
  if (group === "profile") {
    const tools = document.createElement("div");
    tools.className = "group-tools";
    const select = document.createElement("select");
    select.id = "profile-preset";
    fillProfilePresetSelect(select);
    const apply = document.createElement("button");
    apply.textContent = "Apply profile";
    apply.onclick = () => applyPreset().catch((err) => $("config-status").textContent = err.message);
    tools.append(select, apply);
    return tools;
  }
  if (group === "motor") {
    const tools = document.createElement("div");
    tools.className = "group-tools";
    const select = document.createElement("select");
    select.id = "motor-preset";
    fillMotorPresetSelect(select);
    const apply = document.createElement("button");
    apply.textContent = "Load defaults";
    apply.onclick = () => applyMotorPreset().catch((err) => $("config-status").textContent = err.message);
    const align = document.createElement("button");
    align.textContent = "Sensor auto-align";
    align.onclick = () => sensorAutoAlign().catch((err) => $("config-status").textContent = err.message);
    tools.append(select, apply, align);
    return tools;
  }
  if (group === "motion") {
    const tools = document.createElement("div");
    tools.className = "group-tools";
    const select = document.createElement("select");
    select.id = "motion-preset";
    fillMotionPresetSelect(select);
    const apply = document.createElement("button");
    apply.textContent = "Load defaults";
    apply.onclick = () => applyMotionPreset().catch((err) => $("config-status").textContent = err.message);
    tools.append(select, apply);
    return tools;
  }
  if (group === "pcb") {
    const tools = document.createElement("div");
    tools.className = "group-tools";
    const select = document.createElement("select");
    select.id = "pcb-preset";
    fillPcbPresetSelect(select);
    const apply = document.createElement("button");
    apply.textContent = "Load defaults";
    apply.onclick = () => applyPcbPreset().catch((err) => $("config-status").textContent = err.message);
    tools.append(select, apply);
    return tools;
  }
  return null;
}

function fillProfilePresetSelect(select) {
  select.innerHTML = "";
  for (const preset of state.presets) {
    const option = document.createElement("option");
    option.value = preset.id;
    option.textContent = preset.name;
    select.appendChild(option);
  }
  selectProfilePresetByCurrentName(select);
}

function fillMotorPresetSelect(select) {
  select.innerHTML = "";
  for (const preset of state.motorPresets) {
    const option = document.createElement("option");
    option.value = preset.id;
    option.textContent = `${preset.index}: ${preset.name}`;
    option.title = preset.description || "";
    select.appendChild(option);
  }
  selectMotorPresetByCurrentName(select);
}

function fillPcbPresetSelect(select) {
  select.innerHTML = "";
  for (const preset of state.pcbPresets) {
    const option = document.createElement("option");
    option.value = preset.id;
    option.textContent = `${preset.index}: ${preset.name}`;
    option.title = preset.description || "";
    select.appendChild(option);
  }
  selectPcbPresetByCurrentName(select);
}

function fillMotionPresetSelect(select) {
  select.innerHTML = "";
  for (const preset of state.motionPresets) {
    const option = document.createElement("option");
    option.value = preset.id;
    option.textContent = `${preset.index}: ${preset.name}`;
    option.title = preset.description || "";
    select.appendChild(option);
  }
  selectMotionPresetByCurrentValues(select);
}

function selectProfilePresetByCurrentName(select = $("profile-preset")) {
  if (!select || !state.configDraft.name || !state.presets.length) return;
  const currentName = normalizePresetName(state.configDraft.name);
  const match = state.presets.find((preset) => normalizePresetName(preset.name) === currentName);
  if (match) select.value = match.id;
}

function selectMotorPresetByCurrentName(select = $("motor-preset")) {
  if (!select || !state.configDraft["motor.name"] || !state.motorPresets.length) return;
  const currentName = normalizePresetName(state.configDraft["motor.name"]);
  const match = state.motorPresets.find((preset) => normalizePresetName(preset.name) === currentName);
  if (match) select.value = match.id;
}

function selectPcbPresetByCurrentName(select = $("pcb-preset")) {
  if (!select || !state.configDraft["pcb.name"] || !state.pcbPresets.length) return;
  const currentName = normalizePresetName(state.configDraft["pcb.name"]);
  const match = state.pcbPresets.find((preset) => normalizePresetName(preset.name) === currentName);
  if (match) select.value = match.id;
}

function selectMotionPresetByCurrentValues(select = $("motion-preset")) {
  if (!select || !state.motionPresets.length) return;
  const match = state.motionPresets.find((preset) => Object.entries(preset.values || {}).every(([path, value]) => (
    sameConfigValue(state.configDraft[path], value, typeof value === "number" ? "float" : "string")
  )));
  if (match) select.value = match.id;
}

function selectPresetCombosByCurrentNames() {
  selectProfilePresetByCurrentName();
  selectMotorPresetByCurrentName();
  selectMotionPresetByCurrentValues();
  selectPcbPresetByCurrentName();
}

function normalizePresetName(name) {
  return String(name || "").trim().toLowerCase();
}

function formatConfigValue(value, field) {
  if (value === null || value === undefined) return "";
  if (field.type === "float" && field.precision !== null && field.precision !== undefined) {
    return Number(value).toFixed(field.precision);
  }
  return String(value);
}

function coerceConfigValue(value, field) {
  if (field.type === "bool") return value === true || value === "true" || value === 1 || value === "1";
  if (field.type === "string") return String(value ?? "");
  const number = Number(value);
  return Number.isFinite(number) ? number : 0;
}

function sameConfigValue(a, b, type) {
  if (type === "float") return Math.abs(Number(a) - Number(b)) < 0.00001;
  return String(a) === String(b);
}

function sameConfigFieldValue(a, b, field) {
  if (field.type !== "float") return sameConfigValue(a, b, field.type);
  if (field.precision !== null && field.precision !== undefined) {
    return Math.abs(Number(a) - Number(b)) <= Math.pow(10, -field.precision) / 2;
  }
  return sameConfigValue(a, b, field.type);
}

function changedConfigValues() {
  const values = {};
  for (const field of state.configSchema) {
    if (field.visible === false) continue;
    const oldValue = state.configValues[field.path];
    const newValue = state.configDraft[field.path];
    if (!sameConfigFieldValue(oldValue, newValue, field)) values[field.path] = newValue;
  }
  return values;
}

async function writeConfig(save = false, reboot = false) {
  const changes = changedConfigValues();
  $("config-status").textContent = `Writing ${Object.keys(changes).length} fields...`;
  const result = await jsonRequest("PATCH", `/api/devices/${selectedNode()}/config`, { values: changes, save, reboot });
  $("config-status").textContent = `OK, ${result.bytes_changed} bytes changed${result.reboot_required ? ", reboot recommended" : ""}`;
  if (!reboot) await loadConfig();
}

async function exportConfig() {
  const node = selectedNode();
  if (!Object.keys(state.configValues).length) await loadConfig();
  const exportedAt = new Date();
  const payload = {
    format: "owldrive-config",
    version: 1,
    exported_at: exportedAt.toISOString(),
    node_id: node,
    firmware_version: state.selected?.firmware_version ?? null,
    values: exportConfigValues(),
  };
  const text = JSON.stringify(payload, null, 2);
  const stamp = exportedAt.toISOString().replace(/[-:]/g, "").replace(/\..+$/, "").replace("T", "-");
  downloadTextFile(`owldrive-node${node}-config-${stamp}.json`, text);
  $("config-status").textContent = `Exported node ${node} config`;
}

function exportConfigValues() {
  const values = {};
  const knownPaths = new Set();
  for (const field of state.configSchema) {
    knownPaths.add(field.path);
    if (!Object.prototype.hasOwnProperty.call(state.configValues, field.path)) continue;
    values[field.path] = formatConfigExportValue(state.configValues[field.path], field);
  }
  for (const [path, value] of Object.entries(state.configValues)) {
    if (!knownPaths.has(path)) values[path] = value;
  }
  return values;
}

function formatConfigExportValue(value, field) {
  if (value === null || value === undefined) return value;
  if (field.type === "float" && field.precision !== null && field.precision !== undefined) {
    return Number(Number(value).toFixed(field.precision));
  }
  if (field.type !== "string" && field.type !== "bool") return Number(value);
  return value;
}

async function importConfigFile(file) {
  selectedNode();
  if (!state.configSchema.length) await loadConfigSchema();
  if (!Object.keys(state.configValues).length) await loadConfig();
  const raw = JSON.parse(await file.text());
  const values = raw && typeof raw === "object" && raw.values && typeof raw.values === "object" ? raw.values : raw;
  if (!values || typeof values !== "object" || Array.isArray(values)) throw new Error("Import file does not contain config values");

  let imported = 0;
  let skipped = 0;
  for (const field of state.configSchema) {
    if (field.visible === false) continue;
    if (!Object.prototype.hasOwnProperty.call(values, field.path)) continue;
    state.configDraft[field.path] = coerceConfigValue(values[field.path], field);
    imported += 1;
  }
  for (const path of Object.keys(values)) {
    const field = state.configSchema.find((item) => item.path === path);
    if (!field || field.visible === false) skipped += 1;
  }

  renderConfigSummary();
  renderConfigEditor();
  selectPresetCombosByCurrentNames();
  const changed = Object.keys(changedConfigValues()).length;
  $("config-status").textContent = `Imported ${imported} fields, ${changed} changed${skipped ? `, ${skipped} skipped` : ""}. Use Write to send them.`;
}

function downloadTextFile(filename, text) {
  const blob = new Blob([text], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = filename;
  document.body.appendChild(link);
  link.click();
  link.remove();
  URL.revokeObjectURL(url);
}

async function applyPreset() {
  const presetId = $("profile-preset").value;
  if (!presetId) throw new Error("No profile selected");
  $("config-status").textContent = "Applying profile...";
  const result = await postJSON(`/api/devices/${selectedNode()}/config/apply-preset`, {
    preset_id: presetId,
    save: false,
    reboot: false,
    keep_node_id: true,
  });
  $("config-status").textContent = `Profile applied, ${result.bytes_changed} bytes changed${result.reboot_required ? ", reboot recommended" : ""}`;
  await loadConfig();
}

async function applyMotorPreset() {
  const presetId = $("motor-preset").value;
  if (!presetId) throw new Error("No motor preset selected");
  $("config-status").textContent = "Loading motor defaults...";
  const result = await postJSON(`/api/devices/${selectedNode()}/config/apply-motor-preset`, {
    preset_id: presetId,
    save: false,
    reboot: false,
  });
  $("config-status").textContent = `Motor defaults loaded, ${result.bytes_changed} bytes changed, reboot recommended`;
  await loadConfig();
}

async function sensorAutoAlign() {
  $("config-status").textContent = "Running sensor auto-align...";
  const result = await postJSON(`/api/devices/${selectedNode()}/config/sensor-auto-align`, {});
  await loadConfig();
  const zeroOfs = Number(result.values?.["motor.zeroOfs"]);
  const direction = result.values?.["motor.senDirCW"] ? "CW" : "CCW";
  $("config-status").textContent = `Sensor auto-align complete: zero offset ${Number.isFinite(zeroOfs) ? zeroOfs.toFixed(4) : "updated"}, direction ${direction}`;
}

async function applyMotionPreset() {
  const presetId = $("motion-preset").value;
  if (!presetId) throw new Error("No motion preset selected");
  $("config-status").textContent = "Loading motion defaults...";
  const result = await postJSON(`/api/devices/${selectedNode()}/config/apply-motion-preset`, {
    preset_id: presetId,
    save: false,
    reboot: false,
  });
  $("config-status").textContent = `Motion defaults loaded, ${result.bytes_changed} bytes changed${result.reboot_required ? ", reboot recommended" : ""}`;
  await loadConfig();
}

async function applyPcbPreset() {
  const presetId = $("pcb-preset").value;
  if (!presetId) throw new Error("No PCB preset selected");
  $("config-status").textContent = "Loading PCB defaults...";
  const result = await postJSON(`/api/devices/${selectedNode()}/config/apply-pcb-preset`, {
    preset_id: presetId,
    save: false,
    reboot: false,
  });
  $("config-status").textContent = `PCB defaults loaded, ${result.bytes_changed} bytes changed, reboot recommended`;
  await loadConfig();
}

function drawPlot() {
  const canvas = $("plot");
  const ctx = canvas.getContext("2d");
  const w = canvas.width;
  const h = canvas.height;
  ctx.clearRect(0, 0, w, h);
  ctx.fillStyle = "#0d1318";
  ctx.fillRect(0, 0, w, h);
  ctx.strokeStyle = "#26333b";
  ctx.lineWidth = 1;
  for (let y = 40; y < h; y += 40) {
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(w, y);
    ctx.stroke();
  }
  for (const series of PLOT_SERIES) {
    const range = seriesRange(series.key);
    ctx.strokeStyle = series.color;
    ctx.lineWidth = 2;
    ctx.beginPath();
    state.samples.forEach((sample, i) => {
      const x = (i / Math.max(1, state.samples.length - 1)) * w;
      const y = valueToY(Number(sample[series.key] || 0), range, h);
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    });
    ctx.stroke();
  }
  renderPlotLegend();
}

function renderPlotLegend() {
  const root = $("plot-legend");
  root.innerHTML = "";
  for (const series of PLOT_SERIES) {
    const range = seriesRange(series.key);
    const item = document.createElement("div");
    item.className = "legend-item";
    item.innerHTML = `
      <span class="legend-swatch" style="background:${series.color}"></span>
      <span>${series.label}</span>
      <span class="muted">${series.unit}, ${formatRange(range.min)}..${formatRange(range.max)}</span>
    `;
    root.appendChild(item);
  }
}

function seriesRange(key) {
  const values = state.samples
    .map((sample) => Number(sample[key]))
    .filter((value) => Number.isFinite(value));
  if (!values.length) return { min: -1, max: 1 };
  let min = Math.min(...values);
  let max = Math.max(...values);
  if (Math.abs(max - min) < 1e-9) {
    const pad = Math.max(1, Math.abs(max) * 0.1);
    min -= pad;
    max += pad;
  } else {
    const pad = (max - min) * 0.08;
    min -= pad;
    max += pad;
  }
  return { min, max };
}

function valueToY(value, range, height) {
  const clamped = Math.min(range.max, Math.max(range.min, value));
  return height - ((clamped - range.min) / (range.max - range.min)) * height;
}

function formatRange(value) {
  const abs = Math.abs(value);
  if (abs >= 1000 || (abs > 0 && abs < 0.01)) return value.toExponential(2);
  return value.toFixed(abs >= 10 ? 1 : 3);
}

function renderMetrics(sample) {
  const root = $("metrics");
  root.innerHTML = "";
  for (const key of ["target", "velocity", "angle", "current", "voltage", "supply_voltage", "error"]) {
    const el = document.createElement("div");
    el.className = "metric";
    el.innerHTML = `<span class="muted">${key}</span><strong>${typeof sample[key] === "number" ? sample[key].toFixed(3) : sample[key]}</strong>`;
    root.appendChild(el);
  }
}

function renderCanUsers() {
  const root = $("can-users");
  root.innerHTML = "";
  const onlyCanActive = $("filter-can-active")?.checked || false;
  const services = onlyCanActive ? state.canServices.filter((service) => service.can_active) : state.canServices;
  updateCanUsersStatus();
  if (!services.length) {
    root.innerHTML = `<p class="muted">${onlyCanActive ? "No services with active CAN sockets found." : "No services found."}</p>`;
    return;
  }
  for (const service of services) {
    const el = document.createElement("div");
    el.className = "service-row";
    const canUsers = service.can_users || [];
    const canText = service.can_active ? `${service.can_socket_count} CAN socket${service.can_socket_count === 1 ? "" : "s"}` : "no CAN socket";
    const cmdline = service.cmdline ? `<code>${escapeHtml(service.cmdline)}</code>` : "";
    const stoppedRecord = service.stopped_can_record
      ? `<div class="service-record">Stopped by owlDrive at ${escapeHtml(formatTimestamp(service.stopped_can_record.stopped_at))}</div>`
      : "";
    const details = canUsers.length
      ? canUsers.map((user) => `${escapeHtml(user.command)} PID ${user.pid}, FD ${escapeHtml(user.fd)}, ${escapeHtml(user.protocol)}<br><code>${escapeHtml(user.cmdline || user.command)}</code>`).join("")
      : '<span class="muted">No active CAN socket detected.</span>';
    el.innerHTML = `
      <div>
        <strong>${escapeHtml(service.name)}</strong>
        <div class="muted">${escapeHtml(service.scope)} | ${escapeHtml(service.active)} / ${escapeHtml(service.sub)} | ${escapeHtml(service.enabled)} | ${escapeHtml(canText)}</div>
        <div class="service-actions">
          <button data-service-action="start" data-service-name="${escapeHtml(service.name)}" data-service-scope="${escapeHtml(service.scope)}" ${service.can_start ? "" : "disabled"}>Start</button>
          <button data-service-action="stop" data-service-name="${escapeHtml(service.name)}" data-service-scope="${escapeHtml(service.scope)}" ${service.can_stop ? "" : "disabled"}>Stop</button>
        </div>
      </div>
      <div>
        <div>${escapeHtml(service.description || "")}</div>
        ${stoppedRecord}
        ${cmdline}
        <div class="service-can-details">${details}</div>
        ${service.is_self ? '<div class="muted">Stop is disabled for this web service.</div>' : ""}
      </div>
    `;
    root.appendChild(el);
  }
  root.querySelectorAll("[data-service-action]").forEach((button) => {
    button.onclick = () => controlService(button.dataset.serviceName, button.dataset.serviceAction, button.dataset.serviceScope);
  });
}

function formatTimestamp(value) {
  if (!value) return "";
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) return value;
  return date.toLocaleString();
}

function updateCanUsersStatus(error = "") {
  if (error) {
    $("can-users-status").textContent = error;
    return;
  }
  const total = state.canServices.length;
  const canActive = state.canServices.filter((service) => service.can_active).length;
  const shown = $("filter-can-active")?.checked ? canActive : total;
  const mode = state.systemPermissions?.full_can_detection ? "full detection" : "limited detection";
  $("can-users-status").textContent = `${shown} shown, ${total} total, ${canActive} using CAN, ${mode}`;
}

function renderCanReceiveLists() {
  const root = $("can-receive-lists");
  root.innerHTML = "";
  const lists = state.canReceiveLists.filter((list) => list.entries.length);
  if (!lists.length) {
    root.innerHTML = '<p class="muted">No kernel CAN receive-list entries found.</p>';
    return;
  }
  for (const list of lists) {
    const el = document.createElement("div");
    el.className = "service-row";
    const rows = list.entries.map((entry) =>
      `${escapeHtml(entry.interface)} id ${escapeHtml(entry.can_id)} mask ${escapeHtml(entry.mask)}, ${escapeHtml(entry.matches)} matches, ${escapeHtml(entry.ident)}`
    ).join("<br>");
    el.innerHTML = `<strong>${escapeHtml(list.name)}</strong><div class="muted">${rows}</div>`;
    root.appendChild(el);
  }
}

function startLive() {
  const node = selectedNode();
  stopLive();
  state.samples = [];
  const proto = location.protocol === "https:" ? "wss" : "ws";
  state.ws = new WebSocket(`${proto}://${location.host}/ws/devices/${node}/telemetry`);
  state.ws.onmessage = (ev) => {
    const sample = JSON.parse(ev.data);
    state.samples.push(sample);
    if (state.samples.length > 240) state.samples.shift();
    renderMetrics(sample);
    drawPlot();
  };
}

function stopLive() {
  if (state.ws) state.ws.close();
  state.ws = null;
}

async function postJSON(path, body) {
  return jsonRequest("POST", path, body);
}

async function jsonRequest(method, path, body) {
  return api(path, {
    method,
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
}

function delay(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

function setTargetControlEnabled(enabled) {
  $("target-enable").checked = enabled;
  $("target-slider").disabled = !enabled;
  $("target-value").disabled = !enabled;
  $("target-zero").disabled = !enabled;
  $("target-status").textContent = enabled ? "ready" : "disabled";
  if (!enabled) stopTargetHold(false);
}

function setTargetDraft(value) {
  const numeric = Number(value);
  const safe = Number.isFinite(numeric) ? numeric : 0;
  $("target-slider").value = String(Math.max(Number($("target-slider").min), Math.min(Number($("target-slider").max), safe)));
  $("target-value").value = safe.toFixed(2);
}

function scheduleTargetSend() {
  if (!$("target-enable").checked) return;
  if (state.targetHeld) {
    state.targetPending = true;
    return;
  }
  clearTimeout(state.targetTimer);
  state.targetTimer = setTimeout(() => sendTarget().catch((err) => {
    $("target-status").textContent = err.message;
  }), 120);
}

async function sendTarget() {
  if (state.targetSending) {
    state.targetPending = true;
    return;
  }
  state.targetSending = true;
  const value = Number($("target-value").value);
  try {
    $("target-status").textContent = `sending ${value.toFixed(2)}...`;
    await postJSON(`/api/devices/${selectedNode()}/values`, { value: 0, data: value });
    $("target-status").textContent = `sent ${value.toFixed(2)}`;
  } finally {
    state.targetSending = false;
  }
}

function startTargetHold() {
  if (!$("target-enable").checked || state.targetHeld) return;
  state.targetHeld = true;
  sendTarget().catch((err) => $("target-status").textContent = err.message);
  state.targetTimer = setInterval(() => {
    sendTarget().catch((err) => $("target-status").textContent = err.message);
  }, 100);
}

function stopTargetHold(sendFinal = true) {
  if (state.targetTimer) {
    clearInterval(state.targetTimer);
    clearTimeout(state.targetTimer);
  }
  state.targetTimer = null;
  const wasHeld = state.targetHeld;
  state.targetHeld = false;
  state.targetPending = false;
  if (sendFinal && wasHeld && $("target-enable").checked) {
    sendTarget().catch((err) => $("target-status").textContent = err.message);
  }
}

async function controlService(name, action, scope) {
  $("can-users-status").textContent = `${action === "start" ? "Starting" : "Stopping"} ${name}...`;
  await postJSON(`/api/system/services/${encodeURIComponent(name)}`, { action, scope });
  await loadCanUsers();
}

async function stopCanServices() {
  $("can-users-status").textContent = "Stopping CAN services...";
  const result = await postJSON("/api/system/services/stop-can", {});
  await loadCanUsers();
  const parts = [
    `${result.stopped.length} stopped`,
    `${result.skipped.length} skipped`,
    `${result.failed.length} failed`,
  ];
  $("can-users-status").textContent = parts.join(", ");
  await delay(1200);
  await loadCanUsers();
}

async function startStoppedCanServices() {
  $("can-users-status").textContent = "Starting stopped CAN services...";
  const result = await postJSON("/api/system/services/start-stopped-can", {});
  $("can-users-status").textContent = `${result.started.length} started, ${result.failed.length} failed`;
  await delay(3500);
  await loadCanUsers();
}

async function flashNode(node) {
  const file = $("firmware").files[0];
  if (!file) throw new Error("No firmware selected");
  const form = new FormData();
  form.append("firmware", file);
  try {
    const job = await api(`/api/devices/${node}/flash`, { method: "POST", body: form });
    state.jobs.set(job.id, job);
    renderJobs();
    pollJob(job.id);
  } catch (err) {
    showFlashStartError(node, file.name, err.message);
  }
}

async function flashImageNode(node) {
  const imageId = $("firmware-image").value;
  if (!imageId) throw new Error("No image selected");
  const image = state.images.find((item) => item.id === imageId);
  try {
    const job = await postJSON(`/api/devices/${node}/flash-image`, { image_id: imageId });
    state.jobs.set(job.id, job);
    renderJobs();
    pollJob(job.id);
  } catch (err) {
    showFlashStartError(node, image?.name || imageId, err.message);
  }
}

function checkedFlashDevices() {
  const devices = state.devices.filter((device) => state.flashDevices.has(device.node_id));
  if (!devices.length) throw new Error("No devices checked");
  return devices;
}

function flashCheckedFile() {
  const file = $("firmware").files[0];
  if (!file) {
    showFlashStartError("checked", "external image", "No firmware selected");
    return;
  }
  try {
    flashFileNodes(checkedFlashDevices().map((device) => device.node_id)).catch((err) => {
      showFlashStartError("checked", file.name, err.message);
    });
  } catch (err) {
    showFlashStartError("checked", file.name, err.message);
  }
}

function flashCheckedImage() {
  const imageId = $("firmware-image").value;
  const image = state.images.find((item) => item.id === imageId);
  if (!imageId || !image) {
    showFlashStartError("checked", "built-in image", "No image selected");
    return;
  }
  try {
    flashImageNodes(checkedFlashDevices().map((device) => device.node_id)).catch((err) => {
      showFlashStartError("checked", image.name, err.message);
    });
  } catch (err) {
    showFlashStartError("checked", image.name, err.message);
  }
}

async function flashFileNodes(nodeIds) {
  const file = $("firmware").files[0];
  if (!file) throw new Error("No firmware selected");
  if (nodeIds.length === 1) {
    await flashNode(nodeIds[0]);
    return;
  }
  const form = new FormData();
  form.append("firmware", file);
  form.append("node_ids", JSON.stringify(nodeIds));
  try {
    const job = await api("/api/devices/flash", { method: "POST", body: form });
    state.jobs.set(job.id, job);
    renderJobs();
    pollJob(job.id);
  } catch (err) {
    showFlashStartError("checked", file.name, err.message);
  }
}

async function flashImageNodes(nodeIds) {
  if (nodeIds.length === 1) {
    await flashImageNode(nodeIds[0]);
    return;
  }
  const imageId = $("firmware-image").value;
  const image = state.images.find((item) => item.id === imageId);
  try {
    const job = await postJSON("/api/devices/flash-image", { image_id: imageId, node_ids: nodeIds });
    state.jobs.set(job.id, job);
    renderJobs();
    pollJob(job.id);
  } catch (err) {
    showFlashStartError("checked", image?.name || imageId, err.message);
  }
}

function showFlashStartError(node, filename, message) {
  const id = `flash-error-${Date.now()}`;
  state.jobs.set(id, {
    id,
    node_id: node,
    filename,
    done: 0,
    total: 1,
    state: "failed",
    error: message,
  });
  renderJobs();
}

async function pollJob(id) {
  const timer = setInterval(async () => {
    const job = await api(`/api/jobs/${id}`);
    state.jobs.set(id, job);
    renderJobs();
    if (job.state === "done" || job.state === "failed" || job.state === "cancelled") clearInterval(timer);
  }, 500);
}

async function cancelJob(id) {
  const job = await postJSON(`/api/jobs/${id}/cancel`, {});
  state.jobs.set(id, job);
  renderJobs();
}

function renderJobs() {
  const root = $("jobs");
  root.innerHTML = "";
  for (const job of state.jobs.values()) {
    const pct = Math.round((job.done / Math.max(1, job.total)) * 100);
    const el = document.createElement("div");
    el.className = "job";
    const canCancel = job.state === "queued" || job.state === "running";
    const action = canCancel ? `<button data-cancel-job="${job.id}">Cancel</button>` : "";
    el.innerHTML = `
      <div>
        <strong>Job ${job.id}: Node ${job.node_id}</strong>
        <div class="muted">${job.filename || ""}</div>
        <div class="muted">${job.state} ${pct}% ${job.error || ""}</div>
      </div>
      ${action}
    `;
    root.appendChild(el);
  }
  root.querySelectorAll("[data-cancel-job]").forEach((button) => {
    button.onclick = () => cancelJob(button.dataset.cancelJob).catch((err) => {
      const job = state.jobs.get(Number(button.dataset.cancelJob)) || state.jobs.get(button.dataset.cancelJob);
      if (job) {
        job.error = err.message;
        state.jobs.set(job.id, job);
        renderJobs();
      }
    });
  });
}

document.querySelectorAll(".tabs button").forEach((button) => {
  button.onclick = () => {
    document.querySelectorAll(".tabs button, .tab").forEach((el) => el.classList.remove("active"));
    button.classList.add("active");
    $(button.dataset.tab).classList.add("active");
  };
});

$("scan").onclick = scan;
$("start-live").onclick = () => startLive();
$("stop-live").onclick = () => stopLive();
$("target-enable").onchange = () => setTargetControlEnabled($("target-enable").checked);
$("target-slider").onpointerdown = () => startTargetHold();
$("target-slider").onpointerup = () => stopTargetHold(true);
$("target-slider").onpointercancel = () => stopTargetHold(true);
$("target-slider").onlostpointercapture = () => stopTargetHold(true);
$("target-slider").oninput = () => {
  setTargetDraft($("target-slider").value);
  scheduleTargetSend();
};
$("target-value").oninput = () => {
  setTargetDraft($("target-value").value);
  scheduleTargetSend();
};
$("target-zero").onclick = () => {
  setTargetDraft(0);
  sendTarget().catch((err) => $("target-status").textContent = err.message);
};
$("send-can").onclick = () => postJSON(`/api/devices/${selectedNode()}/values`, { value: Number($("can-value").value), data: Number($("can-data").value) });
$("choose-firmware").onclick = () => $("firmware").click();
$("firmware").onchange = () => {
  $("firmware-name").textContent = $("firmware").files[0]?.name || "No file selected";
};
$("flash-file-checked").onclick = flashCheckedFile;
$("flash-image-checked").onclick = flashCheckedImage;
$("load-config").onclick = () => loadConfig().catch((err) => $("config-status").textContent = err.message);
$("export-config").onclick = () => exportConfig().catch((err) => $("config-status").textContent = err.message);
$("import-config").onclick = () => $("import-config-file").click();
$("import-config-file").onchange = () => {
  const file = $("import-config-file").files[0];
  if (file) importConfigFile(file).catch((err) => $("config-status").textContent = err.message);
  $("import-config-file").value = "";
};
$("write-config").onclick = () => writeConfig(false, false).catch((err) => $("config-status").textContent = err.message);
$("write-save-config").onclick = () => writeConfig(true, false).catch((err) => $("config-status").textContent = err.message);
$("write-save-reboot").onclick = () => writeConfig(true, true).catch((err) => $("config-status").textContent = err.message);
$("refresh-can-users").onclick = () => loadCanUsers().catch((err) => $("can-users-status").textContent = err.message);
$("stop-can-services").onclick = () => stopCanServices().catch((err) => $("can-users-status").textContent = err.message);
$("start-stopped-can-services").onclick = () => startStoppedCanServices().catch((err) => $("can-users-status").textContent = err.message);
$("filter-can-active").onchange = renderCanUsers;

scan().catch((err) => {
  $("bus-status").textContent = err.message;
});
loadImages().catch(() => {});
loadConfigSchema().catch(() => {});
loadPresets().catch(() => {});
loadMotorPresets().catch(() => {});
loadMotionPresets().catch(() => {});
loadPcbPresets().catch(() => {});
loadCanUsers().catch(() => {});
renderPlotLegend();
