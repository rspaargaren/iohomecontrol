document.addEventListener('DOMContentLoaded', function() {
    const deviceListUL = document.getElementById('device-list');
    const deviceSelect = document.getElementById('device-select');
    let devicesCache = [];
    const remoteListUL = document.getElementById('remote-list');
    const commandInput = document.getElementById('command-input');
    const sendCommandButton = document.getElementById('send-command-button');
    const statusMessagesDiv = document.getElementById('status-messages');
    const MAX_LOGS = 20; // maximaal aantal logs
    const mqttUserInput = document.getElementById('mqtt-user');
    const mqttServerInput = document.getElementById('mqtt-server');
    const mqttPasswordInput = document.getElementById('mqtt-password');
    const mqttDiscoveryInput = document.getElementById('mqtt-discovery');
    const mqttUpdateButton = document.getElementById('mqtt-update');
    const firmwareFileInput = document.getElementById('firmware-file');
    const filesystemFileInput = document.getElementById('filesystem-file');
    const firmwareUploadButton = document.getElementById('upload-firmware');
    const filesystemUploadButton = document.getElementById('upload-filesystem');
    const devicesFileInput = document.getElementById('devices-file');
    const remotesFileInput = document.getElementById('remotes-file');
    const devicesUploadButton = document.getElementById('upload-devices');
    const remotesUploadButton = document.getElementById('upload-remotes');
    const downloadDevicesButton = document.getElementById('download-devices');
    const downloadRemotesButton = document.getElementById('download-remotes');
    const lastAddrInput = document.getElementById('last-address');
    const ws = new WebSocket(`ws://${window.location.host}/ws`);

    // Function to add a message to the status/log
    function logStatus(message, isError = false) {
        const logEntry = document.createElement('p');
        logEntry.textContent = message;
        if (isError) {
            logEntry.style.color = 'red';
        }
        statusMessagesDiv.appendChild(logEntry);
        // Scroll to the bottom of the status messages
        statusMessagesDiv.scrollTop = statusMessagesDiv.scrollHeight;
        while (statusMessagesDiv.children.length > MAX_LOGS) {
            statusMessagesDiv.removeChild(statusMessagesDiv.firstChild);
        }
        
    }
    logStatus('System started');
    logStatus('Loading devices...');

    ws.onmessage = (event) => {
        const data = JSON.parse(event.data);
        if (data.type === 'log') {
            logStatus(data.message);
        } else if (data.type === 'position') {
            updateDeviceFill(data.id, data.position);
        } else if (data.type === 'init') {
            data.logs.forEach(log => logStatus(log));
            fetchAndDisplayDevices();
        } else if (data.type === 'lastaddr') {
            lastAddrInput.value = data.address || '';
        }
    };
    ws.onopen = () => logStatus('WebSocket connected');
    ws.onclose = () => logStatus('WebSocket disconnected', true);

    async function loadLastAddress() {
        try {
            const resp = await fetch('/api/lastaddr');
            if (!resp.ok) return;
            const data = await resp.json();
            lastAddrInput.value = data.address || '';
        } catch (e) {
            console.error('Error fetching last address', e);
        }
    }
    // load MQTT config
    async function loadMqttConfig() {
        try {
            const resp = await fetch('/api/mqtt');
            if (!resp.ok) return;
            const cfg = await resp.json();
            mqttUserInput.value = cfg.user || '';
            mqttServerInput.value = cfg.server || '';
            mqttPasswordInput.value = cfg.password || '';
            mqttDiscoveryInput.value = cfg.discovery || '';
        } catch (e) {
            console.error('Error fetching MQTT config', e);
        }
    }
    // update MQTT config
    async function updateMqttConfig() {
        const payload = {
            user: mqttUserInput.value,
            server: mqttServerInput.value,
            password: mqttPasswordInput.value,
            discovery: mqttDiscoveryInput.value
        };
        try {
            const resp = await fetch('/api/mqtt', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload)
            });
            const result = await resp.json();
            logStatus(result.message || 'MQTT settings updated.');
        } catch (e) {
            console.error('Error updating MQTT config', e);
            logStatus('Error updating MQTT config', true);
        }
    }
    // upload firmware
    async function uploadFirmware() {
        const file = firmwareFileInput.files[0];
        if (!file) {
            logStatus('No firmware file selected', true);
            return;
        }
        const formData = new FormData();
        formData.append('file', file);
        try {
            const resp = await fetch('/api/firmware', { method: 'POST', body: formData });
            const result = await resp.json();
            logStatus(result.message || 'Firmware uploaded');
        } catch (e) {
            console.error('Error uploading firmware', e);
            logStatus('Error uploading firmware', true);
        }
    }
    // upload filesystem
    async function uploadFilesystem() {
        const file = filesystemFileInput.files[0];
        if (!file) {
            logStatus('No filesystem file selected', true);
            return;
        }
        const formData = new FormData();
        formData.append('file', file);
        try {
            const resp = await fetch('/api/filesystem', { method: 'POST', body: formData });
            const result = await resp.json();
            logStatus(result.message || 'Filesystem uploaded');
        } catch (e) {
            console.error('Error uploading filesystem', e);
            logStatus('Error uploading filesystem', true);
        }
    }
    // upload devices
    async function uploadDevices() {
        const file = devicesFileInput.files[0];
        if (!file) {
            logStatus('No devices file selected', true);
            return;
        }
        const formData = new FormData();
        formData.append('file', file);
        try {
            const resp = await fetch('/api/upload/devices', { method: 'POST', body: formData });
            const result = await resp.json();
            logStatus(result.message || 'Devices file uploaded');
            fetchAndDisplayDevices();
            fetchAndDisplayRemotes();
        } catch (e) {
            console.error('Error uploading devices file', e);
            logStatus('Error uploading devices file', true);
        }
    }
    // upload remotes
    async function uploadRemotes() {
        const file = remotesFileInput.files[0];
        if (!file) {
            logStatus('No remotes file selected', true);
            return;
        }
        const formData = new FormData();
        formData.append('file', file);
        try {
            const resp = await fetch('/api/upload/remotes', { method: 'POST', body: formData });
            const result = await resp.json();
            logStatus(result.message || 'Remotes file uploaded');
            fetchAndDisplayRemotes();
        } catch (e) {
            console.error('Error uploading remotes file', e);
            logStatus('Error uploading remotes file', true);
        }
    }
    // download remotes
    function downloadFile(url, filename) {
        fetch(url)
            .then(resp => {
                if (!resp.ok) throw new Error('Network response was not ok');
                return resp.blob();
            })
            .then(blob => {
                const link = document.createElement('a');
                link.href = window.URL.createObjectURL(blob);
                link.download = filename;
                link.click();
                window.URL.revokeObjectURL(link.href);
            })
            .catch(err => {
                console.error('Error downloading file', err);
                logStatus('Error downloading file', true);
            });
    }

    // Fetch and display remotes
   async function fetchAndDisplayRemotes() {
        try {
            const response = await fetch('/api/remotes');
            if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
            const remotes = await response.json();

            const tbody = document.querySelector('#remote-table tbody');
            tbody.textContent = ''; // leeg

            if (!remotes.length) {
            const tr = document.createElement('tr');
            tr.innerHTML = `<td colspan="4">No remotes available.</td>`;
            tbody.appendChild(tr);
            return;
            }

            const frag = document.createDocumentFragment();
            remotes.forEach(remote => {
            const tr = document.createElement('tr'); // <-- nieuw tr per item

            const linkedDevices = (remote.devices && remote.devices.length)
                ? remote.devices.map(d => {
                    const dev = (devicesCache || []).find(v => v.id === d || v.name === d || v.description === d);
                    return dev ? dev.name : d;
                }).join(', ')
                : '0 devices';

            tr.innerHTML = `
                <td><div class="remote-id">${remote.id}</div></td>
                <td>${remote.name}</td>
                <td>${linkedDevices}</td>
                <td>
                <button class="btn edit bg" onclick="editRemote('${remote.id}','${remote.name}')">Edit</button>
                </td>
            `;
            frag.appendChild(tr);
            });
            tbody.appendChild(frag);
        } catch (error) {
            console.error('Error fetching remotes:', error);
        }
    }

    
    // Function to fetch devices and populate the lists
    async function fetchAndDisplayDevices() {
        try {
            const response = await fetch('/api/devices');
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            const devices = await response.json();
            devicesCache = devices;

            deviceListUL.innerHTML = ''; // Clear existing list
            deviceSelect.innerHTML = ''; // Clear existing options

            if (devices.length === 0) {
                logStatus('No devices found.');
                const listItem = document.createElement('li');
                listItem.textContent = 'No devices available.';
                deviceListUL.appendChild(listItem);
                return;
            }

            devices.forEach(device => {
                // Populate the UL for display (simple version)
                const nameSpan = document.createElement('span');
                nameSpan.textContent = device.name;
                const listItem = document.createElement('li');
                listItem.textContent = ' ';
                listItem.classList.add('device');
                listItem.dataset.id = device.id;
                listItem.appendChild(nameSpan); // Append the name span to the list item

                // 🔘 up btn
                const upButton = document.createElement('button');
                upButton.textContent = 'up';
                upButton.classList.add('btn', 'open');
                upButton.onclick = () => {
                    fetch('/api/action', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ deviceId: device.id, action: 'open' })
                    }).then(r => r.json()).then(j => logStatus(j.message));
                };
                 // 🔘 stop btn
                const stopButton = document.createElement('button');
                stopButton.textContent = 'stop';
                stopButton.classList.add('btn', 'stop');
                stopButton.onclick = () => {
                    fetch('/api/action', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ deviceId: device.id, action: 'stop' })
                    }).then(r => r.json()).then(j => logStatus(j.message));
                };
                // 🔘 down btn
                const downButton = document.createElement('button');
                downButton.textContent = 'down';
                downButton.classList.add('btn', 'down');
                downButton.onclick = () => {
                    fetch('/api/action', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ deviceId: device.id, action: 'close' })
                    }).then(r => r.json()).then(j => logStatus(j.message));
                };
                const editButton = document.createElement('button');
                editButton.textContent = 'edit';
                editButton.classList.add('btn', 'edit');
                editButton.onclick = () =>
                    openPopup('Edit Device', "Adjust the name:",
                        [
                            'ID: ' + device.id,
                            'Description: ' + (device.description || ''),
                            'Position: ' + device.position + '%',
                            'Paired: ' + (device.paired ? 'Yes' : 'No'),
                        ], [""], {
                        showSave: true,
                        showInput: true,
                        showTiming: true,
                        btnShowDelete: true,
                        defaultValue: device.name,
                        defaultTiming: device.travel_time,
                        showBoolean: true,
                        booleanLabel: 'Active',
                        defaultBoolean: device.active,  // bv. uit je data
                        pairLabel: 'Add / Remove the device to the physical screen',
                        deleteInfo: 'Only use when the device is not linked to a physical screen.',
                        onSave: async (newName, newTiming) => {
                            try {
                                if (newName.trim() && newName !== device.name) {
                                    const response = await fetch('/api/command', {
                                        method: 'POST',
                                        headers: { 'Content-Type': 'application/json' },
                                        body: JSON.stringify({ deviceId: device.id, command: `edit1W ${newName}` })
                                    });
                                    const result = await response.json();
                                    if (result.success) {
                                        logStatus(result.message || 'Device renamed.');
                                    } else {
                                        logStatus(result.message || 'Failed to rename device.', true);
                                    }
                                }
                                const parsed = parseInt(newTiming, 10);
                                if (!isNaN(parsed) && parsed > 0 && parsed !== device.travel_time) {
                                    const tResp = await fetch('/api/command', {
                                        method: 'POST',
                                        headers: { 'Content-Type': 'application/json' },
                                        body: JSON.stringify({ deviceId: device.id, command: `time1W ${parsed}` })
                                    });
                                    const tResult = await tResp.json();
                                    if (tResult.success) {
                                        logStatus(tResult.message || 'Travel time updated.');
                                    } else {
                                        logStatus(tResult.message || 'Failed to update travel time.', true);
                                    }
                                }
                                fetchAndDisplayDevices();
                            } catch (e) {
                                logStatus(`Error updating device: ${e.message}`, true);
                            }
                        },
                        onPair: async () => {
                            try {
                                const response = await fetch('/api/command', {
                                    method: 'POST',
                                    headers: { 'Content-Type': 'application/json' },
                                    body: JSON.stringify({ deviceId: device.id, command: 'add' })
                                });
                                const result = await response.json();
                                if (result.success) {
                                    logStatus(result.message || 'Device added.');
                                } else {
                                    logStatus(result.message || 'Failed to add device.', true);
                                }
                            } catch (e) {
                                logStatus(`Error adding device: ${e.message}`, true);
                            }
                        },
                        onUnpair: async () => {
                            try {
                                const response = await fetch('/api/command', {
                                    method: 'POST',
                                    headers: { 'Content-Type': 'application/json' },
                                    body: JSON.stringify({ deviceId: device.id, command: 'remove' })
                                });
                                const result = await response.json();
                                if (result.success) {
                                    logStatus(result.message || 'Device unpaired.');
                                } else {
                                    logStatus(result.message || 'Failed to unpair device.', true);
                                }
                            } catch (e) {
                                logStatus(`Error unpairing device: ${e.message}`, true);
                            }
                        },
                       onDelete: async () => {
                            const resp = await fetch('/api/command', {
                                method: 'POST',
                                headers: { 'Content-Type': 'application/json' },
                                body: JSON.stringify({ deviceId: device.id, command: 'del1W' })
                            });
                            const res = await resp.json();
                            if (!resp.ok || res.success === false) throw new Error(res.message || 'Delete failed');

                            logStatus(res.message || 'Device deleted.');
                            await fetchAndDisplayDevices();
                            }
                    });

                listItem.appendChild(upButton);
                listItem.appendChild(stopButton);
                listItem.appendChild(downButton);
                listItem.appendChild(editButton);
                // TODO: Add buttons for simple actions if desired in future
                deviceListUL.appendChild(listItem);

                // Apply initial position now that the element exists in the DOM
                updateDeviceFill(device.id, device.position || 0);

                // Populate the SELECT for command sending
                const option = document.createElement('option');
                option.value = device.id; // Assuming device object has an 'id'
                option.textContent = device.name;
                deviceSelect.appendChild(option);
            });
            logStatus('Device list updated.');

        } catch (error) {
            logStatus(`Error fetching devices: ${error.message}`, true);
            console.error('Error fetching devices:', error);
        }
    }

    // Function to send a command
    async function sendCommand() {
        const selectedDeviceId = deviceSelect.value;
        const commandStr = commandInput.value.trim();

        if (!selectedDeviceId) {
            logStatus('Please select a device.', true);
            return;
        }
        if (!commandStr) {
            logStatus('Please enter a command.', true);
            return;
        }

        logStatus(`Sending command "${commandStr}" to device ID ${selectedDeviceId}...`);

        try {
            const response = await fetch('/api/command', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ deviceId: selectedDeviceId, command: commandStr }),
            });

            const result = await response.json(); // Expecting JSON response e.g. { success: true, message: "..."}

            if (result.success) {
                logStatus(`Command success: ${result.message || 'Command processed.'}`);
            } else {
                logStatus(`Command failed: ${result.message || 'Unknown error.'}`, true);
            }
        } catch (error) {
            logStatus(`Error sending command: ${error.message}`, true);
            console.error('Error sending command:', error);
        }
    }
  
    // Event Listeners
    if (sendCommandButton) {
        sendCommandButton.addEventListener('click', sendCommand);
    }
    if (mqttUpdateButton) {
        mqttUpdateButton.addEventListener('click', updateMqttConfig);
    }
    if (firmwareUploadButton) {
        firmwareUploadButton.addEventListener('click', uploadFirmware);
    }
    if (filesystemUploadButton) {
        filesystemUploadButton.addEventListener('click', uploadFilesystem);
    }
    if (devicesUploadButton) {
        devicesUploadButton.addEventListener('click', uploadDevices);
    }
    if (remotesUploadButton) {
        remotesUploadButton.addEventListener('click', uploadRemotes);
    }
    if (downloadDevicesButton) {
        downloadDevicesButton.addEventListener('click', () => downloadFile('/api/download/devices', '1W.json'));
    }
    if (downloadRemotesButton) {
        downloadRemotesButton.addEventListener('click', () => downloadFile('/api/download/remotes', 'RemoteMap.json'));
    }

    // suggestions for command input
    const suggestions = ["add", "remove", "close", "open", "ls", "cat"];
    const input = document.getElementById('command-input');
    const suggestionBox = document.getElementById('suggestions');

    suggestions.forEach(item => {
      const div = document.createElement('option');
      div.className = 'suggestion';
      div.textContent = item;

      div.addEventListener('click', () => {
        if (input.value !== '' && !input.value.endsWith(' ')) {
          input.value += ' ';
        }
        input.value += item + ' ';
        input.focus();
      });

      suggestionBox.appendChild(div);
    });
    //dark mode toggle


    const toggleBtn = document.getElementById('toggle-theme');

    const body = document.body;

    toggleBtn.addEventListener('click', () => {
      body.classList.toggle('dark-mode');

    // Optional: save to localStorage
      if (body.classList.contains('dark-mode')) {
        localStorage.setItem('theme', 'dark');
      } else {
        localStorage.setItem('theme', 'light');
      }
    });

    function updateDeviceFill(deviceId, percent) {
        const deviceEl = document.querySelector(`.device[data-id="${deviceId}"]`);
        if (!deviceEl) return;

        // calculate color based on percentage (green→red)
        const color = `var(--color-accent3)`;
        const fillPercent = 100 - percent;
        
        // set background as gradient
        deviceEl.style.background = `linear-gradient(to top, var(--color-input) ${percent}%, ${color} ${percent}%)`;
    }

    // Device positions are updated via WebSocket

    // popup open
    function openPopup(title, label, items = [], Content = [], options = {}) {
        const labelInput = document.getElementById('label-input');
        const labelTiming = document.getElementById('label-timing');
        const inputTiming = document.getElementById('popup-input-timing');
        const pairBtn = document.getElementById('popup-add');
        const unPairBtn = document.getElementById('popup-remove');
        const deleteBtn = document.getElementById('popup-delete');
        const devicePopupLabel = document.querySelector('.device-popup-label');
        const devicePopup = document.querySelector('.device-popup');
        const pairLabelEl = document.getElementById('pair-label');
        const deleteInfo = document.getElementById('delete-info');
        const confirmBtn = document.getElementById('popup-confirm');
        const cancelBtn = document.getElementById('popup-cancel');
        const input = document.getElementById('popup-input');
        const saveBtn = document.getElementById('popup-save');
        const boolRow = document.getElementById('popup-boolean-row');
        const boolInput = document.getElementById('popup-boolean');
        const boolLabel = document.getElementById('popup-boolean-label');
        const contentText = document.getElementById('popup-content');
        const leftContent = document.getElementById('popup-content-left');

        document.getElementById('popup-title').textContent = title;
        labelInput.textContent = label;
        const showDevicePopup = options && options.showDevicePopup;
        devicePopupLabel.style.display = showDevicePopup ? 'block' : 'none';
        devicePopup.style.display = showDevicePopup ? 'block' : 'none';
        if (showDevicePopup) {
            devicePopup.innerHTML = '';
            devicesCache.forEach(d => {
                const opt = document.createElement('option');
                opt.value = d.id;
                opt.textContent = d.name;
                devicePopup.appendChild(opt);
            });
        }
        const showBoolean = !!options.showBoolean;
        boolRow.style.display = showBoolean ? 'flex' : 'none';
        if (showBoolean) {
            boolInput.checked = !!options.defaultBoolean;
        }
        const showInput = options && options.showInput;
        input.style.display = showInput ? 'block' : 'none';
        labelInput.style.display = showInput ? 'block' : 'none';
        input.value = options.defaultValue || '';
        const showSave = options && options.showSave;
        saveBtn.style.display = showSave ? 'block' : 'none';
        const showTiming = options && options.showTiming;
        labelTiming.style.display = showTiming ? 'block' : 'none';
        inputTiming.style.display = showTiming ? 'block' : 'none';
        if (showTiming) {
            labelTiming.textContent = options.timingLabel || 'timing:';
        }
        inputTiming.value = showTiming ? (options.defaultTiming || '') : '';
        const btnShowDelete = options && options.btnShowDelete;
        deleteBtn.style.display = btnShowDelete ? 'block' : 'none';
        const btnShowCancel = options && options.btnShowCancel;
        cancelBtn.style.display = btnShowCancel ? 'block' : 'none';
        // items is een array van strings
        const content = items.map(i => `<p>${i}</p>`).join('');
        contentText.innerHTML = content;
        const contentLeft = Content.map(i => `<p>${i}</p>`).join('');
        leftContent.innerHTML = contentLeft;

        // pair & unpair // link & unlink
        const pairBtnN = options.pairBtnName || 'Pair';
        const unpairBtnN = options.unpairBtnName || 'Unpair';
        if (options && options.pairLabel && (options.onPair || options.onUnpair)) {
            pairLabelEl.style.display = 'block';
            pairLabelEl.textContent = options.pairLabel;
        } else {
            pairLabelEl.style.display = 'none';
        }

         if (options && options.onPair) {
            pairBtn.style.display = 'block';
            pairBtn.textContent = pairBtnN;
            pairBtn.onclick = () => {
                const sel = showDevicePopup ? devicePopup.value : undefined;
                closePopup();
                options.onPair(sel);
            };
        } else {
            pairBtn.style.display = 'none';
            pairBtn.onclick = null;
        }

        if (options && options.onUnpair) {
            unPairBtn.style.display = 'block';
            unPairBtn.textContent = unpairBtnN;
            unPairBtn.onclick = () => {
                const sel = showDevicePopup ? devicePopup.value : undefined;
                closePopup();
                options.onUnpair(sel);
            };
        } else {
            unPairBtn.style.display = 'none';
            unPairBtn.onclick = null;
        }
        
        if (options && options.onDelete) {
            
            deleteBtn.style.display = 'block';
            const exitDeleteMode = () => {
                deleteInfo.style.display = 'none';
                cancelBtn.style.display  = 'none';
                deleteBtn.style.display  = 'block';
                confirmBtn.style.display  = 'none';  
                pairBtn.style.display     =  'block';
                unPairBtn.style.display  = 'block';
            };
            const enterDeleteMode = () => {
                pairBtn.style.display    = 'none';
                unPairBtn.style.display = 'none';
                deleteBtn.style.display = 'none';    // Delete-knop verdwijnt

                deleteInfo.style.display = 'block';
                deleteInfo.textContent   = options.deleteInfo || '⚠️ Unpair devices first before deleting.';

                confirmBtn.style.display = 'inline-block';
                confirmBtn.textContent   = 'Confirm';
                confirmBtn.onclick = async (ev) => {
                try {
                        await options.onDelete();
                    closePopup();
                } catch (err) {
                    logStatus?.(`Error deleting: ${err.message}`, true);
                    exitDeleteMode();
                }
            };

                cancelBtn.style.display = 'inline-block';
                cancelBtn.onclick = (ev) => {
                exitDeleteMode();
                };
            };
            exitDeleteMode();
            deleteBtn.onclick = enterDeleteMode;
        } else {
            confirmBtn.style.display  = 'none';
        }

        // OK button
        saveBtn.onclick = () => {
            const value = showInput ? input.value : true;
            const timingValue = showTiming ? inputTiming.value : undefined;
            const deviceValue = showDevicePopup ? devicePopup.value : undefined;
            closePopup();
            if (options.onSave) options.onSave(value, timingValue, deviceValue);
        };

        // add/remove buttons handled above
        document.getElementById('popup').classList.add('open');

    };
    window.openPopup = openPopup;

    // popup close
    function closePopup() {
        document.getElementById('popup').classList.remove('open');
    }
    window.closePopup = closePopup;

    // example function for the edit button
    async function editRemote(remoteId, remoteName) {
        openPopup('Edit Remote', "Adjust the name/devices:", [
            'remote id: ' + remoteId,
            'name: ' + remoteName,
        ], [""], {
            showInput: true,
            showDevicePopup: true,
            btnShowDelete: true,
            showSave: true,
            defaultValue: remoteName,
            pairLabel: 'link / Unlink the remote',
            pairBtnName: 'Link',
            unpairBtnName: 'Unlink',
            onPair: async (deviceId) => {
                try {
                    const response = await fetch('/api/command', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ command: `linkRemote ${remoteId} ${deviceId}` })
                    });
                    const result = await response.json();
                    if (result.success) {
                        logStatus(result.message || 'Device linked.');
                    } else {
                        logStatus(result.message || 'Failed to link device.', true);
                    }
                    fetchAndDisplayRemotes();
                } catch (error) {
                    logStatus(`Error linking device: ${error.message}`, true);
                }
            },
            onUnpair: async (deviceId) => {
                try {
                    const response = await fetch('/api/command', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ command: `unlinkRemote ${remoteId} ${deviceId}` })
                    });
                    const result = await response.json();
                    if (result.success) {
                        logStatus(result.message || 'Device unlinked.');
                    } else {
                        logStatus(result.message || 'Failed to unlink device.', true);
                    }
                    fetchAndDisplayRemotes();
                } catch (error) {
                    logStatus(`Error unlinking device: ${error.message}`, true);
                }
            },
            onSave: async (newName) => {
                try {
                    if (newName.trim() && newName !== remoteName) {
                        const response = await fetch('/api/command', {
                            method: 'POST',
                            headers: { 'Content-Type': 'application/json' },
                            body: JSON.stringify({ RemoteId: remoteId, command: `editRemote ${newName}` })
                        });
                        const result = await response.json();
                        if (result.success) {
                            logStatus(result.message || 'Remote renamed.');
                        } else {
                            logStatus(result.message || 'Failed to rename remote.', true);
                        }
                        fetchAndDisplayRemotes();
                    }
                } catch (error) {
                    console.error('Error renaming remote:', error);
                    logStatus('Error renaming remote.', true);
                }
            },
            onDelete: async () => {
                try {
                    const response = await fetch('/api/command', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ command: `delRemote ${remoteId}` })
                    });
                    const result = await response.json();
                    if (result.success) {
                        logStatus(result.message || 'Remote removed.');
                    } else {
                        logStatus(result.message || 'Failed to remove remote.', true);
                    }
                    fetchAndDisplayRemotes();
                } catch (error) {
                    logStatus(`Error removing remote: ${error.message}`, true);
                }
            }
        });
    }
    window.editRemote = editRemote;

    window.addEventListener('DOMContentLoaded', () => {
        // Theme persistence
      const savedTheme = localStorage.getItem('theme');
      if (savedTheme === 'dark') {
        body.classList.add('dark-mode');
      }
      const helpDeviceBtn = document.getElementById('help-device');
      helpDeviceBtn.addEventListener('click', () => {
        openPopup('Help', "help device", [
                            'Nederlands',
                            'Stap 1: Devices → (+) ',
                            'Maak een nieuw screen aan.',
                            'Stap 2: Open het wieltje (Edit) ',
                            'Ga naar de instellingen van dat screen.',
                            'Stap 3: Zet de fysieke remote in pair mode',
                            'Het screen gaat kort op en neer.',
                            'Stap 4: In Edit → Pair',
                            'Klik Pair in de webpagina. Het screen gaat opnieuw op en neer.',
                            'Stap 5: → Gekoppeld.',
                        ], [
                            'English',
                            'Step 1: Devices → (+) ',
                            'Create a new screen.',
                            'Step 2: Open the gear (Edit) ',
                            'Go to that screen\'s settings.',
                            'Step 3: Put the physical remote in pairing mode',
                            'The screen will briefly move up and down.',
                            'Step 4: In Edit → Pair',
                            'Click Pair in the web page. The screen will move up and down again.',
                            'Step 5: → Paired.',
                        ],  {
                           showSave: false,
                       });
      });
      const helpRemoteBtn = document.getElementById('help-remote');
      helpRemoteBtn.addEventListener('click', () => {
        openPopup('Help', "help remote", [
                            'Nederlands',
                            'Stap 1: Remotes → (+) ',
                            'Maak een nieuwe remote aan.',
                            'Stap 2: Controleer ID',
                            'De ID moet overeenkomen met die van de fysieke remote.',
                            '(Is standaard ingevuld op basis van het laatste command.)',
                            'Stap 3: Klik op Edit (pop-up)',
                            'Link hier de eerder aangemaakte screen aan deze remote.',
                            'Stap 4: → Klaar.',
                        ],[
                            'English',
                            'Step 1: Remotes → (+) ',
                            'Create a new remote.',
                            'Step 2: Check the ID',
                            'The ID must match that of the physical remote.',
                            '(It is pre-filled based on the last command.)',
                            'Step 3: Click Edit (pop-up)',
                            'Link the previously created screen to this remote here.',
                            'Step 4: → Done.',
                        ], {
                           showSave: false,
                       });
      });
      // popup for adding devices
      const addpopup = document.getElementById('add-popup');
      addpopup.addEventListener('click', () => {
         openPopup('Add Device', "new device", [
            'here add your device',
           ], [""], {
           showSave: true,
           showInput: true,
           btnShowDelete: false,
           btnShowCancel: false,
           onSave: async (newName) => {
             if (newName.trim()) {
               try {
                 const response = await fetch('/api/command', {
                   method: 'POST',
                   headers: { 'Content-Type': 'application/json' },
                   body: JSON.stringify({ command: `new1W ${newName}` })
                 });
                 const result = await response.json();
                 if (result.success) {
                   logStatus(result.message || 'Device added.');
                   fetchAndDisplayDevices();
                 } else {
                   logStatus(result.message || 'Failed to add device.', true);
                 }
               } catch (e) {
                 logStatus(`Error adding device: ${e.message}`, true);
               }
             }
           }
         });
      });
      // popup for adding remotes
      const remotePopup = document.getElementById('remote-popup');
      remotePopup.addEventListener('click', () => {
         openPopup('Add Remote', 'Remote ID:', [
            'here add your remote',
         ], [""], {
           showSave: true,
           showInput: true,
           showTiming: true,
           btnShowDelete: false,
           btnShowCancel: false,
           timingLabel: 'Remote Name:',
           defaultValue: lastAddrInput.value.trim(),
           onSave: async (addr, newName) => {
             const id = addr.trim();
             if (!id) {
               logStatus('Please provide a remote ID.', true);
               return;
             }
             if (!newName.trim()) {
               logStatus('Please provide a remote name.', true);
               return;
             }
             try {
               const resp = await fetch('/api/command', {
                 method: 'POST',
                 headers: { 'Content-Type': 'application/json' },
                 body: JSON.stringify({ command: `newRemote ${id} ${newName}` })
               });
               const result = await resp.json();
               if (result.success) {
                 logStatus(result.message || 'Remote added.');
                 fetchAndDisplayRemotes();
               } else {
                 logStatus(result.message || 'Failed to add remote.', true);
               }
             } catch (e) {
               logStatus(`Error adding remote: ${e.message}`, true);
             }
           }
         });
      });

    });

    loadMqttConfig();
    fetchAndDisplayDevices();
    fetchAndDisplayRemotes();
    loadLastAddress();
});