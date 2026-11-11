(() => {
  'use strict';
  const safeHttpUrl = value => {
    try { const url = new URL(value); return url.protocol === 'https:' || url.protocol === 'http:' ? url : null; }
    catch (_) { return null; }
  };
  document.addEventListener('ed-os:window-opened', event => {
    const detail = event.detail || {};
    if (detail.name !== 'E.D. Freedom') return;
    const root = detail.element;
    const searchForm = root.querySelector('[data-freedom-search]');
    const downloadForm = root.querySelector('[data-freedom-download]');
    const message = root.querySelector('[data-freedom-message]');
    if (!searchForm || !downloadForm || !message) return;
    searchForm.addEventListener('submit', event => {
      event.preventDefault();
      const query = searchForm.querySelector('input[name="q"]').value.trim();
      const engine = searchForm.querySelector('select[name="engine"]').value;
      if (!query) { message.textContent = 'Escribe algo que quieras buscar.'; return; }
      const endpoints = {
        duckduckgo: 'https://duckduckgo.com/?q=',
        google: 'https://www.google.com/search?q=',
        bing: 'https://www.bing.com/search?q='
      };
      const base = endpoints[engine] || endpoints.duckduckgo;
      window.open(base + encodeURIComponent(query), '_blank', 'noopener,noreferrer');
      message.textContent = 'Se ha solicitado abrir la búsqueda en una pestaña nueva. El buscador y la red pueden aplicar sus propias restricciones.';
    });
    downloadForm.addEventListener('submit', event => {
      event.preventDefault();
      const raw = downloadForm.querySelector('input[name="url"]').value.trim();
      const url = safeHttpUrl(raw);
      if (!url) { message.textContent = 'Enlace no válido. Utiliza una dirección que empiece por http:// o https://.'; return; }
      window.open(url.href, '_blank', 'noopener,noreferrer');
      message.textContent = 'Se ha abierto el enlace. El navegador o la página de origen decidirán si se descarga o se muestra el archivo.';
    });
  });
})();
