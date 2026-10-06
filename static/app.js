const historyKey = "home-control-history";

function showToast(message) {
    const toast = document.getElementById("toast");
    toast.textContent = message;
    toast.classList.add("show");
    setTimeout(() => toast.classList.remove("show"), 2400);
}

async function sendCommand(device, action) {
    const buttons = document.querySelectorAll(`[data-device="${device}"] .control-btn`);
    buttons.forEach(btn => btn.disabled = true);

    try {
        const response = await fetch(`/api/device/${device}/${action}`, {
            method: "POST",
            headers: { "Content-Type": "application/json" }
        });

        const data = await response.json();

        if (!response.ok || !data.ok) {
            throw new Error(data.error || "Command failed");
        }

        addHistory(device, action);
        showToast(`${capitalize(device)} ${action} command sent`);
        await refreshStatus();
    } catch (error) {
        showToast(`Error: ${error.message}`);
    } finally {
        buttons.forEach(btn => btn.disabled = false);
    }
}

async function refreshStatus() {
    try {
        const response = await fetch("/api/status", { cache: "no-store" });
        const data = await response.json();

        updateConnection(data.connected);

        for (const device of ["light", "fan", "geyser"]) {
            updateDeviceCard(device, data.devices?.[device] || "OFF");
        }

        document.getElementById("temperature").textContent =
            data.sensors?.temperature == null ? "—" : `${data.sensors.temperature} °C`;

        document.getElementById("humidity").textContent =
            data.sensors?.humidity == null ? "—" : `${data.sensors.humidity} %`;

        document.getElementById("gas").textContent =
            data.sensors?.gas == null ? "—" : String(data.sensors.gas);

        document.getElementById("lastUpdate").textContent =
            data.last_update ? formatDate(data.last_update) : "Waiting for data";

        const command = data.last_command;
        document.getElementById("lastCommand").textContent =
            command ? `${capitalize(command.device)} → ${command.action} at ${formatDate(command.time)}` : "No command yet";

        document.getElementById("espStatus").textContent =
            data.connected ? "ESP32 / MQTT: connected" : "ESP32 status: waiting";
    } catch {
        updateConnection(false);
    }
}

function updateConnection(connected) {
    const badge = document.getElementById("connectionBadge");
    const text = document.getElementById("connectionText");
    const metric = document.getElementById("mqttMetric");

    badge.classList.toggle("online", connected);
    badge.classList.toggle("offline", !connected);
    text.textContent = connected ? "MQTT Connected" : "MQTT Disconnected";
    metric.textContent = connected ? "Connected" : "Disconnected";
}

function updateDeviceCard(device, state) {
    const card = document.querySelector(`[data-device="${device}"]`);
    if (!card) return;

    const stateEl = card.querySelector("[data-state]");
    const normalized = String(state).toUpperCase();

    stateEl.textContent = normalized;
    stateEl.classList.toggle("on", normalized === "ON");
    stateEl.classList.toggle("off", normalized !== "ON");
}

function addHistory(device, action) {
    const items = JSON.parse(localStorage.getItem(historyKey) || "[]");

    items.unshift({
        device,
        action,
        time: new Date().toLocaleTimeString()
    });

    localStorage.setItem(historyKey, JSON.stringify(items.slice(0, 20)));
    renderHistory();
}

function renderHistory() {
    const container = document.getElementById("historyList");
    const items = JSON.parse(localStorage.getItem(historyKey) || "[]");

    if (!items.length) {
        container.innerHTML = '<div class="empty-history">No commands yet.</div>';
        return;
    }

    container.innerHTML = items.map(item => `
        <div class="history-row">
            <span class="history-time">${escapeHtml(item.time)}</span>
            <span class="history-action">${capitalize(item.device)}</span>
            <strong>${escapeHtml(item.action)}</strong>
        </div>
    `).join("");
}

function clearHistory() {
    localStorage.removeItem(historyKey);
    renderHistory();
    showToast("History cleared");
}

function capitalize(value) {
    return value.charAt(0).toUpperCase() + value.slice(1);
}

function formatDate(value) {
    const date = new Date(value);
    return Number.isNaN(date.getTime()) ? value : date.toLocaleTimeString();
}

function escapeHtml(value) {
    return String(value)
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#039;");
}

renderHistory();
refreshStatus();
setInterval(refreshStatus, 2000);
