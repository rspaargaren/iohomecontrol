(function () {
    function setDisplayStatus(app, message, isError) {
        if (!app.elements.displayStatus) {
            return;
        }

        app.elements.displayStatus.textContent = message;
        app.elements.displayStatus.classList.toggle("error", !!isError);
    }

    async function loadLastAddress(app) {
        try {
            const data = await window.MiOpenApi.requestJson("/api/lastaddr");
            app.elements.lastAddrInput.value = data.address || "";
        } catch (error) {
            console.error("Error fetching last address", error);
        }
    }

    async function loadAuthConfig(app) {
        try {
            const config = await window.MiOpenApi.requestJson("/api/auth");
            app.elements.authUserInput.value = config.user || "";
            app.elements.authPasswordInput.value = "";
        } catch (error) {
            console.error("Error fetching auth config", error);
        }
    }

    async function updateAuthConfig(app) {
        try {
            const result = await window.MiOpenApi.postJson("/api/auth", {
                user: app.elements.authUserInput.value,
                password: app.elements.authPasswordInput.value
            });
            app.logStatus(result.message || "Auth settings updated.");
            app.elements.authPasswordInput.value = "";
        } catch (error) {
            console.error("Error updating auth config", error);
            app.logStatus("Error updating auth config", true);
        }
    }

    async function loadMqttConfig(app) {
        try {
            const config = await window.MiOpenApi.requestJson("/api/mqtt");
            app.elements.mqttUserInput.value = config.user || "";
            app.elements.mqttServerInput.value = config.server || "";
            app.elements.mqttPasswordInput.value = config.password || "";
            app.elements.mqttPortInput.value = config.port || "";
            app.elements.mqttDiscoveryInput.value = config.discovery || "";
        } catch (error) {
            console.error("Error fetching MQTT config", error);
        }
    }

    async function updateMqttConfig(app) {
        try {
            const result = await window.MiOpenApi.postJson("/api/mqtt", {
                user: app.elements.mqttUserInput.value,
                server: app.elements.mqttServerInput.value,
                password: app.elements.mqttPasswordInput.value,
                port: app.elements.mqttPortInput.value,
                discovery: app.elements.mqttDiscoveryInput.value
            });
            app.logStatus(result.message || "MQTT settings updated.");
        } catch (error) {
            console.error("Error updating MQTT config", error);
            app.logStatus("Error updating MQTT config", true);
        }
    }

    function setWifiStatus(app, message, isError) {
        if (app.elements.wifiConfigStatus) {
            app.elements.wifiConfigStatus.textContent = message || "";
            app.elements.wifiConfigStatus.classList.toggle("error", !!isError);
        }
        if (message && typeof window.showToast === "function") {
            window.showToast(message, isError);
        }
    }

    async function loadWifiConfig(app) {
        if (!app.elements.wifiSsidInput) {
            return;
        }
        try {
            const config = await window.MiOpenApi.requestJson("/api/wifi");
            app.elements.wifiSsidInput.value = config.ssid || config.currentSsid || "";
            if (app.elements.wifiPasswordInput) {
                app.elements.wifiPasswordInput.value = "";
            }
            setWifiStatus(app, config.connected ? "WiFi connected" : "WiFi not connected", !config.connected);
        } catch (error) {
            console.error("Error fetching WiFi config", error);
            setWifiStatus(app, error.message || "WiFi settings load failed", true);
        }
    }

    function openWifiScanModal(app, message) {
        if (typeof app.openPopup === "function") {
            app.openPopup("WiFi networks", "", [message || "Scanning WiFi networks..."], [], {
                showSave: false,
                btnShowCancel: true
            });
        }
    }

    function renderWifiScanResults(app, scanResult) {
        const networks = Array.isArray(scanResult) ? scanResult : (scanResult && Array.isArray(scanResult.networks) ? scanResult.networks : []);
        const validNetworks = networks.filter(function (network) { return network && network.ssid; });

        if (app.elements.wifiScanResults) {
            app.elements.wifiScanResults.style.display = "none";
            app.elements.wifiScanResults.textContent = "";
        }

        const content = document.getElementById("popup-content");
        if (!content) {
            return;
        }
        content.textContent = "";

        if (validNetworks.length === 0) {
            const message = document.createElement("p");
            message.textContent = "No WiFi networks found";
            content.appendChild(message);
            setWifiStatus(app, "No WiFi networks found", true);
            return;
        }

        const list = document.createElement("div");
        list.className = "wifi-scan-modal-list";
        validNetworks.forEach(function (network) {
            const button = document.createElement("button");
            button.type = "button";
            button.className = "wifi-scan-result";
            button.textContent = network.ssid + " (" + network.rssi + " dBm" + (network.secure ? ", secure" : ", open") + ")";
            button.addEventListener("click", function () {
                app.elements.wifiSsidInput.value = network.ssid;
                if (typeof app.closePopup === "function") {
                    app.closePopup();
                }
                if (app.elements.wifiPasswordInput) {
                    app.elements.wifiPasswordInput.focus();
                }
            });
            list.appendChild(button);
        });
        content.appendChild(list);
    }

    async function scanWifiNetworks(app) {
        if (!app.elements.wifiScanButton) {
            return;
        }
        app.elements.wifiScanButton.disabled = true;
        openWifiScanModal(app, "Scanning WiFi networks...");
        setWifiStatus(app, "Scanning WiFi networks...");
        try {
            const scanResult = await window.MiOpenApi.requestJson("/api/wifi-scan");
            renderWifiScanResults(app, scanResult);
            const networks = Array.isArray(scanResult) ? scanResult : (scanResult && Array.isArray(scanResult.networks) ? scanResult.networks : []);
            if (networks.length > 0) {
                setWifiStatus(app, networks.length + " WiFi networks found");
            }
        } catch (error) {
            console.error("Error scanning WiFi networks", error);
            setWifiStatus(app, error.message || "WiFi scan failed", true);
        } finally {
            app.elements.wifiScanButton.disabled = false;
        }
    }

    async function saveWifiConfig(app) {
        if (!app.elements.wifiSsidInput || !app.elements.wifiConfigSaveButton) {
            return;
        }
        const ssid = app.elements.wifiSsidInput.value.trim();
        if (!ssid) {
            setWifiStatus(app, "SSID is required", true);
            return;
        }

        app.elements.wifiConfigSaveButton.disabled = true;
        setWifiStatus(app, "Saving WiFi settings...");
        try {
            const result = await window.MiOpenApi.postJson("/api/wifi", {
                ssid: ssid,
                password: app.elements.wifiPasswordInput ? app.elements.wifiPasswordInput.value : ""
            });
            setWifiStatus(app, result.message || "WiFi settings saved, rebooting");
        } catch (error) {
            console.error("Error saving WiFi settings", error);
            setWifiStatus(app, error.message || "Saving WiFi settings failed", true);
            app.elements.wifiConfigSaveButton.disabled = false;
        }
    }

    function setNetworkStatus(app, message, isError) {
        if (!app.elements.networkStatus) {
            return;
        }
        app.elements.networkStatus.textContent = message || "";
        app.elements.networkStatus.classList.toggle("error", !!isError);
        if (message && typeof window.showToast === "function") {
            window.showToast(message, isError);
        }
    }

    async function loadNetworkConfig(app) {
        if (!app.elements.networkHostnameInput) {
            return;
        }
        try {
            const config = await window.MiOpenApi.requestJson("/api/network");
            app.elements.networkHostnameInput.value = config.hostname || "";
            app.elements.networkDhcpInput.checked = config.dhcp !== false;
            app.elements.networkIpInput.value = config.ip || "";
            app.elements.networkMaskInput.value = config.mask || "";
            app.elements.networkGatewayInput.value = config.gateway || "";
            app.elements.networkDns1Input.value = config.dns1 || "";
            app.elements.networkDns2Input.value = config.dns2 || "";
            app.elements.networkSntpInput.value = config.sntp || "";
        } catch (error) {
            console.error("Error fetching network config", error);
            setNetworkStatus(app, error.message || "Network config load failed", true);
        }
    }

    async function saveNetworkConfig(app) {
        if (!app.elements.networkSaveButton || !app.elements.networkHostnameInput) {
            return;
        }
        app.elements.networkSaveButton.disabled = true;
        setNetworkStatus(app, "Saving network config...");
        try {
            const result = await window.MiOpenApi.postJson("/api/network", {
                hostname: app.elements.networkHostnameInput.value,
                dhcp: app.elements.networkDhcpInput.checked,
                ip: app.elements.networkIpInput.value,
                mask: app.elements.networkMaskInput.value,
                gateway: app.elements.networkGatewayInput.value,
                dns1: app.elements.networkDns1Input.value,
                dns2: app.elements.networkDns2Input.value,
                sntp: app.elements.networkSntpInput.value
            });
            setNetworkStatus(app, result.message || "Network config saved, rebooting");
        } catch (error) {
            console.error("Error saving network config", error);
            setNetworkStatus(app, error.message || "Saving network config failed", true);
            app.elements.networkSaveButton.disabled = false;
        }
    }

    function setFallbackStatus(app, message, isError) {
        if (!app.elements.fallbackStatus) {
            return;
        }
        app.elements.fallbackStatus.textContent = message || "";
        app.elements.fallbackStatus.classList.toggle("error", !!isError);
        if (message && typeof window.showToast === "function") {
            window.showToast(message, isError);
        }
    }

    async function loadFallbackConfig(app) {
        if (!app.elements.fallbackEnabledInput) {
            return;
        }
        try {
            const config = await window.MiOpenApi.requestJson("/api/fallback");
            app.elements.fallbackEnabledInput.checked = config.enabled !== false;
            app.elements.fallbackRetriesBootInput.value = config.retriesBoot || 3;
            app.elements.fallbackRetriesRunningInput.value = config.retriesRunning || 3;
            app.elements.fallbackTimeoutInput.value = config.timeout || 600;
        } catch (error) {
            console.error("Error fetching fallback config", error);
            setFallbackStatus(app, error.message || "Fallback AP load failed", true);
        }
    }

    async function saveFallbackConfig(app) {
        if (!app.elements.fallbackSaveButton) {
            return;
        }
        app.elements.fallbackSaveButton.disabled = true;
        try {
            const result = await window.MiOpenApi.postJson("/api/fallback", {
                enabled: app.elements.fallbackEnabledInput.checked,
                retriesBoot: Number(app.elements.fallbackRetriesBootInput.value || 3),
                retriesRunning: Number(app.elements.fallbackRetriesRunningInput.value || 3),
                timeout: Number(app.elements.fallbackTimeoutInput.value || 600)
            });
            setFallbackStatus(app, result.message || "Fallback AP settings saved");
        } catch (error) {
            console.error("Error saving fallback config", error);
            setFallbackStatus(app, error.message || "Fallback AP save failed", true);
        } finally {
            app.elements.fallbackSaveButton.disabled = false;
        }
    }

    async function loadDisplayConfig(app) {
        if (!app.elements.displayEnabledInput) {
            return;
        }

        setDisplayStatus(
            app,
            app.i18nText("status.display_loading", "Display settings loading...")
        );

        try {
            const config = await window.MiOpenApi.requestJson("/api/display");
            const enabled = config.enabled !== false;
            app.elements.displayEnabledInput.checked = enabled;
            setDisplayStatus(
                app,
                enabled
                    ? app.i18nText("status.display_enabled", "Display is enabled")
                    : app.i18nText("status.display_disabled", "Display is disabled")
            );
        } catch (error) {
            console.error("Error fetching display config", error);
            setDisplayStatus(
                app,
                app.i18nText("status.display_load_error", "Could not load display settings"),
                true
            );
            app.logStatus(app.i18nText("log.error_fetching_display", "Error fetching display config"), true);
        }
    }

    let displayUpdateInFlight = false;

    async function updateDisplayConfig(app) {
        if (!app.elements.displayEnabledInput || displayUpdateInFlight) {
            return;
        }

        displayUpdateInFlight = true;
        if (app.elements.displayUpdateButton) {
            app.elements.displayUpdateButton.disabled = true;
        }

        const requestedEnabled = app.elements.displayEnabledInput.checked;
        setDisplayStatus(
            app,
            app.i18nText("status.display_saving", "Saving display setting...")
        );
        try {
            const result = await window.MiOpenApi.postJson("/api/display", {
                enabled: requestedEnabled
            });
            const enabled = result.enabled !== false;
            app.elements.displayEnabledInput.checked = enabled;
            setDisplayStatus(
                app,
                enabled
                    ? app.i18nText("status.display_saved_enabled", "Saved: display enabled")
                    : app.i18nText("status.display_saved_disabled", "Saved: display disabled")
            );
            app.logStatus(result.message || app.i18nText("log.display_updated", "Display settings updated."));
        } catch (error) {
            console.error("Error updating display config", error);
            app.elements.displayEnabledInput.checked = !requestedEnabled;
            setDisplayStatus(
                app,
                app.i18nText("status.display_save_error", "Saving display setting failed"),
                true
            );
            app.logStatus(app.i18nText("log.error_updating_display", "Error updating display config"), true);
        } finally {
            displayUpdateInFlight = false;
            if (app.elements.displayUpdateButton) {
                app.elements.displayUpdateButton.disabled = false;
            }
        }
    }

    async function loadSyslogConfig(app) {
        if (!app.elements.syslogServerInput) {
            return;
        }
        try {
            const config = await window.MiOpenApi.requestJson("/api/syslog");
            app.elements.syslogEnabledInput.checked = config.enabled !== false;
            app.elements.syslogServerInput.value = config.server || "";
            app.elements.syslogPortInput.value = config.port || "";
            app.elements.syslogTagInput.value = config.tag || "";
        } catch (error) {
            console.error("Error fetching syslog config", error);
        }
    }

    let syslogTestInFlight = false;
    let syslogUpdateInFlight = false;

    async function updateSyslogConfig(app) {
        if (!app.elements.syslogServerInput || syslogUpdateInFlight) {
            return;
        }
        syslogUpdateInFlight = true;
        if (app.elements.syslogUpdateButton) {
            app.elements.syslogUpdateButton.disabled = true;
        }
        try {
            const result = await window.MiOpenApi.postJson("/api/syslog", {
                enabled: app.elements.syslogEnabledInput.checked,
                server: app.elements.syslogServerInput.value,
                port: parseInt(app.elements.syslogPortInput.value, 10),
                tag: app.elements.syslogTagInput.value
            });
            app.elements.syslogEnabledInput.checked = result.enabled !== false;
            app.elements.syslogServerInput.value = result.server || "";
            app.elements.syslogPortInput.value = result.port || "";
            app.elements.syslogTagInput.value = result.tag || "";
            app.logStatus(result.message || app.i18nText("log.syslog_updated", "Syslog settings updated."));
        } catch (error) {
            console.error("Error updating syslog config", error);
            app.logStatus(app.i18nText("log.error_updating_syslog", "Error updating syslog config"), true);
        } finally {
            syslogUpdateInFlight = false;
            if (app.elements.syslogUpdateButton) {
                app.elements.syslogUpdateButton.disabled = false;
            }
        }
    }

    async function sendSyslogTest(app) {
        if (syslogTestInFlight) return;
        syslogTestInFlight = true;
        if (app.elements.syslogTestButton) app.elements.syslogTestButton.disabled = true;
        try {
            const result = await window.MiOpenApi.postJson("/api/syslog/test", {});
            if (result.success) {
                app.logStatus(app.i18nText("log.syslog_test_sent", "Test message sent — check your syslog server."));
            } else {
                app.logStatus(app.i18nText("log.syslog_test_failed", "Test failed: ") + (result.message || ""), true);
            }
        } catch (error) {
            console.error("Error sending syslog test", error);
            app.logStatus(app.i18nText("log.error_syslog_test", "Error sending syslog test message"), true);
        } finally {
            syslogTestInFlight = false;
            if (app.elements.syslogTestButton) app.elements.syslogTestButton.disabled = false;
        }
    }

    async function uploadSelectedFile(app, input, url, missingMessage, successMessage, refreshFn) {
        const file = input.files[0];
        if (!file) {
            app.logStatus(missingMessage, true);
            if (typeof window.showToast === "function") {
                window.showToast(missingMessage, true);
            }
            return;
        }

        try {
            const result = await window.MiOpenApi.uploadFile(url, file);
            const message = result.message || successMessage;
            app.logStatus(message);
            if (typeof window.showToast === "function") {
                window.showToast(message);
            }
            input.value = "";
            if (refreshFn) {
                await refreshFn();
            }
        } catch (error) {
            const message = error.message || successMessage;
            app.logStatus(message, true);
            if (typeof window.showToast === "function") {
                window.showToast(message, true);
            }
        }
    }

    function initSettingsTabs() {
        const tabs = Array.from(document.querySelectorAll("[data-settings-tab]"));
        const panels = Array.from(document.querySelectorAll("[data-settings-panel]"));

        function activate(name) {
            tabs.forEach(function (tab) {
                tab.classList.toggle("active", tab.dataset.settingsTab === name);
            });
            panels.forEach(function (panel) {
                const isActive = panel.dataset.settingsPanel === name;
                panel.classList.toggle("active", isActive);
                panel.hidden = !isActive;
            });
        }

        tabs.forEach(function (tab) {
            tab.addEventListener("click", function () {
                activate(tab.dataset.settingsTab);
            });
        });

        const activeTab = tabs.find(function (tab) {
            return tab.classList.contains("active");
        });
        activate(activeTab ? activeTab.dataset.settingsTab : "integration");
    }

    function initSettingsActions() {
        const closeButton = document.getElementById("settings-close");
        if (closeButton) {
            closeButton.addEventListener("click", function () {
                if (typeof window.showPage === "function") {
                    window.showPage("devices");
                }
            });
        }
    }

    function init(app) {
        initSettingsTabs();
        initSettingsActions();

        app.loadLastAddress = function () {
            return loadLastAddress(app);
        };
        app.loadAuthConfig = function () {
            return loadAuthConfig(app);
        };
        app.updateAuthConfig = function () {
            return updateAuthConfig(app);
        };
        app.loadMqttConfig = function () {
            return loadMqttConfig(app);
        };
        app.updateMqttConfig = function () {
            return updateMqttConfig(app);
        };
        app.loadWifiConfig = function () {
            return loadWifiConfig(app);
        };
        app.scanWifiNetworks = function () {
            return scanWifiNetworks(app);
        };
        app.saveWifiConfig = function () {
            return saveWifiConfig(app);
        };
        app.loadNetworkConfig = function () {
            return loadNetworkConfig(app);
        };
        app.saveNetworkConfig = function () {
            return saveNetworkConfig(app);
        };
        app.loadFallbackConfig = function () {
            return loadFallbackConfig(app);
        };
        app.saveFallbackConfig = function () {
            return saveFallbackConfig(app);
        };
        app.loadDisplayConfig = function () {
            return loadDisplayConfig(app);
        };
        app.updateDisplayConfig = function () {
            return updateDisplayConfig(app);
        };
        app.loadSyslogConfig = function () {
            return loadSyslogConfig(app);
        };
        app.updateSyslogConfig = function () {
            return updateSyslogConfig(app);
        };
        app.sendSyslogTest = function () {
            return sendSyslogTest(app);
        };
        app.uploadFirmware = function () {
            return uploadSelectedFile(
                app,
                app.elements.firmwareFileInput,
                "/api/firmware",
                "No firmware file selected",
                "Firmware uploaded"
            );
        };
        app.uploadFilesystem = function () {
            return uploadSelectedFile(
                app,
                app.elements.filesystemFileInput,
                "/api/filesystem",
                "No filesystem file selected",
                "Filesystem uploaded"
            );
        };
        app.uploadDevices = function () {
            return uploadSelectedFile(
                app,
                app.elements.devicesFileInput,
                "/api/upload/devices",
                "No devices file selected",
                "Devices file uploaded",
                async function () {
                    await app.fetchAndDisplayDevices();
                    await app.fetchAndDisplayRemotes();
                }
            );
        };
        app.uploadRemotes = function () {
            return uploadSelectedFile(
                app,
                app.elements.remotesFileInput,
                "/api/upload/remotes",
                "No remotes file selected",
                "Remotes file uploaded",
                function () {
                    return app.fetchAndDisplayRemotes();
                }
            );
        };
    }

    window.MiOpenSettings = {
        init: init
    };
})();
