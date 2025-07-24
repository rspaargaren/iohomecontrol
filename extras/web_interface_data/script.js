document.addEventListener('DOMContentLoaded', function() {
    const deviceListUL = document.getElementById('device-list');
    const deviceSelect = document.getElementById('device-select');
    const commandInput = document.getElementById('command-input');
    const sendCommandButton = document.getElementById('send-command-button');
    const statusMessagesDiv = document.getElementById('status-messages');

    // Function to add a message to the status/log
    function logStatus(message, isError = false) {
        const p = document.createElement('p');
        p.textContent = message;
        if (isError) {
            p.style.color = 'red';
        }
        statusMessagesDiv.appendChild(p);
        // Scroll to the bottom of the status messages
        statusMessagesDiv.scrollTop = statusMessagesDiv.scrollHeight;
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
                const listItem = document.createElement('li');
                listItem.textContent = ' ';
                const nameSpan = document.createElement('span');
                nameSpan.textContent = device.name;

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
                downButton.classList.add('btn', 'close');
                downButton.onclick = () => {
                    fetch('/api/action', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ deviceId: device.id, action: 'close' })
                    }).then(r => r.json()).then(j => logStatus(j.message));
                };
                listItem.appendChild(upButton);
                listItem.appendChild(stopButton);
                listItem.appendChild(downButton);
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
    }
    // suggestions for command input
    const suggestions = ["add", "remove", "close", "open", "ls", "cat"];
    const input = document.getElementById('command-input');
    const suggestionBox = document.getElementById('suggestions');

    // Toon alle suggesties als knoppen
    suggestions.forEach(item => {
      const div = document.createElement('option');
      div.className = 'suggestion';
      div.textContent = item;

      // Voeg toe aan input bij klik
      div.addEventListener('click', () => {
        // Spatie toevoegen als laatste teken geen spatie is
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

    // Thema behouden bij herladen
    window.addEventListener('DOMContentLoaded', () => {
      const savedTheme = localStorage.getItem('theme');
      if (savedTheme === 'dark') {
        body.classList.add('dark-mode');
      }
    });

    setInterval(fetchLogs, 2000);
    fetchLogs();
    // Initial fetch of devices
    fetchAndDisplayDevices();
});