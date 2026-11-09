# Contributing

Thank you for your interest in LoRaScope. This repository accompanies a scientific
article, so the datasets are kept **frozen** to
match the published results. Corrections to them are welcome through an issue, but
changes will be released as a new tagged version, never by editing past data.

- **Questions and bug reports:** open a GitHub Issue with the firmware version
  (git commit), board, GNSS module and a serial log excerpt when relevant.
- **Firmware improvements:** open a pull request. Run
  `node tests/decoder.test.js` before submitting.
- **Credentials:** never include DevEUI, AppEUI, AppKey or other secrets in issues or
  pull requests.
- **Licensing:** by contributing you agree that your contribution is released under the
  license of the folder it modifies (see the main README).
