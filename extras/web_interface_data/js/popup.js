(function () {
    function createParagraphs(container, items) {
        container.textContent = "";
        items.forEach(function (item) {
            const paragraph = document.createElement("p");
            paragraph.textContent = item;
            container.appendChild(paragraph);
        });
    }

    function closePopup() {
        document.getElementById("popup").classList.remove("open");
    }

    function openPopup(app, title, label, items, content, options) {
        const popup = document.getElementById("popup");
        const labelInput = document.getElementById("label-input");
        const labelTiming = document.getElementById("label-timing");
        const inputTiming = document.getElementById("popup-input-timing");
        const pairBtn = document.getElementById("popup-add");
        const unPairBtn = document.getElementById("popup-remove");
        const deleteBtn = document.getElementById("popup-delete");
        const devicePopupLabel = document.querySelector(".device-popup-label");
        const devicePopup = document.querySelector(".device-popup");
        const pairLabelEl = document.getElementById("pair-label");
        const deleteInfo = document.getElementById("delete-info");
        const confirmBtn = document.getElementById("popup-confirm");
        const cancelBtn = document.getElementById("popup-cancel");
        const input = document.getElementById("popup-input");
        const saveBtn = document.getElementById("popup-save");
        const boolRow = document.getElementById("popup-boolean-row");
        const boolInput = document.getElementById("popup-boolean");
        const boolLabel = document.getElementById("popup-boolean-label");
        const repeatRow = document.getElementById("popup-repeat-row");
        const repeatInput = document.getElementById("popup-repeat");
        const repeatLabel = document.getElementById("popup-repeat-label");
        const contentText = document.getElementById("popup-content");
        const leftContent = document.getElementById("popup-content-left");
        const popupTitle = document.getElementById("popup-title");

        const popupOptions = options || {};
        popupTitle.textContent = title;
        labelInput.textContent = label;
        const protectedMessage = popupOptions.protectedMessage ||
            app.i18nText("popup.active_blocks_destructive", "Disable Active before unpairing or deleting.");

        const showDevicePopup = !!popupOptions.showDevicePopup;
        devicePopupLabel.style.display = showDevicePopup ? "block" : "none";
        devicePopup.style.display = showDevicePopup ? "block" : "none";
        devicePopup.textContent = "";
        if (showDevicePopup) {
            app.state.devicesCache.forEach(function (device) {
                const option = document.createElement("option");
                option.value = device.id;
                option.textContent = device.name;
                devicePopup.appendChild(option);
            });
        }

        const showBoolean = !!popupOptions.showBoolean;
        boolRow.style.display = showBoolean ? "flex" : "none";
        boolInput.checked = !!popupOptions.defaultBoolean;
        boolLabel.textContent = popupOptions.booleanLabel || app.i18nText("popup.boolean_label_default", "solar energy");
        boolInput.onchange = null;

        const showRepeat = !!popupOptions.showRepeatOnNoResponse;
        repeatRow.style.display = showRepeat ? "flex" : "none";
        repeatInput.checked = !!popupOptions.defaultRepeatOnNoResponse;
        repeatLabel.textContent = popupOptions.repeatOnNoResponseLabel ||
            app.i18nText("popup.repeat_on_no_response", "Repeat command if shutter does not respond");

        const showInput = !!popupOptions.showInput;
        input.style.display = showInput ? "block" : "none";
        labelInput.style.display = showInput ? "block" : "none";
        input.value = popupOptions.defaultValue || "";

        const showSave = !!popupOptions.showSave;
        saveBtn.style.display = showSave ? "block" : "none";

        const showTiming = !!popupOptions.showTiming;
        labelTiming.style.display = showTiming ? "block" : "none";
        inputTiming.style.display = showTiming ? "block" : "none";
        labelTiming.textContent = popupOptions.timingLabel || "timing:";
        inputTiming.value = showTiming ? (popupOptions.defaultTiming || "") : "";

        deleteBtn.style.display = popupOptions.btnShowDelete ? "block" : "none";
        cancelBtn.style.display = popupOptions.btnShowCancel ? "block" : "none";
        confirmBtn.style.display = "none";
        deleteInfo.style.display = "none";

        createParagraphs(contentText, items || []);
        createParagraphs(leftContent, content || []);

        if (popupOptions.pairLabel && (popupOptions.onPair || popupOptions.onUnpair)) {
            pairLabelEl.style.display = "block";
            pairLabelEl.textContent = popupOptions.pairLabel;
        } else {
            pairLabelEl.style.display = "none";
        }

        const destructiveActionsBlocked = function () {
            return !!popupOptions.blockDestructiveWhenBoolean && showBoolean && boolInput.checked;
        };
        const showProtectedMessage = function () {
            deleteInfo.style.display = "block";
            deleteInfo.textContent = protectedMessage;
        };
        const updateDestructiveButtons = function () {
            const blocked = destructiveActionsBlocked();
            unPairBtn.disabled = blocked;
            deleteBtn.disabled = blocked;
            unPairBtn.title = blocked ? protectedMessage : "";
            deleteBtn.title = blocked ? protectedMessage : "";
            if (blocked) {
                showProtectedMessage();
            } else if (deleteInfo.textContent === protectedMessage) {
                deleteInfo.style.display = "none";
                deleteInfo.textContent = "";
            }
        };
        if (showBoolean && popupOptions.blockDestructiveWhenBoolean) {
            boolInput.onchange = updateDestructiveButtons;
        }

        if (popupOptions.onPair) {
            pairBtn.style.display = "block";
            pairBtn.textContent = popupOptions.pairBtnName || "Pair";
            pairBtn.onclick = function () {
                const selectedDevice = showDevicePopup ? devicePopup.value : undefined;
                closePopup();
                popupOptions.onPair(selectedDevice);
            };
        } else {
            pairBtn.style.display = "none";
            pairBtn.onclick = null;
        }

        if (popupOptions.onUnpair) {
            unPairBtn.style.display = "block";
            unPairBtn.textContent = popupOptions.unpairBtnName || "Unpair";
            unPairBtn.onclick = function () {
                if (destructiveActionsBlocked()) {
                    showProtectedMessage();
                    return;
                }
                const selectedDevice = showDevicePopup ? devicePopup.value : undefined;
                closePopup();
                popupOptions.onUnpair(selectedDevice);
            };
        } else {
            unPairBtn.style.display = "none";
            unPairBtn.onclick = null;
        }

        if (popupOptions.onDelete) {
            const exitDeleteMode = function () {
                deleteInfo.style.display = "none";
                cancelBtn.style.display = "none";
                deleteBtn.style.display = "block";
                confirmBtn.style.display = "none";
                pairBtn.style.display = popupOptions.onPair ? "block" : "none";
                unPairBtn.style.display = popupOptions.onUnpair ? "block" : "none";
                updateDestructiveButtons();
            };

            deleteBtn.onclick = function () {
                if (destructiveActionsBlocked()) {
                    showProtectedMessage();
                    return;
                }
                pairBtn.style.display = "none";
                unPairBtn.style.display = "none";
                deleteBtn.style.display = "none";
                deleteInfo.style.display = "block";
                deleteInfo.textContent = popupOptions.deleteInfo ||
                    app.i18nText("popup.unpair_before_delete", "Unpair devices first before deleting.");
                confirmBtn.style.display = "inline-block";
                confirmBtn.textContent = "Confirm";
                confirmBtn.onclick = async function () {
                    if (destructiveActionsBlocked()) {
                        showProtectedMessage();
                        exitDeleteMode();
                        return;
                    }
                    try {
                        await popupOptions.onDelete();
                        closePopup();
                    } catch (error) {
                        app.logStatus("Error deleting: " + error.message, true);
                        exitDeleteMode();
                    }
                };
                cancelBtn.style.display = "inline-block";
                cancelBtn.onclick = exitDeleteMode;
            };

            exitDeleteMode();
        } else {
            deleteBtn.onclick = null;
        }

        updateDestructiveButtons();

        saveBtn.onclick = function () {
            const value = showInput ? input.value : true;
            const timingValue = showTiming ? inputTiming.value : undefined;
            const deviceValue = showDevicePopup ? devicePopup.value : undefined;
            const repeatValue = showRepeat ? repeatInput.checked : undefined;
            closePopup();
            if (popupOptions.onSave) {
                popupOptions.onSave(value, timingValue, deviceValue, repeatValue);
            }
        };

        popup.classList.add("open");
    }

    function init(app) {
        app.openPopup = function (title, label, items, content, options) {
            openPopup(app, title, label, items, content, options);
        };
        app.closePopup = closePopup;
        window.openPopup = app.openPopup;
        window.closePopup = closePopup;
    }

    window.MiOpenPopup = {
        init: init
    };
})();
