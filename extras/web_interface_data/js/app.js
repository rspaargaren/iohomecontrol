(function () {
    function ensureApiModule() {
        if (window.MiOpenApi) {
            return;
        }

        function ensureJson(response) {
            return response.json().catch(function () {
                return {};
            });
        }

        async function requestJson(url, options) {
            const requestOptions = options || {};
            const method = (requestOptions.method || "GET").toUpperCase();
            const requestUrl = method === "GET"
                ? url + (url.indexOf("?") === -1 ? "?" : "&") + "_=" + Date.now()
                : url;

            if (method === "GET") {
                requestOptions.cache = "no-store";
            }

            const response = await fetch(requestUrl, requestOptions);
            const data = await ensureJson(response);
            if (!response.ok) {
                throw new Error(data.message || ("HTTP error " + response.status));
            }
            return data;
        }

        window.MiOpenApi = {
            downloadFile: async function (url, filename) {
                const response = await fetch(url);
                if (!response.ok) {
                    throw new Error("Network response was not ok");
                }

                const blob = await response.blob();
                const link = document.createElement("a");
                link.href = window.URL.createObjectURL(blob);
                link.download = filename;
                link.click();
                window.URL.revokeObjectURL(link.href);
            },
            postJson: function (url, payload) {
                return requestJson(url, {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify(payload)
                });
            },
            requestJson: requestJson,
            uploadFile: function (url, file) {
                const formData = new FormData();
                formData.append("file", file);
                return requestJson(url, {
                    method: "POST",
                    body: formData
                });
            }
        };
    }

    function createElements() {
        return {
            addPopupButton: document.getElementById("add-popup"),
            commandDeviceSelect: document.querySelector("#help-page #device-select"),
            commandInput: document.getElementById("command-input"),
            deviceList: document.getElementById("device-list"),
            devicesFileInput: document.getElementById("devices-file"),
            devicesUploadButton: document.getElementById("upload-devices"),
            downloadDevicesButton: document.getElementById("download-devices"),
            downloadRemotesButton: document.getElementById("download-remotes"),
            filesystemFileInput: document.getElementById("filesystem-file"),
            filesystemUploadButton: document.getElementById("upload-filesystem"),
            firmwareFileInput: document.getElementById("firmware-file"),
            firmwareUploadButton: document.getElementById("upload-firmware"),
            helpDeviceButton: document.getElementById("help-device"),
            helpRemoteButton: document.getElementById("help-remote"),
            lastAddrInput: document.getElementById("last-address"),
            mqttDiscoveryInput: document.getElementById("mqtt-discovery"),
            mqttPasswordInput: document.getElementById("mqtt-password"),
            mqttPortInput: document.getElementById("mqtt-port"),
            mqttServerInput: document.getElementById("mqtt-server"),
            mqttUpdateButton: document.getElementById("mqtt-update"),
            mqttUserInput: document.getElementById("mqtt-user"),
            remotePopupButton: document.getElementById("remote-popup"),
            remotesFileInput: document.getElementById("remotes-file"),
            remotesUploadButton: document.getElementById("upload-remotes"),
            sendCommandButton: document.getElementById("send-command-button"),
            statusMessages: document.getElementById("status-messages"),
            suggestions: document.getElementById("suggestions"),
            themeToggle: document.getElementById("toggle-theme")
        };
    }

    function i18nText(key, fallback) {
        if (typeof window.t === "function") {
            const value = window.t(key);
            if (value && value !== key) {
                return value;
            }
        }
        return fallback || key;
    }

    function logStatus(app, message, isError) {
        const logEntry = document.createElement("p");
        logEntry.textContent = message;
        if (isError) {
            logEntry.style.color = "red";
        }

        app.elements.statusMessages.appendChild(logEntry);
        app.elements.statusMessages.scrollTop = app.elements.statusMessages.scrollHeight;
        while (app.elements.statusMessages.children.length > 20) {
            app.elements.statusMessages.removeChild(app.elements.statusMessages.firstChild);
        }
    }

    function initSuggestions(app) {
        const suggestions = ["add", "remove", "close", "open", "ls", "cat"];
        app.elements.suggestions.textContent = "";

        suggestions.forEach(function (item) {
            const option = document.createElement("option");
            option.value = item;
            option.textContent = item;
            app.elements.suggestions.appendChild(option);
        });

        app.elements.suggestions.addEventListener("change", function () {
            if (!app.elements.suggestions.value) {
                return;
            }

            if (app.elements.commandInput.value !== "" &&
                !app.elements.commandInput.value.endsWith(" ")) {
                app.elements.commandInput.value += " ";
            }

            app.elements.commandInput.value += app.elements.suggestions.value + " ";
            app.elements.commandInput.focus();
            app.elements.suggestions.selectedIndex = 0;
        });
    }

    function initTheme(app) {
        const savedTheme = localStorage.getItem("theme");
        if (savedTheme === "dark") {
            document.body.classList.add("dark-mode");
        }

        if (app.elements.themeToggle) {
            app.elements.themeToggle.addEventListener("click", function () {
                document.body.classList.toggle("dark-mode");
                localStorage.setItem(
                    "theme",
                    document.body.classList.contains("dark-mode") ? "dark" : "light"
                );
            });
        }
    }

    function initHelpButtons(app) {
        if (app.elements.helpDeviceButton) {
            app.elements.helpDeviceButton.addEventListener("click", function () {
                app.openPopup(
                    app.i18nText("popup.help_title", "Help"),
                    app.i18nText("popup.help_device", "help device"),
                    [],
                    app.i18nText("help.device", "No help text").split("\n"),
                    { showSave: false }
                );
            });
        }

        if (app.elements.helpRemoteButton) {
            app.elements.helpRemoteButton.addEventListener("click", function () {
                app.openPopup(
                    app.i18nText("popup.help_title", "Help"),
                    app.i18nText("popup.help_remote", "help remote"),
                    [],
                    app.i18nText("help.remote", "No help text").split("\n"),
                    { showSave: false }
                );
            });
        }
    }

    function initWebSocket(app) {
        const wsScheme = window.location.protocol === "https:" ? "wss" : "ws";
        const ws = new WebSocket(wsScheme + "://" + window.location.host + "/ws");

        ws.onmessage = function (event) {
            const data = JSON.parse(event.data);
            if (data.type === "log") {
                app.logStatus(data.message);
            } else if (data.type === "position") {
                app.updateDeviceFill(data.id, data.position);
            } else if (data.type === "init") {
                app.fetchAndDisplayDevices();
            } else if (data.type === "lastaddr") {
                app.elements.lastAddrInput.value = data.address || "";
            }
        };

        ws.onopen = function () {
            app.logStatus("WebSocket connected");
        };

        ws.onclose = function () {
            app.logStatus("WebSocket disconnected", true);
        };

        app.state.ws = ws;
    }

    function bindEvents(app) {
        if (app.elements.sendCommandButton) {
            app.elements.sendCommandButton.addEventListener("click", app.sendCommand);
        }
        if (app.elements.mqttUpdateButton) {
            app.elements.mqttUpdateButton.addEventListener("click", app.updateMqttConfig);
        }
        if (app.elements.firmwareUploadButton) {
            app.elements.firmwareUploadButton.addEventListener("click", app.uploadFirmware);
        }
        if (app.elements.filesystemUploadButton) {
            app.elements.filesystemUploadButton.addEventListener("click", app.uploadFilesystem);
        }
        if (app.elements.devicesUploadButton) {
            app.elements.devicesUploadButton.addEventListener("click", app.uploadDevices);
        }
        if (app.elements.remotesUploadButton) {
            app.elements.remotesUploadButton.addEventListener("click", app.uploadRemotes);
        }
        if (app.elements.downloadDevicesButton) {
            app.elements.downloadDevicesButton.addEventListener("click", function () {
                window.MiOpenApi.downloadFile("/api/download/devices", "1W.json").catch(function (error) {
                    app.logStatus("Error downloading file: " + error.message, true);
                });
            });
        }
        if (app.elements.downloadRemotesButton) {
            app.elements.downloadRemotesButton.addEventListener("click", function () {
                window.MiOpenApi.downloadFile("/api/download/remotes", "RemoteMap.json").catch(function (error) {
                    app.logStatus("Error downloading file: " + error.message, true);
                });
            });
        }
        if (app.elements.addPopupButton) {
            app.elements.addPopupButton.addEventListener("click", app.openAddDevicePopup);
        }
        if (app.elements.remotePopupButton) {
            app.elements.remotePopupButton.addEventListener("click", app.openAddRemotePopup);
        }
    }

    document.addEventListener("DOMContentLoaded", function () {
        ensureApiModule();

        const app = {
            elements: createElements(),
            i18nText: i18nText,
            logStatus: function (message, isError) {
                logStatus(app, message, isError);
            },
            state: {
                devicesCache: [],
                ws: null
            }
        };

        window.MiOpenPopup.init(app);
        window.MiOpenDevices.init(app);
        window.MiOpenRemotes.init(app);
        window.MiOpenSettings.init(app);
        window.MiOpenApp = app;

        initSuggestions(app);
        initTheme(app);
        initHelpButtons(app);
        initWebSocket(app);
        bindEvents(app);

        window.addEventListener("i18n:changed", function () {
            app.fetchAndDisplayDevices();
            app.fetchAndDisplayRemotes();
        });

        app.logStatus("System started");
        app.logStatus("Loading devices...");
        app.loadMqttConfig();
        app.fetchAndDisplayDevices();
        app.fetchAndDisplayRemotes();
        app.loadLastAddress();
    });
})();
