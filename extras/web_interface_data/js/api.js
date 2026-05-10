(function () {
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

    async function postJson(url, payload) {
        return requestJson(url, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(payload)
        });
    }

    async function uploadFile(url, file) {
        const formData = new FormData();
        formData.append("file", file);
        return requestJson(url, {
            method: "POST",
            body: formData
        });
    }

    async function downloadFile(url, filename) {
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
    }

    window.MiOpenApi = {
        downloadFile: downloadFile,
        postJson: postJson,
        requestJson: requestJson,
        uploadFile: uploadFile
    };
})();
