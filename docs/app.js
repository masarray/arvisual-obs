(() => {
  'use strict';

  const versionNodes = document.querySelectorAll('[data-release-version]');
  if (!versionNodes.length) return;

  fetch('https://api.github.com/repos/masarray/arvisual-obs/releases/latest', {
    headers: { Accept: 'application/vnd.github+json' }
  })
    .then((response) => {
      if (!response.ok) throw new Error(`GitHub API returned ${response.status}`);
      return response.json();
    })
    .then((release) => {
      const version = typeof release.tag_name === 'string' ? release.tag_name.trim() : '';
      if (!version) return;
      versionNodes.forEach((node) => { node.textContent = version; });
    })
    .catch(() => {
      // The static fallback remains visible when the API is unavailable or rate-limited.
    });
})();
