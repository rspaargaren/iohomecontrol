(function () {
    async function runAction(app, deviceId, action) {
        const result = await window.MiOpenApi.postJson("/api/action", {
            deviceId: deviceId,
            action: action
        });
        app.logStatus(result.message || ("Action " + action + " sent."));
    }

    function updateDeviceFill(deviceId, percent) {
        const deviceEl = document.querySelector('.device[data-id="' + deviceId + '"]');
        if (!deviceEl) {
            return;
        }

        deviceEl.style.background = "linear-gradient(to top, var(--color-input) " +
            percent + "%, var(--color-accent3) " + percent + "%)";
    }

    function createDeviceButton(label, className, onClick) {
        const button = document.createElement("button");
        button.textContent = label;
        button.classList.add("btn", className);
        button.addEventListener("click", onClick);
        return button;
    }

    async function fetchAndDisplayDevices(app) {
        const deviceList = app.elements.deviceList;
        const deviceSelect = app.elements.commandDeviceSelect;

        try {
            const devices = await window.MiOpenApi.requestJson("/api/devices");
            app.state.devicesCache = devices;

            deviceList.textContent = "";
            deviceSelect.textContent = "";

            if (devices.length === 0) {
                app.logStatus("No devices found.");
                const listItem = document.createElement("li");
                listItem.textContent = "No devices available.";
                deviceList.appendChild(listItem);
                return;
            }

            devices.forEach(function (device) {
                const nameSpan = document.createElement("span");
                nameSpan.textContent = device.name;

                const listItem = document.createElement("li");
                listItem.classList.add("device");
                listItem.dataset.id = device.id;
                listItem.appendChild(nameSpan);

                listItem.appendChild(createDeviceButton("up", "open", function () {
                    runAction(app, device.id, "open").catch(function (error) {
                        app.logStatus(error.message, true);
                    });
                }));

                listItem.appendChild(createDeviceButton("stop", "stop", function () {
                    runAction(app, device.id, "stop").catch(function (error) {
                        app.logStatus(error.message, true);
                    });
                }));

                listItem.appendChild(createDeviceButton("down", "down", function () {
                    runAction(app, device.id, "close").catch(function (error) {
                        app.logStatus(error.message, true);
                    });
                }));

                listItem.appendChild(createDeviceButton(app.i18nText("button.edit", "edit"), "edit", async function () {
                    try {
                        const freshDevices = await window.MiOpenApi.requestJson("/api/devices");
                        app.state.devicesCache = freshDevices;
                        const freshDevice = freshDevices.find(function (candidate) {
                            return candidate.id === device.id;
                        });
                        if (freshDevice) {
                            device = freshDevice;
                        }
                    } catch (error) {
                        app.logStatus("Error refreshing device: " + error.message, true);
                    }

                    app.openPopup(
                        app.i18nText("popup.edit_device_title", "Edit Device"),
                        app.i18nText("popup.adjust_name", "Adjust the name:"),
                        [
                            app.i18nText("popup.info_id", "ID: {value}").replace("{value}", device.id),
                            app.i18nText("popup.info_description", "Description: {value}").replace("{value}", device.description || ""),
                            app.i18nText("popup.info_position", "Position: {value}%").replace("{value}", String(device.position)),
                            app.i18nText("popup.info_paired", "Paired: {value}").replace(
                                "{value}",
                                device.paired ? app.i18nText("value.yes", "Yes") : app.i18nText("value.no", "No")
                            )
                        ],
                        [""],
                        {
                            showSave: true,
                            showInput: true,
                            showTiming: true,
                            btnShowDelete: true,
                            defaultValue: device.name,
                            defaultTiming: device.travel_time,
                            showBoolean: true,
                            booleanLabel: app.i18nText("popup.active", "Active"),
                            defaultBoolean: !!device.active,
                            blockDestructiveWhenBoolean: true,
                            showRepeatOnNoResponse: true,
                            repeatOnNoResponseLabel: app.i18nText(
                                "popup.repeat_on_no_response",
                                "Repeat command if shutter does not respond"
                            ),
                            defaultRepeatOnNoResponse: !!device.repeatOnNoResponse,
                            protectedMessage: app.i18nText(
                                "popup.active_blocks_destructive",
                                "Disable Active before unpairing or deleting."
                            ),
                            pairLabel: app.i18nText("popup.pair_label_device", "Add / Remove the device to the physical screen"),
                            deleteInfo: app.i18nText("popup.delete_device_info", "Only use when the device is not linked to a physical screen."),
                            onSave: async function (newName, newTiming, _deviceValue, repeatOnNoResponse) {
                                try {
                                    if (newName.trim() && newName !== device.name) {
                                        const renameResult = await window.MiOpenApi.postJson("/api/command", {
                                            deviceId: device.id,
                                            command: "edit1W " + newName
                                        });
                                        app.logStatus(renameResult.message || "Device renamed.");
                                    }

                                    const parsedTiming = parseInt(newTiming, 10);
                                    if (!isNaN(parsedTiming) && parsedTiming > 0 && parsedTiming !== device.travel_time) {
                                        const timeResult = await window.MiOpenApi.postJson("/api/command", {
                                            deviceId: device.id,
                                            command: "time1W " + parsedTiming
                                        });
                                        app.logStatus(timeResult.message || "Travel time updated.");
                                    }

                                    if (typeof repeatOnNoResponse === "boolean" &&
                                            repeatOnNoResponse !== !!device.repeatOnNoResponse) {
                                        const repeatResult = await window.MiOpenApi.postJson("/api/command", {
                                            deviceId: device.id,
                                            command: "repeat1W " + (repeatOnNoResponse ? "1" : "0")
                                        });
                                        app.logStatus(repeatResult.message || "Repeat setting updated.");
                                    }

                                    await fetchAndDisplayDevices(app);
                                } catch (error) {
                                    app.logStatus("Error updating device: " + error.message, true);
                                }
                            },
                            onPair: async function () {
                                try {
                                    const result = await window.MiOpenApi.postJson("/api/command", {
                                        deviceId: device.id,
                                        command: "add"
                                    });
                                    app.logStatus(result.message || "Device added.");
                                    await fetchAndDisplayDevices(app);
                                } catch (error) {
                                    app.logStatus("Error adding device: " + error.message, true);
                                }
                            },
                            onUnpair: async function () {
                                try {
                                    const result = await window.MiOpenApi.postJson("/api/command", {
                                        deviceId: device.id,
                                        command: "remove"
                                    });
                                    app.logStatus(result.message || "Device unpaired.");
                                    await fetchAndDisplayDevices(app);
                                } catch (error) {
                                    app.logStatus("Error unpairing device: " + error.message, true);
                                }
                            },
                            onDelete: async function () {
                                const result = await window.MiOpenApi.postJson("/api/command", {
                                    deviceId: device.id,
                                    command: "del1W"
                                });
                                app.logStatus(result.message || "Device deleted.");
                                await fetchAndDisplayDevices(app);
                            }
                        }
                    );
                }));

                deviceList.appendChild(listItem);
                updateDeviceFill(device.id, device.position || 0);

                const option = document.createElement("option");
                option.value = device.id;
                option.textContent = device.name;
                deviceSelect.appendChild(option);
            });

            app.logStatus("Device list updated.");
        } catch (error) {
            app.logStatus("Error fetching devices: " + error.message, true);
            console.error("Error fetching devices:", error);
        }
    }

    async function sendCommand(app) {
        const selectedDeviceId = app.elements.commandDeviceSelect.value;
        const commandStr = app.elements.commandInput.value.trim();

        if (!selectedDeviceId) {
            app.logStatus("Please select a device.", true);
            return;
        }
        if (!commandStr) {
            app.logStatus("Please enter a command.", true);
            return;
        }

        app.logStatus('Sending command "' + commandStr + '" to device ID ' + selectedDeviceId + "...");

        try {
            const result = await window.MiOpenApi.postJson("/api/command", {
                deviceId: selectedDeviceId,
                command: commandStr
            });

            if (result.success) {
                app.logStatus("Command success: " + (result.message || "Command processed."));
            } else {
                app.logStatus("Command failed: " + (result.message || "Unknown error."), true);
            }
        } catch (error) {
            app.logStatus("Error sending command: " + error.message, true);
            console.error("Error sending command:", error);
        }
    }

    // -------------------------------------------------------------------------
    // Pair Wizard
    // -------------------------------------------------------------------------
    // Step 1 – Scan: instruct user to put screen in pair mode; auto-detect
    //          the incoming address by polling /api/scan every 1.5 s.
    // Step 2 – Confirm: show detected address, ask for a friendly name,
    //          then call new1W <name> + add to complete pairing.
    // -------------------------------------------------------------------------

    function openPairWizard(app) {
        let scanInterval = null;
        let detectedAddress = null;
        let wizardCancelled = false;

        // Overlay container (sits on top of everything, outside the normal popup)
        const overlay = document.createElement("div");
        overlay.id = "pair-wizard-overlay";
        overlay.style.cssText = [
            "position:fixed",
            "inset:0",
            "background:rgba(0,0,0,0.55)",
            "display:flex",
            "align-items:center",
            "justify-content:center",
            "z-index:9999"
        ].join(";");

        const card = document.createElement("div");
        card.style.cssText = [
            "background:var(--color-bg, #fff)",
            "color:var(--color-text, #222)",
            "border-radius:12px",
            "padding:28px 32px",
            "min-width:320px",
            "max-width:460px",
            "width:90%",
            "box-shadow:0 8px 32px rgba(0,0,0,0.28)",
            "font-family:inherit"
        ].join(";");

        overlay.appendChild(card);
        document.body.appendChild(overlay);

        function stopScan() {
            if (scanInterval) {
                clearInterval(scanInterval);
                scanInterval = null;
            }
            window.MiOpenApi.postJson("/api/scan/stop", {}).catch(function () {});
        }

        function closeWizard() {
            wizardCancelled = true;
            stopScan();
            if (overlay.parentNode) overlay.parentNode.removeChild(overlay);
        }

        // ------------------------------------------------------------------
        // Step 2 – Confirm detected address and choose a name
        // ------------------------------------------------------------------
        function showStep2(address) {
            detectedAddress = address;
            card.innerHTML = "";

            const title = document.createElement("h3");
            title.textContent = app.i18nText("wizard.step2_title", "Screen detected!");
            title.style.marginTop = "0";

            const addrLine = document.createElement("p");
            addrLine.innerHTML = app.i18nText("wizard.address_label", "Address:") +
                " <code style='font-size:1.1em;font-weight:bold'>" + address + "</code>";

            const nameLabel = document.createElement("label");
            nameLabel.textContent = app.i18nText("wizard.name_label", "Give this screen a name:");
            nameLabel.style.display = "block";
            nameLabel.style.marginTop = "14px";
            nameLabel.style.marginBottom = "6px";

            const nameInput = document.createElement("input");
            nameInput.type = "text";
            nameInput.placeholder = app.i18nText("wizard.name_placeholder", "e.g. Living room left");
            nameInput.style.cssText = [
                "width:100%",
                "box-sizing:border-box",
                "padding:8px 10px",
                "border:1px solid var(--color-border, #ccc)",
                "border-radius:6px",
                "font-size:1em",
                "background:var(--color-input, #f8f8f8)",
                "color:var(--color-text, #222)"
            ].join(";");

            const btnRow = document.createElement("div");
            btnRow.style.cssText = "display:flex;gap:10px;margin-top:20px;justify-content:flex-end";

            const cancelBtn = document.createElement("button");
            cancelBtn.textContent = app.i18nText("button.cancel", "Cancel");
            cancelBtn.className = "btn";
            cancelBtn.addEventListener("click", closeWizard);

            const confirmBtn = document.createElement("button");
            confirmBtn.textContent = app.i18nText("wizard.confirm_pair", "Pair screen");
            confirmBtn.className = "btn open";
            confirmBtn.addEventListener("click", async function () {
                const name = nameInput.value.trim();
                if (!name) {
                    nameInput.style.borderColor = "red";
                    nameInput.focus();
                    return;
                }
                confirmBtn.disabled = true;
                confirmBtn.textContent = app.i18nText("wizard.pairing", "Pairing...");

                try {
                    // 1. Create the virtual device with chosen name
                    await window.MiOpenApi.postJson("/api/command", {
                        command: "new1W " + name
                    });

                    // 2. Fetch device list to find the newly created device by name
                    const devices = await window.MiOpenApi.requestJson("/api/devices");
                    const newDevice = devices.find(function (d) {
                        return d.name === name || d.description === name;
                    });

                    if (!newDevice) {
                        throw new Error(app.i18nText("wizard.error_not_found", "Device not found after creation."));
                    }

                    // 3. Send pair command (add) so the ESP32 pairs with the physical screen
                    await window.MiOpenApi.postJson("/api/command", {
                        deviceId: newDevice.id,
                        command: "add"
                    });

                    app.logStatus(app.i18nText("wizard.success", "Screen paired: ") + name + " (" + address + ")");
                    closeWizard();
                    await fetchAndDisplayDevices(app);
                } catch (err) {
                    app.logStatus(app.i18nText("wizard.error", "Pairing failed: ") + err.message, true);
                    confirmBtn.disabled = false;
                    confirmBtn.textContent = app.i18nText("wizard.confirm_pair", "Pair screen");
                }
            });

            btnRow.appendChild(cancelBtn);
            btnRow.appendChild(confirmBtn);

            card.appendChild(title);
            card.appendChild(addrLine);
            card.appendChild(nameLabel);
            card.appendChild(nameInput);
            card.appendChild(btnRow);

            nameInput.focus();
        }

        // ------------------------------------------------------------------
        // Step 1 – Scanning
        // ------------------------------------------------------------------
        function showStep1() {
            card.innerHTML = "";

            const title = document.createElement("h3");
            title.textContent = app.i18nText("wizard.step1_title", "Add new screen");
            title.style.marginTop = "0";

            const instruction = document.createElement("p");
            instruction.innerHTML = app.i18nText(
                "wizard.step1_instruction",
                "Press the <strong>small button on the back</strong> of your screen's remote to put it in pair mode (the screen will move briefly up/down).<br><br>The address will be detected automatically."
            );

            const statusLine = document.createElement("p");
            statusLine.style.cssText = "font-size:0.9em;color:var(--color-accent,#888);min-height:1.4em";
            statusLine.textContent = app.i18nText("wizard.scanning", "Scanning...");

            // Animated dots
            let dotCount = 0;
            const dotTimer = setInterval(function () {
                if (wizardCancelled) { clearInterval(dotTimer); return; }
                dotCount = (dotCount + 1) % 4;
                statusLine.textContent = app.i18nText("wizard.scanning", "Scanning") +
                    ".".repeat(dotCount);
            }, 500);

            const btnRow = document.createElement("div");
            btnRow.style.cssText = "display:flex;gap:10px;margin-top:20px;justify-content:flex-end";

            const cancelBtn = document.createElement("button");
            cancelBtn.textContent = app.i18nText("button.cancel", "Cancel");
            cancelBtn.className = "btn";
            cancelBtn.addEventListener("click", function () {
                clearInterval(dotTimer);
                closeWizard();
            });

            btnRow.appendChild(cancelBtn);

            card.appendChild(title);
            card.appendChild(instruction);
            card.appendChild(statusLine);
            card.appendChild(btnRow);

            // Start scanning: reset baseline first
            window.MiOpenApi.requestJson("/api/scan?reset=1").then(function () {
                scanInterval = setInterval(function () {
                    if (wizardCancelled) { stopScan(); clearInterval(dotTimer); return; }

                    window.MiOpenApi.requestJson("/api/scan").then(function (data) {
                        if (data.detected && data.address) {
                            clearInterval(dotTimer);
                            stopScan();
                            showStep2(data.address);
                        }
                    }).catch(function () {});
                }, 1500);
            }).catch(function (err) {
                statusLine.textContent = app.i18nText("wizard.scan_error", "Could not start scan: ") + err.message;
                statusLine.style.color = "red";
            });
        }

        showStep1();
    }

    function init(app) {
        app.fetchAndDisplayDevices = function () {
            return fetchAndDisplayDevices(app);
        };
        app.sendCommand = function () {
            return sendCommand(app);
        };
        app.updateDeviceFill = updateDeviceFill;
        app.openAddDevicePopup = function () {
            openPairWizard(app);
        };
        app.openPairWizard = function () {
            openPairWizard(app);
        };
    }

    window.MiOpenDevices = {
        init: init
    };
})();
