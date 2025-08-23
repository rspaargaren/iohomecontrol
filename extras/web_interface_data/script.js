document.addEventListener('DOMContentLoaded', function() {
    const deviceListUL = document.getElementById('device-list');
    const deviceSelect = document.getElementById('device-select');
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
   
    async function fetchAndDisplayRemotes() {
        try {
            const response = await fetch('/api/remotes');
            if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
            const remotes = await response.json();

            const tbody = document.querySelector('#remote-table tbody');
            tbody.innerHTML = '';

            if (remotes.length === 0) {
                const tr = document.createElement('tr');
                tr.innerHTML = `<td colspan="3">No remotes available.</td>`;
                tbody.appendChild(tr);
                return;
            }

            remotes.forEach(remote => {
                const tr = document.createElement('tr');

                // gekoppelde devices tonen, neem aan remote.devices is array
                const linkedDevices = remote.devices && remote.devices.length > 0 
                    ? remote.devices.map(d => d.name).join(', ')
                    : '0 devices';

                tr.innerHTML = `
                    <td><div class="remote-id">${remote.id}</div></td>
                    <td>${remote.name}</td>
                    <td>${linkedDevices}</td>
                    <td>
                    <button class="btn edit" onclick="editRemote('${remote.id}', '${remote.name}')">Edit</button>
                    </td>
                `;
                tbody.appendChild(tr);
            });
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
                            'Status: ' + device.position,
                        ], {
                        showInput: true,
                        showTiming: true,
                        defaultValue: device.name,
                        defaultTiming: device.travel_time,
                        onConfirm: async (newName, newTiming) => {
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
                        onAdd: async () => {
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
                        onRemove: async () => {
                            try {
                                const response = await fetch('/api/command', {
                                    method: 'POST',
                                    headers: { 'Content-Type': 'application/json' },
                                    body: JSON.stringify({ deviceId: device.id, command: 'remove' })
                                });
                                const result = await response.json();
                                if (result.success) {
                                    logStatus(result.message || 'Device removed.');
                                } else {
                                    logStatus(result.message || 'Failed to remove device.', true);
                                }
                            } catch (e) {
                                logStatus(`Error removing device: ${e.message}`, true);
                            }
                        },
                        onDelete: async () => {
                            try {
                                const response = await fetch('/api/command', {
                                    method: 'POST',
                                    headers: { 'Content-Type': 'application/json' },
                                    body: JSON.stringify({ deviceId: device.id, command: 'del1W' })
                                });
                                const result = await response.json();
                                const devicesResp = await fetch('/api/devices');
                                const devices = await devicesResp.json();
                                const exists = devices.some(d => d.id === device.id);
                                if (result.success && !exists) {
                                    logStatus(result.message || 'Device deleted.');
                                } else {
                                    logStatus(result.message || 'Failed to delete device. Ensure it is unpaired.', true);
                                }
                                fetchAndDisplayDevices();
                            } catch (e) {
                                logStatus(`Error deleting device: ${e.message}`, true);
                            }
                        }
                    });
                updateDeviceFill(device.id, device.position || 0);

                listItem.appendChild(upButton);
                listItem.appendChild(stopButton);
                listItem.appendChild(downButton);
                listItem.appendChild(editButton);
                // TODO: Add buttons for simple actions if desired in future
                deviceListUL.appendChild(listItem);

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

      // Optioneel: opslaan in localStorage
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
    function openPopup(title, label, items = [], options = {}) {
        const labelInput = document.getElementById('label-input');
        const labelTiming = document.getElementById('label-timing');
        const inputTiming = document.getElementById('popup-input-timing');
        const addBtn = document.getElementById('popup-add');
        const removeBtn = document.getElementById('popup-remove');
        const deleteBtn = document.getElementById('popup-delete');
        const devicePopupLabel = document.querySelector('.device-popup-label');
        const devicePopup = document.querySelector('.device-popup');
        document.getElementById('popup-title').textContent = title;
        labelInput.textContent = label;

        const input = document.getElementById('popup-input');
        const showDevicePopup = options && options.showDevicePopup;
        devicePopupLabel.style.display = showDevicePopup ? 'block' : 'none';
        devicePopup.style.display = showDevicePopup ? 'block' : 'none';

        const showInput = options && options.showInput;
        input.style.display = showInput ? 'block' : 'none';
        labelInput.style.display = showInput ? 'block' : 'none';
        input.value = options.defaultValue || '';

        const showTiming = options && options.showTiming;
        labelTiming.style.display = showTiming ? 'block' : 'none';
        inputTiming.style.display = showTiming ? 'block' : 'none';
        inputTiming.value = options.defaultTiming || '';

        // items is een array van strings
        const content = items.map(i => `<p>${i}</p>`).join('');
        document.getElementById('popup-content').innerHTML = content;
        if (options && options.onAdd) {
            addBtn.style.display = 'block';
            addBtn.onclick = () => {
                closePopup();
                options.onAdd();
            };
        } else {
            addBtn.style.display = 'none';
            addBtn.onclick = null;
        }

        if (options && options.onRemove) {
            removeBtn.style.display = 'block';
            removeBtn.onclick = () => {
                closePopup();
                options.onRemove();
            };
        } else {
            removeBtn.style.display = 'none';
            removeBtn.onclick = null;
        }

        if (options && options.onDelete) {
            deleteBtn.style.display = 'block';
            deleteBtn.onclick = () => {
                closePopup();
                options.onDelete();
            };
        } else {
            deleteBtn.style.display = 'none';
            deleteBtn.onclick = null;
        }

        // OK button
        document.getElementById('popup-confirm').onclick = () => {
            const value = showInput ? input.value : true;
            const timingValue = showTiming ? inputTiming.value : undefined;
            closePopup();
            if (options.onConfirm) options.onConfirm(value, timingValue);
        };

        // add/remove buttons handled above
        document.getElementById('popup').classList.add('open');

    };


    // popup close
    function closePopup() {
        document.getElementById('popup').classList.remove('open');
    }

    window.closePopup = closePopup;

     // voorbeeld functie voor de edit-knop
    async function editRemote(remoteId, remoteName) {
        openPopup('Edit Remote', "Adjust the name/devices:", [
            'remote id: ' + remoteId,
            'name: ' + remoteName,
        ], {
            showInput: true,
            showDevicePopup: true,
            defaultValue: remoteName,
            onConfirm: async (newName) => {
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
                    }
                } catch (error) {
                    console.error('Error renaming remote:', error);
                    logStatus('Error renaming remote.', true);
                }
            }
        });
    }
    window.editRemote = editRemote;
    // Theme persistence on reload and open popup

    window.addEventListener('DOMContentLoaded', () => {
      const savedTheme = localStorage.getItem('theme');
      if (savedTheme === 'dark') {
        body.classList.add('dark-mode');
      }
      
      const remotePopup = document.getElementById('remote-popup');
      remotePopup.addEventListener('click', () => {
         openPopup('Add Remote', "new remote", [
            'here add your remote',
         ], {
           showInput: true,
           showDevicePopup: true,
           onConfirm: async (newName) => {
             
           }
         });
      });
      const addpopup = document.getElementById('add-popup');
      addpopup.addEventListener('click', () => {
         openPopup('Add Device', "new device", [
            'here add your device',
           ], {
           showInput: true,
           onConfirm: async (newName) => {
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

    });

    loadMqttConfig();
    fetchAndDisplayDevices();
    fetchAndDisplayRemotes();
    loadLastAddress();
});
