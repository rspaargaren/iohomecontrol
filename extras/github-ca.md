# GitHub CA Maintenance

`github-ca.pem` is the pinned root CA used for the firmware's HTTPS request to:

- `https://api.github.com/repos/<owner>/<repo>/releases/latest`

The firmware loads this PEM with `WiFiClientSecure::setCACert(...)` from the embedded file in `extras/github-ca.pem`.

## When to update it

Update this file when:

- TLS to `api.github.com` starts failing on-device
- GitHub changes the certificate chain used by `api.github.com`
- the current root CA is close to expiry

Do not update this file unless there is a concrete reason. Pinning a root CA is intentionally stable.

## How to inspect the current chain

From the repository root, run:

```sh
openssl s_client -showcerts -servername api.github.com -connect api.github.com:443 </dev/null
```

Look for the certificate chain. At the time this file was last updated, `api.github.com` chained to:

- leaf: `*.github.com`
- intermediate: `Sectigo Public Server Authentication CA DV E36`
- root: `Sectigo Public Server Authentication Root E46`

Prefer pinning the root CA, not the leaf certificate.

## How to update `github-ca.pem`

1. Fetch the current chain:

```sh
openssl s_client -showcerts -servername api.github.com -connect api.github.com:443 </dev/null
```

2. Copy the root CA certificate block from the output into `extras/github-ca.pem`.

3. Make sure the file contains exactly one complete PEM certificate:

- starts with `-----BEGIN CERTIFICATE-----`
- ends with `-----END CERTIFICATE-----`
- no truncated lines
- no extra text before or after the certificate

## How to validate the PEM locally

Before building or flashing, verify the file parses cleanly:

```sh
openssl x509 -in extras/github-ca.pem -text -noout
```

If this fails, the PEM file is malformed and the firmware will fail TLS initialization.

## How to validate the target server still chains to this root

You can also confirm the chain from your machine:

```sh
openssl s_client -showcerts -servername api.github.com -connect api.github.com:443 </dev/null
```

The root shown in the chain should match the certificate stored in `extras/github-ca.pem`.

## Build-time check

After updating the PEM, rebuild at least the supported targets:

```sh
rtk pio run -e HeltecLoraV2ESP32 -s
rtk pio run -e LilyGoLoraESP32 -s
```

## Runtime validation on device

After flashing, confirm the update check works in serial logs. A healthy run should show TLS success and then version output similar to:

```text
Version check: current=DEV1 latest=v1.0.1 update=yes
```

If TLS fails, inspect:

- serial logs
- `/api/info`
- MQTT version/update topics

## Known pitfalls

- A malformed PEM file can still build successfully but fail at runtime.
- Pinning a leaf certificate is brittle; use the root CA.
- If GitHub changes to a different trust chain, this file must be refreshed.
