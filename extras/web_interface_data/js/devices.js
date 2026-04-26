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
                            protectedMessage: app.i18nText(
                                "popup.active_blocks_destructive",
                                "Disable Active before unpairing or deleting."
                            ),
                            pairLabel: app.i18nText("popup.pair_label_device", "Add / Remove the device to the physical screen"),
                            deleteInfo: app.i18nText("popup.delete_device_info", "Only use when the device is not linked to a physical screen."),
                            onSave: async function (newName, newTiming) {
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

    function openAddDevicePopup(app) {
        app.openPopup(
            app.i18nText("popup.add_device_title", "Add Device"),
            app.i18nText("popup.new_device", "new device"),
            [app.i18nText("popup.here_add_device", "here add your device")],
            [""],
            {
                showSave: true,
                showInput: true,
                btnShowDelete: false,
                btnShowCancel: false,
                onSave: async function (newName) {
                    if (!newName.trim()) {
                        return;
                    }

                    try {
                        const result = await window.MiOpenApi.postJson("/api/command", {
                            command: "new1W " + newName
                        });
                        app.logStatus(result.message || "Device added.");
                        await fetchAndDisplayDevices(app);
                    } catch (error) {
                        app.logStatus("Error adding device: " + error.message, true);
                    }
                }
            }
        );
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
            openAddDevicePopup(app);
        };
    }

    window.MiOpenDevices = {
        init: init
    };
})();
