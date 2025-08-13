document.addEventListener('DOMContentLoaded', function() {
    const deviceListUL = document.getElementById('device-list');
    const deviceSelect = document.getElementById('device-select');
    const commandInput = document.getElementById('command-input');
    const sendCommandButton = document.getElementById('send-command-button');
    const statusMessagesDiv = document.getElementById('status-messages');
    const MAX_LOGS = 20; // maximaal aantal logs
    const mqttUserInput = document.getElementById('mqtt-user');
    const mqttServerInput = document.getElementById('mqtt-server');
    const mqttPasswordInput = document.getElementById('mqtt-password');
    const mqttDiscoveryInput = document.getElementById('mqtt-discovery');
    const mqttUpdateButton = document.getElementById('mqtt-update');

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
                    openPopup('Edit Device', "here edit your device", "Adjust the name:", device.id, {
                        showInput: true,
                        defaultValue: device.name,
                        onConfirm: async (newName) => {
                            if (newName.trim()) {
                                try {
                                    const response = await fetch('/api/command', {
                                        method: 'POST',
                                        headers: { 'Content-Type': 'application/json' },
                                        body: JSON.stringify({ deviceId: device.id, command: `edit1W ${newName}` })
                                    });
                                    const result = await response.json();
                                    if (result.success) {
                                        logStatus(result.message || 'Device renamed.');
                                        fetchAndDisplayDevices();
                                    } else {
                                        logStatus(result.message || 'Failed to rename device.', true);
                                    }
                                } catch (e) {
                                    logStatus(`Error renaming device: ${e.message}`, true);
                                }
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
                updateDeviceFill('b60d1a', 50);

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

   async function fetchLogs() {
        try {
            const response = await fetch('/api/logs');

        if (!response.ok) return;
            const logs = await response.json();
            statusMessagesDiv.innerHTML = '';
            logs.forEach(line => {
                const p = document.createElement('p');
                p.textContent = line;
                statusMessagesDiv.appendChild(p);
            });
          statusMessagesDiv.scrollTop = statusMessagesDiv.scrollHeight;
        } catch (e) {
            console.error('Error fetching logs', e);
        }
        while (statusMessagesDiv.children.length > MAX_LOGS) {
            statusMessagesDiv.removeChild(statusMessagesDiv.firstChild);
        }
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

        // set background as gradient
        deviceEl.style.background = `linear-gradient(to bottom, ${color} ${percent}%, var(--color-input) ${percent}%)`;
    }

    // popup open
    function openPopup(title, text, label, data, options = {}) {
        const labelInput = document.getElementById('label-input');
        const labelTiming = document.getElementById('label-timing');
        const inputTiming = document.getElementById('popup-input-timing');
        const removeBtn = document.getElementById('popup-remove');
        document.getElementById('popup-title').textContent = title;
        document.getElementById('popup-text').textContent = text;
        labelInput.textContent = label;
        const input = document.getElementById('popup-input');
        input.style.display = (options && options.showInput) ? 'block' : 'none';
        input.value = options.defaultValue || '';
        
        if (options && options.onDelete) {
            removeBtn.style.display = 'block';
            labelTiming.style.display = 'block';
            inputTiming.style.display = 'block';
            removeBtn.onclick = () => {
                closePopup();
                options.onDelete();
            };
        } else {
            removeBtn.style.display = 'none';
            labelTiming.style.display = 'none';
            inputTiming.style.display = 'none';
            removeBtn.onclick = null;
        }

        // OK button
        document.getElementById('popup-confirm').onclick = () => {
            const value = (options && options.showInput) ? input.value : true;
            closePopup();
            if (options.onConfirm) options.onConfirm(value);
        };

        // pair button
        document.getElementById('popup-pair').onclick = () => {

        };
        document.getElementById('popup').classList.add('open');

        };


    // popup close
    function closePopup() {
        document.getElementById('popup').classList.remove('open');
    }

    window.closePopup = closePopup;

    // Theme persistence on reload and open popup

    window.addEventListener('DOMContentLoaded', () => {
      const savedTheme = localStorage.getItem('theme');
      if (savedTheme === 'dark') {
        body.classList.add('dark-mode');
      }
      const addpopup = document.getElementById('add-popup');
      addpopup.addEventListener('click', () => {
         openPopup('Add Device', "here add your device", "new device", null, {
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

    setInterval(fetchLogs, 2000);
    fetchLogs();
    loadMqttConfig();
    // Initial fetch of devices
    fetchAndDisplayDevices();
});