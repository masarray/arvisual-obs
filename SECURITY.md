# Security Policy

ArVisual is a native plugin that runs inside the OBS Studio process. Security reports are handled separately from normal bug reports.

## Supported versions

Security fixes are normally prepared for the latest published release. Users should upgrade to the newest release before reporting an issue that may already be resolved.

## Report a vulnerability privately

Do not open a public issue for a suspected vulnerability.

Email the maintainer at `ari.sulistiono@gmail.com` with:

- a clear description of the issue;
- affected ArVisual and OBS versions;
- operating system and architecture;
- reproduction steps or proof of concept;
- potential impact;
- suggested mitigation when known.

Avoid including credentials, private recordings or other unrelated sensitive data.

## Response process

The maintainer will review the report, determine whether it is reproducible and assess the affected release range. Confirmed issues may be fixed privately before coordinated public disclosure.

## Safe installation guidance

- Download release packages only from this repository’s GitHub Releases page.
- Verify that the repository owner is `masarray`.
- Do not combine the native DLL and effect files from different versions.
- Close OBS Studio before installing or upgrading.
- Build from source when independent verification is required.

## Scope

Examples of security-relevant reports include unsafe file handling, installer path traversal, arbitrary code execution, untrusted data causing memory corruption, or package workflow compromise.

Visual tuning differences, ordinary crashes without a security impact, installation support and performance regressions should use the public issue tracker.
