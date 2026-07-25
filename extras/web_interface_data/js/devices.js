(function () {
    async function runAction(app, deviceId, action) {
        const result = await window.MiOpenApi.postJson("/api/action", {
            deviceId: deviceId,
            action: action
        });
        app.logStatus(result.message || ("Action " + action + " sent."));
    }

    async function setPosition(app, deviceId, percent) {
        const result = await window.MiOpenApi.postJson("/api/action", {
            deviceId: deviceId,
            action: "position",
            position: percent
        });
        app.logStatus(result.message || ("Position " + percent + "% sent."));
    }

    function updateDeviceFill(deviceId, percent) {
        const deviceEl = document.querySelector('.device[data-id="' + deviceId + '"]');
        if (!deviceEl) {
            return;
        }
        deviceEl.style.background = "linear-gradient(to top, var(--color-input) " +
            percent + "%, var(--color-accent3) " + percent + "%)";

        const slider = deviceEl.querySelector('.position-slider');
        if (slider && Math.abs(parseInt(slider.value) - percent) > 1) {
            slider.value = percent;
        }
        const label = deviceEl.querySelector('.position-label');
        if (label) {
            label.textContent = Math.round(percent) + "%";
        }
    }

    function createDeviceButton(label, className, onClick) {
        const button = document.createElement("button");
        button.textContent = label;
        button.classList.add("btn", className);
        button.addEventListener("click", onClick);
        return button;
    }

    function createPositionSlider(app, device) {
        const wrapper = document.createElement("div");
        wrapper.style.cssText = "display:flex;align-items:center;gap:6px;margin-top:6px;width:100%";

        const slider = document.createElement("input");
        slider.type = "range";
        slider.min = "0";
        slider.max = "100";
        slider.value = String(Math.round(device.position || 0));
        slider.className = "position-slider";
        slider.style.cssText = "flex:1;cursor:pointer";
        slider.title = app.i18nText("slider.position_title", "Drag to set position");

        const label = document.createElement("span");
        label.className = "position-label";
        label.style.cssText = "min-width:36px;text-align:right;font-size:0.85em";
        label.textContent = Math.round(device.position || 0) + "%";

        slider.addEventListener("input", function () {
            label.textContent = slider.value + "%";
        });

        slider.addEventListener("change", function () {
            const val = parseInt(slider.value, 10);
            label.textContent = val + "%";
            setPosition(app, device.id, val).catch(function (err) {
                app.logStatus(err.message, true);
            });
        });

        wrapper.appendChild(slider);
        wrapper.appendChild(label);
        return wrapper;
    }

    function createTravelTimeLabel(app, device) {
        const wrapper = document.createElement("div");
        wrapper.className = "travel-time-label";
        wrapper.style.cssText = "display:flex;align-items:center;gap:4px;margin-top:4px;font-size:0.78em;color:var(--color-accent,#888);width:100%";

        const icon = document.createElement("span");
        icon.textContent = "\u23F1";
        icon.setAttribute("aria-hidden", "true");

        const text = document.createElement("span");
        text.dataset.travelTimeId = device.id;
        text.textContent = app.i18nText("label.travel_time", "Travel time:") + " " +
            (device.travel_time || 10) + " " + app.i18nText("label.seconds", "s");
        text.title = app.i18nText("label.travel_time_hint", "Time for the screen to travel from fully open to fully closed. Edit to change.");

        wrapper.appendChild(icon);
        wrapper.appendChild(text);
        return wrapper;
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

                listItem.appendChild(createPositionSlider(app, device));
                listItem.appendChild(createTravelTimeLabel(app, device));

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
                                        const ttText = document.querySelector('[data-travel-time-id="' + device.id + '"]');
                                        if (ttText) {
                                            ttText.textContent = app.i18nText("label.travel_time", "Travel time:") + " " +
                                                parsedTiming + " " + app.i18nText("label.seconds", "s");
                                        }
                                        device.travel_time = parsedTiming;
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

    var SCAN_TIMEOUT_MS = 60000;

    function openPairWizard(app) {
        let scanInterval = null;
        let scanTimeoutTimer = null;
        let dotTimer = null;
        let detectedAddress = null;
        let wizardCancelled = false;

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
            if (dotTimer) { clearInterval(dotTimer); dotTimer = null; }
            if (scanInterval) { clearInterval(scanInterval); scanInterval = null; }
            if (scanTimeoutTimer) { clearTimeout(scanTimeoutTimer); scanTimeoutTimer = null; }
            window.MiOpenApi.postJson("/api/scan/stop", {}).catch(function () {});
        }

        function closeWizard() {
            wizardCancelled = true;
            stopScan();
            if (overlay.parentNode) overlay.parentNode.removeChild(overlay);
        }

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
                    await window.MiOpenApi.postJson("/api/command", {
                        command: detectedAddress
                            ? "new1W " + detectedAddress + " " + name
                            : "new1W " + name
                    });

                    const devices = await window.MiOpenApi.requestJson("/api/devices");
                    const newDevice = devices.find(function (d) {
                        return d.name === name || d.description === name;
                    });

                    if (!newDevice) {
                        throw new Error(app.i18nText("wizard.error_not_found", "Device not found after creation."));
                    }

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

        function showStep1(useMotorButton) {
            card.innerHTML = "";

            const title = document.createElement("h3");
            title.textContent = app.i18nText("wizard.step1_title", "Add new screen");
            title.style.marginTop = "0";

            const instruction = document.createElement("p");
            if (useMotorButton) {
                instruction.innerHTML = app.i18nText(
                    "wizard.step1_instruction_motor",
                    "Press and hold the <strong>Up ▲ and Down ▼ buttons simultaneously</strong> on the motor until the screen moves briefly up/down.<br><br>The address will be detected automatically."
                );
            } else {
                instruction.innerHTML = app.i18nText(
                    "wizard.step1_instruction",
                    "Press the <strong>small button on the back</strong> of your screen's remote to put it in pair mode (the screen will move briefly up/down).<br><br>The address will be detected automatically."
                );
            }

            const statusLine = document.createElement("p");
            statusLine.style.cssText = "font-size:0.9em;color:var(--color-accent,#888);min-height:1.4em";
            statusLine.textContent = app.i18nText("wizard.scanning", "Scanning...");

            let dotCount = 0;
            dotTimer = setInterval(function () {
                if (wizardCancelled) { clearInterval(dotTimer); dotTimer = null; return; }
                dotCount = (dotCount + 1) % 4;
                statusLine.textContent = app.i18nText("wizard.scanning", "Scanning") +
                    ".".repeat(dotCount);
            }, 500);

            const btnRow = document.createElement("div");
            btnRow.style.cssText = "display:flex;gap:10px;margin-top:20px;justify-content:flex-end";

            const cancelBtn = document.createElement("button");
            cancelBtn.textContent = app.i18nText("button.cancel", "Cancel");
            cancelBtn.className = "btn";
            cancelBtn.addEventListener("click", function () { closeWizard(); });

            btnRow.appendChild(cancelBtn);

            card.appendChild(title);
            card.appendChild(instruction);
            card.appendChild(statusLine);
            card.appendChild(btnRow);

            window.MiOpenApi.requestJson("/api/scan?reset=1").then(function () {
                scanTimeoutTimer = setTimeout(function () {
                    if (wizardCancelled) { return; }
                    stopScan();
                    statusLine.style.color = "red";
                    statusLine.textContent = useMotorButton
                        ? app.i18nText("wizard.scan_timeout_motor", "No screen detected. Hold Up + Down on the motor until it moves briefly and try again.")
                        : app.i18nText("wizard.scan_timeout", "No screen detected. Press the button on the back of the remote and try again.");
                    cancelBtn.textContent = app.i18nText("button.close", "Close");
                }, SCAN_TIMEOUT_MS);

                scanInterval = setInterval(function () {
                    if (wizardCancelled) { stopScan(); return; }
                    window.MiOpenApi.requestJson("/api/scan").then(function (data) {
                        if (data.detected && data.address) {
                            stopScan();
                            showStep2(data.address);
                        }
                    }).catch(function () {});
                }, 1500);
            }).catch(function (err) {
                stopScan();
                statusLine.textContent = app.i18nText("wizard.scan_error", "Could not start scan: ") + err.message;
                statusLine.style.color = "red";
            });
        }

        // Step 0: choose pairing method
        function showStep0() {
            card.innerHTML = "";

            const title = document.createElement("h3");
            title.textContent = app.i18nText("wizard.step0_title", "How do you want to start pairing?");
            title.style.marginTop = "0";

            const subtitle = document.createElement("p");
            subtitle.style.cssText = "margin-bottom:18px;font-size:0.95em;color:var(--color-accent,#888)";
            subtitle.textContent = app.i18nText("wizard.step0_subtitle", "Choose the method that applies to your screen.");

            function makeMethodCard(emoji, labelKey, labelDefault, descKey, descDefault, onClick) {
                const btn = document.createElement("button");
                btn.style.cssText = [
                    "display:flex",
                    "align-items:flex-start",
                    "gap:14px",
                    "width:100%",
                    "background:var(--color-card,var(--color-input,#f5f5f5))",
                    "border:2px solid var(--color-border,#ddd)",
                    "border-radius:10px",
                    "padding:14px 16px",
                    "margin-bottom:12px",
                    "cursor:pointer",
                    "text-align:left",
                    "font-family:inherit",
                    "color:var(--color-text,#222)",
                    "transition:border-color 0.15s"
                ].join(";");
                btn.addEventListener("mouseover", function () { btn.style.borderColor = "var(--color-primary,#0078d4)"; });
                btn.addEventListener("mouseout",  function () { btn.style.borderColor = "var(--color-border,#ddd)"; });

                const emojiEl = document.createElement("span");
                emojiEl.textContent = emoji;
                emojiEl.style.cssText = "font-size:1.8em;line-height:1;flex-shrink:0";

                const textWrap = document.createElement("div");
                const labelEl = document.createElement("strong");
                labelEl.textContent = app.i18nText(labelKey, labelDefault);
                const descEl = document.createElement("p");
                descEl.style.cssText = "margin:4px 0 0;font-size:0.85em;color:var(--color-accent,#888)";
                descEl.textContent = app.i18nText(descKey, descDefault);
                textWrap.appendChild(labelEl);
                textWrap.appendChild(descEl);

                btn.appendChild(emojiEl);
                btn.appendChild(textWrap);
                btn.addEventListener("click", onClick);
                return btn;
            }

            const remoteBtn = makeMethodCard(
                "\uD83D\uDCF9",
                "wizard.method_remote_label",
                "I have a remote control",
                "wizard.method_remote_desc",
                "Press the small PROG button on the back of the remote. The screen will move briefly.",
                function () { showStep1(false); }
            );

            const motorBtn = makeMethodCard(
                "\uD83D\uDD27",
                "wizard.method_motor_label",
                "I use the motor buttons (no remote)",
                "wizard.method_motor_desc",
                "Hold Up \u25b2 and Down \u25bc simultaneously on the motor until the screen moves briefly (e.g. Somfy Altea io, Oximo io).",
                function () { showStep1(true); }
            );

            const cancelBtn = document.createElement("button");
            cancelBtn.textContent = app.i18nText("button.cancel", "Cancel");
            cancelBtn.className = "btn";
            cancelBtn.style.cssText = "margin-top:4px;width:100%";
            cancelBtn.addEventListener("click", closeWizard);

            card.appendChild(title);
            card.appendChild(subtitle);
            card.appendChild(remoteBtn);
            card.appendChild(motorBtn);
            card.appendChild(cancelBtn);
        }

        showStep0();
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
